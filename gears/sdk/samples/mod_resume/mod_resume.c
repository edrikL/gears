/* Copyright 2007, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. Neither the name of Google Inc. nor the names of its contributors may be
 *     used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"
#include "util_script.h"
#include "http_connection.h"

#include "apr_lib.h"
#include "apr_strings.h"

#include <stdio.h>

module AP_MODULE_DECLARE_DATA resume_module;

typedef enum {
    STREAM,
    CACHE
} streaming_mode;

/* from mod_disk_cache */
typedef struct {
    const char* cache_root;

    apr_time_t expiration;
    apr_size_t min_checkpoint;
    apr_size_t min_filesize;
    apr_size_t max_filesize;

    streaming_mode initial_mode;
    streaming_mode resume_mode;
} resume_config;

#define DEFAULT_MIN_CHECKPOINT 1024*1024
#define DEFAULT_MAX_FILESIZE 1024*1024*1024
#define DEFAULT_EXPIRATION 60*60*24
#define DEFAULT_INITIAL_MODE STREAM
#define DEFAULT_RESUME_MODE CACHE

typedef struct {
    int expecting_103;
    const char *etag;
    apr_off_t offset;
    apr_off_t expected_length;          /* UNKNOWN_OFFSET if not specified */
    apr_off_t instance_length; /* UNKNOWN_OFFSET if '*' */

    char *filename;
    apr_file_t *fd;
    apr_time_t mtime;
    apr_off_t file_size;
    apr_off_t max_filesize; /* instance_length or conf->max_filesize */
    apr_off_t received_length; /* number of bytes actually received */
    apr_off_t skip; /* redundant bytes in the incoming stream to be skipped */
    apr_off_t unacked;

    streaming_mode mode;
    apr_bucket_brigade *bb; /* only use for checkpoints */
} resume_ctx;

#define ETAG_LENGTH 6
const apr_off_t UNKNOWN_OFFSET = (apr_off_t)-1;

/* from http_etag */
/* Generate the human-readable hex representation of an apr_uint64_t
 * (basically a faster version of 'sprintf("%llx")')
 */
#define HEX_DIGITS "0123456789abcdef"
static char *etag_uint64_to_hex(char *next, apr_uint64_t u)
{
    int printing = 0;
    int shift = sizeof(apr_uint64_t) * 8 - 4;
    do {
        unsigned short next_digit = (unsigned short)
                                    ((u >> shift) & (apr_uint64_t)0xf);
        if (next_digit) {
            *next++ = HEX_DIGITS[next_digit];
            printing = 1;
        }
        else if (printing) {
            *next++ = HEX_DIGITS[next_digit];
        }
        shift -= 4;
    } while (shift);
    *next++ = HEX_DIGITS[u & (apr_uint64_t)0xf];
    return next;
}

static int validate_etag(const char *etag) {
    int i;
    if (!etag) {
        return 0;
    }
    for (i = 0; i < ETAG_LENGTH; i++) {
        if (!apr_isalnum(etag[i])) {
            return 0;
        }
    }
    return (etag[ETAG_LENGTH] == '\0');
}

static int parse_expect(request_rec *r, resume_ctx *ctx, resume_config *conf) {
    /* TODO(fry):
     * const char *expect = apr_table_get(r->headers_in, "Expect");
     */
    const char *expect = apr_table_get(r->headers_in, "Pragma");

    if (!expect || !*expect) {
        /* Ignore absense of expect header */
        return 1;
    }

    /* from mod_negotiation */
    const char *name = ap_get_token(r->pool, &expect, 0);
    if (!name || strcasecmp(name, "103-checkpoint")) {
        /* Ignore expects that aren't for us */
        return 1;
    }
    ctx->expecting_103 = 1;

    while (*expect++ == ';') {
        /* Parameters ... */
        char *parm;
        char *cp;
        char *end;
        apr_off_t last_byte_pos;

        parm = ap_get_token(r->pool, &expect, 1);

        /* Look for 'var = value' */

        for (cp = parm; (*cp && !apr_isspace(*cp) && *cp != '='); ++cp);

        if (!*cp) {
            continue;           /* No '='; just ignore it. */
        }

        *cp++ = '\0';           /* Delimit var */
        while (*cp && (apr_isspace(*cp) || *cp == '=')) {
            ++cp;
        }

        if (*cp == '"') {
            ++cp;
            for (end = cp;
                 (*end && *end != '\n' && *end != '\r' && *end != '\"');
                 end++);
        }
        else {
            for (end = cp; (*end && !apr_isspace(*end)); end++);
        }
        if (*end) {
            *end = '\0';        /* strip ending quote or return */
        }

        if (!strcmp(parm, "resume")) {
            if (!validate_etag(cp)) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: Invalid etag %s", ctx->etag);
                return 0;
            }
            ctx->etag = cp;
        }
        else if (!strcmp(parm, "bytes")) {
            /* from byterange_filter */
            ctx->mode = conf->resume_mode;
            /* must have first_byte_pos (which is inclusive) */
            if (apr_strtoff(&ctx->offset, cp, &end, 10)
                || *end != '-'
                || ctx->offset < 0)
            {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: Invalid first_byte_pos %s", cp);
                return 0;
            }
            cp = end + 1;

            /* may have last_byte_pos (which is inclusive) */
            if (*cp == '/') {
                ctx->expected_length = UNKNOWN_OFFSET;
                if (!r->read_chunked) {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                                 "resume: Content-Length specified "
                                 "without last_byte_pos %s", cp);
                    return 0;
                }
            }
            else if (apr_strtoff(&last_byte_pos, cp, &end, 10)
                || *end != '/'
                || ctx->offset > last_byte_pos
                || last_byte_pos >= conf->max_filesize)
            {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: Invalid last_byte_pos %s", cp);
                return 0;
            }
            else if (r->read_chunked) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: last_byte_pos specified "
                             "without Content-Length %s", cp);
                return 0;
            }
            else { /* valid last_byte_pos */
                /* convert from last_byte_pos to expected_length */
                ctx->expected_length = last_byte_pos - ctx->offset + 1;
            }
            cp = end + 1;

            /* must have either instance_length or "*" */
            if (*cp == '*' && *(cp + 1) == '\0') {
                /* ctx->instance_length  remains  UNKNOWN_OFFSET; */
            }
            else if (apr_strtoff(&ctx->instance_length, cp, &end, 10)
                     || *end
                     || ctx->instance_length > conf->max_filesize
                     || (ctx->expected_length == UNKNOWN_OFFSET
                         ? ctx->offset >= ctx->instance_length
                         : ctx->offset + ctx->expected_length >
                             ctx->instance_length))
            {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: Invalid instance_length %s", cp);
                return 0;
            }
            else { /* valid instance_length */
                ctx->max_filesize = ctx->instance_length;
            }
        }
    }
    return 1;
}

/* send 103 (Checkpoint) */
static void send_103(resume_ctx *ctx, request_rec *r, resume_config *conf)
{
    apr_bucket_brigade *bb = ctx->bb;
    ap_filter_t *of = r->connection->output_filters;

    /* Copied from ap_send_interim_response to avoid saving status. */
    ap_fputstrs(of, bb, AP_SERVER_PROTOCOL, " ", "103 Checkpoint", CRLF, NULL);
    ap_fputstrs(of, bb, "ETag: \"", ctx->etag, "\"", CRLF, NULL);

    if (ctx->file_size > 0) {
        ap_fprintf(of, bb, "Range: 0-%d", ctx->file_size - 1);
        ap_fputs(of, bb, CRLF);
    }
    if (conf->expiration > 0) {
        apr_time_t expires = ctx->mtime + conf->expiration;
        char *timestr = apr_palloc(r->pool, APR_RFC822_DATE_LEN);
        apr_rfc822_date(timestr, expires);
        ap_fputstrs(of, bb, "Expires: ", timestr, CRLF, NULL);
    }

    ap_fputs(of, bb, CRLF);
    ap_fflush(of, bb);
    ctx->unacked = 0;
}

/* This is the resume filter, whose responsibilities are to:
 *  1) periodically send 100 (Continue) responses to the client
 *  2) persist the incoming byte stream in case of failure
 *  3) intercept resumes, injecting previous data
 */
apr_status_t resume_filter(ap_filter_t *f, apr_bucket_brigade *bb,
                           ap_input_mode_t mode, apr_read_type_e block,
                           apr_off_t readbytes)
{
    resume_config *conf = ap_get_module_config(f->r->server->module_config,
                                             &resume_module);
    resume_ctx *ctx = f->ctx;
    apr_status_t rv;
    apr_bucket *bucket;
    int i;
    int seen_eos;

    /* just get out of the way of things we don't want. */
    if (mode != AP_MODE_READBYTES) {
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }

    /* TODO(fry): ignore certain request types */

    if (!ctx) {
        f->ctx = ctx = apr_pcalloc(f->r->pool, sizeof(*ctx));
        ctx->bb = apr_brigade_create(f->r->pool, f->c->bucket_alloc);
        ctx->expecting_103 = 0;
        ctx->etag = NULL;
        ctx->offset = 0;
        ctx->expected_length = 0;
        ctx->instance_length = UNKNOWN_OFFSET;
        ctx->max_filesize = conf->max_filesize;
        ctx->received_length = 0;
        ctx->skip = 0;
        ctx->unacked = 0;
        ctx->mode = conf->initial_mode;

        /* read Content-Length and Transfer-Encoding, but don't trust them
         * populates f->r->remaining, f->r->read_chunked */
        rv = ap_setup_client_block(f->r, REQUEST_CHUNKED_DECHUNK);
        if (rv != APR_SUCCESS) {
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                         "resume: Error reading length and encoding %s",
                         ctx->filename);
            return rv;
        }

        if (!parse_expect(f->r, ctx, conf)) {
            return APR_EGENERAL;
        }

        if (ctx->etag) {
            apr_finfo_t finfo;

            ctx->filename = apr_pstrcat(f->r->pool, conf->cache_root,
                                        "/mod_resume.", ctx->etag, NULL);
            rv = apr_file_open(&ctx->fd, ctx->filename,
                               APR_READ | APR_WRITE | APR_BINARY |
                               APR_BUFFERED, 0, f->r->pool);
            if (rv != APR_SUCCESS) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                             "resume: Error creating byte archive %s",
                             ctx->filename);
                return rv;
            }

            rv = apr_file_info_get(&finfo, APR_FINFO_MTIME | APR_FINFO_SIZE,
                                   ctx->fd);
            if (rv != APR_SUCCESS) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                             "resume: Error stating byte archive %s",
                             ctx->filename);
                return rv;
            }
            ctx->mtime = finfo.mtime;
            ctx->file_size = finfo.size;
            if (ctx->offset > ctx->file_size) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                             "resume: Non-contiguous first_byte_pos %s",
                             ctx->filename);
                return APR_EGENERAL;
            }
            /* skip re-transmitted bytes */
            ctx->skip = ctx->file_size - ctx->offset;
        }
        else {
            if (!ctx->expecting_103) {
                /* look for excuses not to enable resumability */
                if (!f->r->read_chunked &&
                    (f->r->remaining < conf->min_filesize ||
                     f->r->remaining > conf->max_filesize))
                {
                    /* Note that this is an optimistic decision made based on
                     * Content-Length, which may be incorrect. See:
                     * http://mail-archives.apache.org/mod_mbox/httpd-modules-dev/200802.mbox/%3c003701c86ff6$4f2b39d0$6501a8c0@T60%3e
                     */
                    ap_remove_input_filter(f);
                    return ap_get_brigade(f->next, bb, mode, block, readbytes);
                }
            }
            else if (ctx->offset > 0) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                             "resume: Non-contiguous first_byte_pos %s",
                             ctx->filename);
                return APR_EGENERAL;
            }

            /* Create cache file.
             */

            ctx->filename = apr_pstrcat(f->r->pool, conf->cache_root,
                                        "/mod_resume.XXXXXX", NULL);
            ctx->file_size = 0;
            ctx->mtime = apr_time_now();
            rv = apr_file_mktemp(&ctx->fd, ctx->filename,
                                 APR_CREATE | APR_WRITE | APR_BINARY |
                                 APR_BUFFERED | APR_EXCL, f->r->pool);
            if (rv != APR_SUCCESS) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                             "resume: Error creating byte archive %s",
                             ctx->filename);
                return rv;
            }
             /* ETag is XXXXXX from filename.
             */
            /* TODO(fry): longer etag please! */
            ctx->etag = ap_strrchr(ctx->filename, '.');
            if (!ctx->etag++ || !validate_etag(ctx->etag)) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                             "resume: Invalid etag %s", ctx->etag);
                return 0;
            }
        }

        /* Always send a 103 as soon as we're committed to supporting
         * resumability.
         */
        send_103(ctx, f->r, conf);

        if (ctx->mode == STREAM && ctx->file_size > 0) {
            /* Stream any previous bytes in streaming mode */
            bucket = apr_bucket_file_create(ctx->fd, 0,
                                            ctx->file_size,
                                            f->r->pool,
                                            bb->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(bb, bucket);
            /* delay reading more data until previous bytes are re-sent */
            return APR_SUCCESS;
        }
    }

    do {
        apr_brigade_cleanup(bb);
        /* TODO(fry): get non-blocking
         *  if no data then 103 and get blocking
         *
         *  if not blocking then count bytes, otherwise always ack before
         *  blocking
         */

        /* Might read less than requested in order to ack progressively. */
        rv = ap_get_brigade(f->next, bb, mode, block,
                            (readbytes < conf->min_checkpoint ?
                             readbytes : conf->min_checkpoint));
        if (rv != APR_SUCCESS) {
            return rv;
        }

        for (bucket = APR_BRIGADE_FIRST(bb);
             bucket != APR_BRIGADE_SENTINEL(bb);
             bucket = APR_BUCKET_NEXT(bucket))
        {
            const char *str;
            apr_size_t length;

            if (APR_BUCKET_IS_EOS(bucket)) {
                seen_eos = 1;
                break;
            }
            if (APR_BUCKET_IS_FLUSH(bucket)) {
                continue;
            }

            rv = apr_bucket_read(bucket, &str, &length, APR_BLOCK_READ);
            if (rv != APR_SUCCESS) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                             "resume: Error reading bytes %s",
                             ctx->etag);
                return rv;
            }
            /* received_length includes skipped bytes */
            ctx->received_length += length;
            if (ctx->expected_length != UNKNOWN_OFFSET
                && ctx->received_length > ctx->expected_length)
            {
                apr_file_close(ctx->fd);
                apr_file_remove(ctx->filename, f->r->pool);
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                             "resume: Received incorrect number of bytes %s",
                             ctx->etag);
                return APR_EGENERAL;
            }

            if (ctx->skip > 0) {
                /* skip bytes we've already received */
                if (ctx->skip >= length) {
                    ctx->skip -= length;
                    continue;
                }

                str += ctx->skip;
                length -= ctx->skip;
                ctx->skip = 0;
            }
            ctx->mtime = apr_time_now();
            rv = apr_file_write_full(ctx->fd, str, length, NULL);
            if (rv != APR_SUCCESS) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                             "resume: Error archiving bytes %s",
                             ctx->etag);
                return rv;
            }
            ctx->unacked += length;
            ctx->file_size += length;
            if (ctx->file_size > ctx->max_filesize) {
                /* This is just too many bytes. We'd have noticed it earlier in
                 * non-chunked mode, but now that we know we must disapprove. */
                apr_file_close(ctx->fd);
                apr_file_remove(ctx->filename, f->r->pool);
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                             "resume: Received too many bytes %s",
                             ctx->etag);
                return APR_EGENERAL;
            }
            if (ctx->unacked >= conf->min_checkpoint) {
                send_103(ctx, f->r, conf);
            }
        }

        if (ctx->mode == STREAM && !seen_eos) {
            return APR_SUCCESS;
        }
    } while (!seen_eos);

    /* eos */
    if (ctx->unacked > 0) {
        send_103(ctx, f->r, conf);
    }
    apr_file_flush(ctx->fd);
    if (ctx->received_length != ctx->expected_length
        || (ctx->instance_length != UNKNOWN_OFFSET
            && ctx->file_size < ctx->instance_length)) {
        /* TODO(fry): 418 */
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, f->r->server,
                     "resume: Received insufficient bytes %s",
                     ctx->etag);
        return APR_EGENERAL;
    }
    /* Already checked above for ctx->file_size > ctx->instance_length */
    if (ctx->mode == CACHE) {
        apr_brigade_cleanup(bb);
        bucket = apr_bucket_file_create(ctx->fd, 0,
                                        (apr_size_t) ctx->file_size,
                                        f->r->pool, bb->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bb, bucket);
        bucket = apr_bucket_eos_create(f->c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bb, bucket);
    }
    return APR_SUCCESS;
}

static void *create_resume_config(apr_pool_t *p, server_rec *s)
{
    resume_config *conf = apr_pcalloc(p, sizeof(resume_config));

    /* set default values */
    conf->cache_root = NULL;
    conf->min_checkpoint = DEFAULT_MIN_CHECKPOINT;
    conf->min_filesize = DEFAULT_MIN_CHECKPOINT;
    conf->max_filesize = DEFAULT_MAX_FILESIZE;
    conf->expiration = apr_time_from_sec(DEFAULT_EXPIRATION);
    conf->initial_mode = DEFAULT_INITIAL_MODE;
    conf->resume_mode = DEFAULT_RESUME_MODE;

    return conf;
}

static const char *set_cache_root(cmd_parms *parms, void *in_struct_ptr,
                                  const char *root)
{
    resume_config *c = ap_get_module_config(parms->server->module_config,
                                             &resume_module);
    c->cache_root = root;
    return NULL;
}


static const char *set_min_checkpoint(cmd_parms *parms, void *in_struct_ptr,
                                  const char *min)
{
    resume_config *conf = ap_get_module_config(parms->server->module_config,
                                             &resume_module);
    int n = atoi(min);
    if (n <= 0) {
        return "ResumeCheckpointBytes must be positive";
    }
    conf->min_checkpoint = n;
    return NULL;
}

static const char *set_min_filesize(cmd_parms *parms, void *in_struct_ptr,
                                    const char *min)
{
    resume_config *conf = ap_get_module_config(parms->server->module_config,
                                             &resume_module);
    int n = atoi(min);
    if (n <= 0) {
        return "ResumeMinBytes must be positive";
    }
    conf->min_filesize = n;
    return NULL;
}

static const char *set_max_filesize(cmd_parms *parms, void *in_struct_ptr,
                                    const char *max)
{
    resume_config *conf = ap_get_module_config(parms->server->module_config,
                                             &resume_module);
    int n = atoi(max);
    if (n <= 0) {
        return "ResumeMaxBytes must be positive";
    }
    conf->max_filesize = n;
    return NULL;
}

static const char *set_expiration(cmd_parms *parms, void *in_struct_ptr,
                                  const char *expiration)
{
    resume_config *conf = ap_get_module_config(parms->server->module_config,
                                             &resume_module);
    int n = atoi(expiration);
    conf->expiration = apr_time_from_sec(n);
    return NULL;
}

static int get_mode(const char *str, streaming_mode *mode)
{
    if (!strcasecmp(str, "STREAM")) {
        *mode = STREAM;
        return 1;
    }
    else if (!strcasecmp(str, "CACHE")) {
        *mode = CACHE;
        return 1;
    }
    return 0;
}

static const char *set_streaming(cmd_parms *parms, void *in_struct_ptr,
                                  const char *initial, const char *resume)
{
    resume_config *conf = ap_get_module_config(parms->server->module_config,
                                             &resume_module);

    if (!get_mode(initial, &conf->initial_mode)) {
        return "Invalid ResumeStreamingMode; must be one of: STREAM, CACHE";
    }
    if (!get_mode(resume, &conf->resume_mode)) {
        return "Invalid ResumeStreamingMode; must be one of: STREAM, CACHE";
    }

    return NULL;
}


static const command_rec resume_cmds[] =
{
    AP_INIT_TAKE1("ResumeCacheRoot", set_cache_root, NULL, RSRC_CONF,
                  "The directory to store resume cache files"),
    AP_INIT_TAKE1("ResumeCheckpointBytes", set_min_checkpoint, NULL, RSRC_CONF,
                  "The minimum number of bytes to read between checkpoints."),
    AP_INIT_TAKE1("ResumeMinBytes", set_min_filesize, NULL, RSRC_CONF,
                  "The minimum number of bytes for a request to be resumable."),
    AP_INIT_TAKE1("ResumeMaxBytes", set_max_filesize, NULL, RSRC_CONF,
                  "The maximum number of bytes in a resumable request."),
    AP_INIT_TAKE1("ResumeExpiration", set_expiration, NULL, RSRC_CONF,
                  "The number of seconds until a resumed operation expires."),
    AP_INIT_TAKE2("ResumeStreamingMode", set_streaming, NULL, RSRC_CONF,
                   "The mode for handling bytes as they arrive. "
                   "The first parameter specifies how the initial request "
                   "should be dealt with; the second parameter applies to "
                   "resume requests. Valid values for both parameters are: "
                   "STREAM, CACHE. The default values are STREAM for initial "
                   "requests and CACHE for resume requests."),
    {NULL}
};

static void resume_register_hooks(apr_pool_t *p)
{
    ap_register_input_filter("RESUME", resume_filter, NULL, AP_FTYPE_TRANSCODE);
    /* TODO(fry): initial garbage collection of file and etag */
}

module AP_MODULE_DECLARE_DATA resume_module =
{
    STANDARD20_MODULE_STUFF,
    NULL,                  /* per-directory config creator */
    NULL,                  /* dir config merger -- default is to override */
    create_resume_config,  /* server config creator */
    NULL,                  /* server config merger */
    resume_cmds,           /* command table */
    resume_register_hooks, /* set up other request processing hooks */
};
