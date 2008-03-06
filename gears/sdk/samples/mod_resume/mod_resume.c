/* Copyright 2008, Google Inc.
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
#include "http_connection.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"
#include "util_script.h"

#include "apr_date.h"
#include "apr_lib.h"
#include "apr_strings.h"

/* TODO(fry): figure out the right way to import this */
#include "../cache/mod_cache.h"

#include <stdio.h>

module AP_MODULE_DECLARE_DATA resume_module;

/* should be consistent with httpd.h */
#define HTTP_CHECKPOINT                    103
#define HTTP_RESUME_INCOMPLETE             418

/* additions to status_lines in http_protocol */
#define HTTP_CHECKPOINT_STATUS_LINE        "103 Checkpoint"
#define HTTP_RESUME_INCOMPLETE_STATUS_LINE "418 Resume Incomplete"

#define EXPECT_CHECKPOINT                  "103-checkpoint"

typedef enum {
    STREAM,
    HOLD_AND_FORWARD
} streaming_mode;

/* from mod_disk_cache */
typedef struct {
    /* fabricated cache_conf to hijack methods from mod_cache */
    cache_server_conf *cache_conf;

    apr_array_header_t *resume_enable;
    apr_array_header_t *resume_disable;
    const char* input_cache_root;

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
#define DEFAULT_RESUME_MODE HOLD_AND_FORWARD

typedef struct {
    int input_initialized;           /* has input initialization happened? */

    /* derived from Expect header parameters */
    int expecting_checkpoint;        /* was an Expect: 103-continue received? */
    const char *etag;                /* the resume ETag in the Expect header */
    apr_off_t offset;
    apr_off_t expected_length;       /* UNKNOWN_OFFSET if not specified */
    apr_off_t instance_length;       /* UNKNOWN_OFFSET if '*' */

    /* info about the input cache file we are writing */
    char *filename;
    apr_file_t *fd;
    apr_time_t mtime;
    apr_off_t file_size;
    apr_off_t max_filesize;          /* instance_length or conf->max_filesize */
    apr_off_t received_length;       /* number of bytes actually received */
    apr_off_t skip;                  /* redundant incoming bytes to skip */
    apr_off_t unacked;               /* number of bytes we haven't acked yet */
    streaming_mode mode;             /* HOLD_AND_FORWARD or STREAM mode */
    apr_bucket_brigade *bb;          /* only use for checkpoints */

    /* for use in caching the reqeust's output */
    cache_provider_list *providers;  /* potential cache providers */
    const cache_provider *provider;  /* current cache provider */
    const char *provider_name;       /* current cache provider name */
    cache_handle_t *handle;          /* cache handle for saving output */
    char *key;                       /* the cache key */
} resume_request_rec;

#define ETAG_LENGTH 6
const apr_off_t UNKNOWN_OFFSET = (apr_off_t)-1;

static resume_request_rec *generate_request_rec(request_rec *r,
                                               resume_config *conf)
{
    resume_request_rec *ctx;
    ctx = ap_get_module_config(r->request_config, &resume_module);
    if (!ctx) {
        ctx = apr_pcalloc(r->pool, sizeof(resume_request_rec));
        ap_set_module_config(r->request_config, &resume_module, ctx);
    }

    ctx->input_initialized = 0;

    ctx->expecting_checkpoint = 0;
    ctx->etag = NULL;
    ctx->offset = 0;
    ctx->expected_length = UNKNOWN_OFFSET;
    ctx->instance_length = UNKNOWN_OFFSET;

    ctx->filename = NULL;
    ctx->fd = NULL;
    ctx->mtime = 0;
    ctx->file_size = 0;
    ctx->max_filesize = conf->max_filesize;
    ctx->received_length = 0;
    ctx->skip = 0;
    ctx->unacked = 0;
    ctx->mode = conf->initial_mode;
    ctx->bb = NULL; /* delay instantiation */

    ctx->providers = NULL;
    ctx->provider = NULL;
    ctx->provider_name = NULL;
    ctx->handle = NULL;
    ctx->key = NULL;

    return ctx;
}

/* Handles for resume filters, resolved at startup to eliminate a
 * name-to-function mapping on each request.
 */
static ap_filter_rec_t *resume_input_filter_handle;
static ap_filter_rec_t *resume_save_filter_handle;
static ap_filter_rec_t *resume_out_filter_handle;

/*******************************************************************************
 * Shared methods                                                              *
 ******************************************************************************/

static int validate_etag(const char *etag)
{
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

/* Sets headers that are common between 103 and 418 responses, ensuring
 * consistent implementations in both cases.
 */
static void set_shared_resume_headers(request_rec *r, resume_request_rec *ctx,
                                      apr_table_t *headers)
{
    if (ctx->file_size > 0) {
        apr_table_setn(r->err_headers_out, "Range",
                       apr_psprintf(r->pool, "0-%d", ctx->file_size - 1));
    }
}

/*******************************************************************************
 * Handler methods                                                             *
 ******************************************************************************/

static cache_provider_list *resume_get_providers(request_rec *r,
                                                 resume_config *conf,
                                                 apr_uri_t uri)
{
    return ap_cache_get_providers(r, conf->cache_conf, uri);
}

static int return_incomplete(request_rec *r, resume_config *conf,
                             resume_request_rec *ctx)
{
    /* manual status_line specification since we're not in http_protocol */
    r->status_line = HTTP_RESUME_INCOMPLETE_STATUS_LINE;
    set_shared_resume_headers(r, ctx, r->err_headers_out);
    return HTTP_RESUME_INCOMPLETE;
}

static int parse_expect(request_rec *r, resume_config *conf,
                        resume_request_rec *ctx)
{
    const char *expect = apr_table_get(r->headers_in,
                                       /* TODO(fry): "Expect"); */
                                       "Pragma");

    if (!expect || !*expect) {
        /* Ignore absense of expect header */
        return OK;
    }

    /* from mod_negotiation */
    const char *name = ap_get_token(r->pool, &expect, 0);
    if (!name || strcasecmp(name, EXPECT_CHECKPOINT)) {
        /* Ignore expects that aren't for us */
        return OK;
    }
    ctx->expecting_checkpoint = 1;
    ctx->expected_length = 0;

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
                             "resume: Invalid etag \"%s\"", ctx->etag);
                return HTTP_BAD_REQUEST;
            }
            ctx->etag = cp;
        }
        else if (!strcmp(parm, "bytes")) {
            /* from byterange_filter */
            ctx->mode = conf->resume_mode;
            /* must have first_byte_pos (which is inclusive) */
            if (apr_strtoff(&ctx->offset, cp, &end, 10) || *end != '-') {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: Invalid first_byte_pos \"%s\"", cp);
                return HTTP_BAD_REQUEST;
            }
            cp = end + 1;

            /* may have last_byte_pos (which is inclusive) */
            if (*cp == '/') {
                last_byte_pos = UNKNOWN_OFFSET;
            }
            else if (apr_strtoff(&last_byte_pos, cp, &end, 10) || *end != '/') {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: Invalid last_byte_pos \"%s\"", cp);
                return HTTP_BAD_REQUEST;
            }
            cp = end + 1;

            /* must have either instance_length or "*" */
            if (*cp == '*' && *(cp + 1) == '\0') {
                ctx->instance_length = UNKNOWN_OFFSET; /* currently redundant */
            }
            else if (apr_strtoff(&ctx->instance_length, cp, &end, 10) || *end) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: Invalid instance_length \"%s\"", cp);
                return HTTP_BAD_REQUEST;
            }

            /* syntactic checks of byte ranges */

            /* ctx->offset */
            if (ctx->offset < 0) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: Invalid first_byte_pos \"%s\"", cp);
                return HTTP_BAD_REQUEST;
            }

            /* ctx->expected_length */
            if (last_byte_pos == UNKNOWN_OFFSET) {
                ctx->expected_length = UNKNOWN_OFFSET;
                if (!r->read_chunked) {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                                 "resume: Content-Length specified "
                                 "without last_byte_pos \"%s\"", cp);
                    return HTTP_BAD_REQUEST;
                }
            }
            else if (ctx->offset > last_byte_pos
                     || last_byte_pos >= conf->max_filesize) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: Invalid last_byte_pos \"%s\"", cp);
                return HTTP_BAD_REQUEST;
            }
            else if (r->read_chunked) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: last_byte_pos specified "
                             "without Content-Length \"%s\"", cp);
                return HTTP_BAD_REQUEST;
            }
            else {
                /* convert from last_byte_pos to expected_length */
                ctx->expected_length = last_byte_pos - ctx->offset + 1;
            }

            /* ctx->instance_length */
            if (ctx->instance_length == UNKNOWN_OFFSET) {
                /* this space intentionally left blank */
            }
            else if (ctx->expected_length == UNKNOWN_OFFSET) {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: instance_length specified "
                             "without last_byte_pos \"%s\"", cp);
                return HTTP_BAD_REQUEST;
            }
            else if (ctx->instance_length > conf->max_filesize ||
                     ctx->offset + ctx->expected_length > ctx->instance_length)
            {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                             "resume: Invalid instance_length \"%s\"", cp);
                return HTTP_BAD_REQUEST;
            }
            else {
                ctx->max_filesize = ctx->instance_length;
            }
        }
    }

    /* cross-parameter validation */
    if (!ctx->etag && ctx->offset > 0) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                     "resume: Non-zero initial first_byte_pos \"%d\"",
                     ctx->offset);
        /* client can retry with contiguous range */
        return return_incomplete(r, conf, ctx);
    }

    return OK;
}

static int send_cached_response(request_rec *r, resume_request_rec *ctx)
{
    apr_bucket_brigade *bb;
    apr_status_t rv;

    /* from mod_cache */
    ap_filter_t *next;

    /* Remove all filters that are before the resume_out filter. This
     * ensures that we kick off the filter stack with our resume_out
     * filter being the first in the chain. This make sense because we
     * want to restore things in the same manner as we saved them.
     * There may be filters before our resume_out filter, because:
     *     1. We call ap_set_content_type during cache_select. This
     *        causes Content-Type specific filters to be added.
     *     2. We call the insert_filter hook. This causes filters like
     *        the ones set with SetOutputFilter to be added.
     */
    next = r->output_filters;
    while (next && (next->frec != resume_out_filter_handle)) {
        ap_remove_output_filter(next);
        next = next->next;
    }

    /* kick off the filter stack */
    bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
    rv = ap_pass_brigade(r->output_filters, bb);
    if (rv != APR_SUCCESS) {
        if (rv != AP_FILTER_ERROR) {
            ap_log_error(APLOG_MARK, APLOG_ERR, rv, r->server,
                         "resume: Error returning %s cached data",
                         ctx->provider_name);
        }
        return rv;
    }

    return OK;
}

static int read_input_filters(request_rec *r)
{
    apr_bucket_brigade *bb;
    apr_status_t rv;
    int seen_eos;

    /* Partial request, necessitating a 418.
     *
     * It is _incorrect_ to allow downstream handlers to process this request as
     * they will incorrectly interpret the input EOS. So instead we read all of
     * the bytes ourself, ignoring them as the input filter is already storing
     * them.
     */

    /* from mod_cgi */
    bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
    seen_eos = 0;
    do {
        apr_bucket *bucket;
        rv = ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES,
                            APR_BLOCK_READ, HUGE_STRING_LEN);
        if (rv != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                          "Error reading request entity data");
            return HTTP_INTERNAL_SERVER_ERROR;
        }

        /* Search for EOS (even though we only expect a single bucket) */
        for (bucket = APR_BRIGADE_FIRST(bb);
             bucket != APR_BRIGADE_SENTINEL(bb);
             bucket = APR_BUCKET_NEXT(bucket))
        {
            if (APR_BUCKET_IS_EOS(bucket)) {
                seen_eos = 1;
                break;
            }
        }
        apr_brigade_cleanup(bb);
    } while (!seen_eos);
    return OK;
}

static int create_input_buffer(request_rec *r, resume_config *conf,
                               resume_request_rec *ctx)
{
    apr_status_t rv;

    ctx->filename = apr_pstrcat(r->pool, conf->input_cache_root,
                                "/mod_resume.XXXXXX", NULL);
    ctx->file_size = 0;
    ctx->mtime = apr_time_now();
    rv = apr_file_mktemp(&ctx->fd, ctx->filename,
                         APR_CREATE | APR_EXCL
                         | APR_READ | APR_WRITE | APR_APPEND
                         | APR_BINARY | APR_BUFFERED,
                         r->pool);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                     "resume: Error creating byte archive \"%s\"",
                     ctx->filename);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    /* ETag is XXXXXX from filename. */
    /* TODO(fry): longer etag please!
     *            also cryptographically random?
     *            use etag_uint64_to_hex and http_etag
     */
    ctx->etag = ap_strrchr(ctx->filename, '.');
    if (!ctx->etag++ || !validate_etag(ctx->etag)) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                     "resume: Invalid etag generated \"%s\"", ctx->etag);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    return OK;
}

static int open_input_buffer(request_rec *r, resume_config *conf,
                             resume_request_rec *ctx)
{
    apr_status_t rv;
    apr_finfo_t finfo;

    ctx->filename = apr_pstrcat(r->pool, conf->input_cache_root, "/mod_resume.",
                                ctx->etag, NULL);

    rv = apr_stat(&finfo, ctx->filename, 0, r->pool);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                     "resume: ETag \"%s\" not found", ctx->etag);
        /* client can not retry with this invalid etag */
        return HTTP_EXPECTATION_FAILED;
    }

    rv = apr_file_open(&ctx->fd, ctx->filename,
                       APR_READ | APR_WRITE | APR_APPEND
                       | APR_BINARY | APR_BUFFERED, 0, r->pool);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                     "resume: Error creating byte archive \"%s\"",
                     ctx->filename);
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    rv = apr_file_info_get(&finfo, APR_FINFO_MTIME | APR_FINFO_SIZE, ctx->fd);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                     "resume: Error stating byte archive \"%s\"",
                     ctx->filename);
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    ctx->mtime = finfo.mtime;
    ctx->file_size = finfo.size;
    if (ctx->offset > ctx->file_size) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                     "resume: Non-contiguous first_byte_pos \"%s\"",
                     ctx->filename);
        /* client can retry with contiguous range */
        return return_incomplete(r, conf, ctx);
    }

    /* skip re-transmitted bytes */
    ctx->skip = ctx->file_size - ctx->offset;
    return OK;
}

/* This is the resume handler, in charge of installing all filters necessary
 * for this request.
 *
 * Note that this is very similar to mod_cache, and we only avoid conflicting
 * with them because mod_cache operates on GET commands, and we operate on
 * POST/PUT commands.
 */
static int resume_handler(request_rec *r)
{
    resume_config *conf;
    resume_request_rec *ctx;
    apr_status_t rv;

    /* Delay initialization until we know we are handling a POST/PUT */
    if (r->method_number != M_POST && r->method_number != M_PUT) {
        return DECLINED;
    }

    conf = ap_get_module_config(r->server->module_config, &resume_module);

    ctx = generate_request_rec(r, conf);
    ctx->providers = resume_get_providers(r, conf, r->parsed_uri);
    if (!ctx->providers) {
        ap_log_error(APLOG_MARK, APLOG_WARNING, 0, r->server,
                     "resume: No cache provider enabled for output caching");
        /* we continue, as we can still do input buffering */
    }
    /* TODO(fry): check whether or not resume is enabled for this request */
    /* TODO(fry): can we use conf->cache_conf to manage output cache root? */

    /* read Content-Length and Transfer-Encoding, but don't trust them
     * populates r->remaining, r->read_chunked */
    rv = ap_setup_client_block(r, REQUEST_CHUNKED_DECHUNK);
    if (rv != OK) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                     "resume: Error reading length and encoding \"%s\"",
                     ctx->filename);
        return rv;
    }

    rv = parse_expect(r, conf, ctx);
    if (rv != OK) {
        return rv;
    }

    if (ctx->etag) {
        /* If a final response was already generated, return it again. */
        rv = resume_cache_select(r, ctx);
        if (rv == OK) {
            /* resend the cached response */
            ap_add_output_filter_handle(resume_out_filter_handle, ctx, r,
                                        r->connection);
            return send_cached_response(r, ctx);
        }

        rv = open_input_buffer(r, conf, ctx);
        if (rv != OK) {
            /* invalid etag or error opening file */
            return rv;
        }
    }
    else {
        /* Look for reasons not to enable resumability, testing conditions one
         * by one to be clear and unambiguous. */
        if (ctx->expecting_checkpoint) {
            /* always enable resumability if an Expect Checkpoint is received */
        }
        else if (r->proto_num < 1001) {
            /* don't send interim response to HTTP/1.0 Clients
             * (they are ignored by ap_send_interim_response anyway)
             */
            return DECLINED;
        }
        else if (!r->read_chunked
                 && (r->remaining < conf->min_filesize
                     || r->remaining > conf->max_filesize))
        {
            /* For non-chunked transfers, make an optimistic decision based on
             * Content-Length, even though it may be slightly off. See:
             * http://mail-archives.apache.org/mod_mbox/httpd-modules-dev/200802.mbox/%3c003701c86ff6$4f2b39d0$6501a8c0@T60%3e
             */
            return DECLINED;
        }

        rv = create_input_buffer(r, conf, ctx);
        if (rv != OK) {
            return rv;
        }
    }

    ap_add_input_filter_handle(resume_input_filter_handle, ctx, r,
                               r->connection);
    if (!ctx->expecting_checkpoint
        || ctx->skip + ctx->expected_length == ctx->instance_length
        || (ctx->expected_length == UNKNOWN_OFFSET
            && ctx->instance_length == UNKNOWN_OFFSET)) {
        /* All bytes are present, no 418 required.
         * We expect a final response, so install the save output filter.
         */
        ap_add_output_filter_handle(resume_save_filter_handle, ctx, r,
                                    r->connection);
        return DECLINED;
    }

    /* no reason to stream as we aren't reading bytes */
    ctx->mode = HOLD_AND_FORWARD;
    /* read and buffer input bytes */
    rv = read_input_filters(r);
    if (rv != OK) {
        return rv;
    }
    return return_incomplete(r, conf, ctx);
}

/*******************************************************************************
 * Input Filter methods                                                        *
 ******************************************************************************/

/* TODO use this to generate etags */
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

/* send 103 (Checkpoint) */
static void send_checkpoint(request_rec *r, resume_config *conf,
                            resume_request_rec *ctx)
{
    apr_bucket_brigade *bb = ctx->bb;
    ap_filter_t *of = r->connection->output_filters;
    int old_status;
    const char *old_status_line;

    /* flush the bytes which we will now ack */
    apr_file_flush(ctx->fd);

    old_status = r->status;
    old_status_line = r->status_line;
    r->status = HTTP_CHECKPOINT;
    r->status_line = HTTP_CHECKPOINT_STATUS_LINE;

    set_shared_resume_headers(r, ctx, r->headers_out);
    apr_table_setn(r->headers_out, "ETag", ctx->etag);
    if (conf->expiration > 0) {
        apr_time_t expires = ctx->mtime + conf->expiration;
        char *timestr = apr_palloc(r->pool, APR_RFC822_DATE_LEN);
        apr_rfc822_date(timestr, expires);
        apr_table_setn(r->headers_out, "Expires", timestr);
    }

    ap_send_interim_response(r, 1);
    ctx->unacked = 0;
    r->status = old_status;
    r->status_line = old_status_line;
}

static apr_status_t read_bytes(request_rec *r, apr_bucket_brigade *bb ,
                               resume_config *conf, resume_request_rec *ctx,
                               int *seen_eos)
{
    apr_bucket *bucket;
    apr_status_t rv;

    for (bucket = APR_BRIGADE_FIRST(bb);
         bucket != APR_BRIGADE_SENTINEL(bb);
         bucket = APR_BUCKET_NEXT(bucket))
    {
        const char *str;
        apr_size_t length;

        if (APR_BUCKET_IS_EOS(bucket)) {
            *seen_eos = 1;
            return APR_SUCCESS;
        }
        if (APR_BUCKET_IS_FLUSH(bucket)) {
            continue;
        }

        rv = apr_bucket_read(bucket, &str, &length, APR_BLOCK_READ);
        if (rv != APR_SUCCESS) {
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                         "resume: Error reading bytes for \"%s\"",
                         ctx->etag);
            return rv;
        }
        /* received_length includes skipped bytes */
        ctx->received_length += length;
        if (ctx->expected_length != UNKNOWN_OFFSET
            && ctx->received_length > ctx->expected_length)
        {
            apr_file_close(ctx->fd);
            apr_file_remove(ctx->filename, r->pool);
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                         "resume: Received incorrect number of bytes for "
                         "\"%s\"", ctx->etag);
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
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                         "resume: Error archiving bytes for \"%s\"",
                         ctx->etag);
            return rv;
        }
        ctx->unacked += length;
        ctx->file_size += length;
        if (ctx->file_size > ctx->max_filesize) {
            /* This is just too many bytes. We'd have noticed it earlier in
             * non-chunked mode, but now that we know we must disapprove. */
            apr_file_close(ctx->fd);
            apr_file_remove(ctx->filename, r->pool);
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                         "resume: Received too many bytes for \"%s\"",
                         ctx->etag);
            return APR_EGENERAL;
        }
        if (ctx->unacked >= conf->min_checkpoint) {
            send_checkpoint(r, conf, ctx);
        }
    }
    return APR_SUCCESS;
}

/* This is the resume filter, whose responsibilities are to:
 *  1) periodically send 100 (Continue) responses to the client
 *  2) persist the incoming byte stream in case of failure
 *  3) intercept resumes, injecting previous data
 *
 * We can't use a mod_cache provider since they require all bytes to be
 * written from a single request.
 */
static apr_status_t resume_input_filter(ap_filter_t *f,
                                        apr_bucket_brigade *bb,
                                        ap_input_mode_t mode,
                                        apr_read_type_e block,
                                        apr_off_t readbytes)
{
    resume_config *conf = ap_get_module_config(f->r->server->module_config,
                                               &resume_module);
    resume_request_rec *ctx = f->ctx;
    request_rec *r = f->r;
    apr_status_t rv;
    int seen_eos;

    /* just get out of the way of things we don't want. */
    if (mode != AP_MODE_READBYTES) {
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }
    if (!ctx) {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, r->server,
                     "resume: RESUME input filter enabled unexpectedly");
        ap_remove_input_filter(f);
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }

    if (!ctx->input_initialized) {
        ctx->input_initialized = 1;
        ctx->bb = apr_brigade_create(r->pool, f->c->bucket_alloc);

        /* Always send a checkpoint as soon as we're committed to supporting
         * resumability.
         */
        send_checkpoint(r, conf, ctx);

        if (ctx->mode == STREAM && ctx->file_size > 0) {
            /* Stream any previous bytes in streaming mode */
            apr_bucket *bucket;
            bucket = apr_bucket_file_create(ctx->fd, 0,
                                            ctx->file_size,
                                            r->pool,
                                            bb->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(bb, bucket);
            /* delay reading more data until previous bytes are re-sent */
            return APR_SUCCESS;
        }
    }

    if (ctx->mode == HOLD_AND_FORWARD) {
        /* we don't return until EOS, so we might as well block while reading */
        block = APR_BLOCK_READ;
    }
    do {
        rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
        if (rv != APR_SUCCESS) {
            return rv;
        }

        rv = read_bytes(r, bb, conf, ctx, &seen_eos);
        if (rv != APR_SUCCESS) {
            return rv;
        }

        if (ctx->mode == HOLD_AND_FORWARD) {
            apr_brigade_cleanup(bb);
        }
        else if (!seen_eos) { /* STREAM mode */
            return APR_SUCCESS;
        }
    } while (!seen_eos);

    /* eos */

    if (ctx->unacked > 0) {
        /* ack any remaining bytes as the response might be slow in coming */
        send_checkpoint(r, conf, ctx);
    }

    if ((ctx->expected_length != UNKNOWN_OFFSET
         && ctx->received_length != ctx->expected_length)
        || (ctx->instance_length != UNKNOWN_OFFSET
            && ctx->file_size < ctx->instance_length))
        /* Already checked above for ctx->file_size > ctx->instance_length */
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                     "resume: Received insufficient bytes for \"%s\"",
                     ctx->etag);
        return APR_EGENERAL;
    }

    if (ctx->mode == HOLD_AND_FORWARD) {
        /* TODO(fry): rename file during processing, cleanup afterwords or on
         *            server crash
         */
        apr_bucket *bucket;
        apr_off_t offset = 0;
        apr_file_seek(ctx->fd, APR_SET, &offset);
        /* bb was just cleaned above */
        bucket = apr_bucket_file_create(ctx->fd, 0,
                                        (apr_size_t) ctx->file_size,
                                        r->pool, bb->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bb, bucket);
        bucket = apr_bucket_eos_create(f->c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bb, bucket);
    }
    else if (ctx->filename) {
        /* TODO(fry): why does EOS logic happen multiple times? */
        /* we won't need the file again, so blow it away now */
        apr_file_close(ctx->fd);
        rv = apr_file_remove(ctx->filename, r->pool);
        if (rv == APR_SUCCESS) {
            ctx->filename = NULL;
        }
        else {
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                         "resume: Error removing input cache file %s",
                         ctx->filename);
        }
    }

    return APR_SUCCESS;
}

/*******************************************************************************
 * Output Filter methods                                                       *
 ******************************************************************************/

/* from mod_cache */
/* Get the content length if known. */
static apr_off_t get_size(request_rec *r, apr_bucket_brigade *in)
{
    apr_off_t size;
    const char *cl = apr_table_get(r->err_headers_out, "Content-Length");
    if (!cl) {
        cl = apr_table_get(r->headers_out, "Content-Length");
    }

    if (cl) {
        char *errp;
        if (apr_strtoff(&size, cl, &errp, 10) || *errp || size < 0) {
            cl = NULL; /* parse error, see next 'if' block */
        }
    }

    if (!cl) {
        /* if we don't get the content-length, see if we have all the
         * buckets and use their length to calculate the size
         */
        apr_bucket *e;
        int all_buckets_here = 0;
        size = 0;
        for (e = APR_BRIGADE_FIRST(in);
             e != APR_BRIGADE_SENTINEL(in);
             e = APR_BUCKET_NEXT(e))
        {
            if (APR_BUCKET_IS_EOS(e)) {
                all_buckets_here = 1;
                break;
            }
            if (APR_BUCKET_IS_FLUSH(e)) {
                continue;
            }
            if (e->length == (apr_size_t)-1) {
                break;
            }
            size += e->length;
        }
        if (!all_buckets_here) {
            size = -1;
        }
    }

    return size;
}

/* from mod_cache */
/* Read the date. Generate one if one is not supplied */
static apr_off_t get_date(request_rec *r)
{
    apr_time_t date;
    const char *dates = apr_table_get(r->err_headers_out, "Date");
    if (!dates) {
        dates = apr_table_get(r->headers_out, "Date");
    }

    if (dates) {
        date = apr_date_parse_http(dates);
    }
    else {
        date = APR_DATE_BAD;
    }

    return date;
}

/* standin for cache_generate_key */
static apr_status_t resume_generate_key(request_rec *r, resume_request_rec *ctx,
                                        char **key)
{
    if (!ctx->key) {
        /* generate a key that won't conflict with mod_cache URI keys */
        ctx->key = apr_pstrcat(r->pool, "mod_resume://", ctx->etag, NULL);
    }
    *key = ctx->key;
    return APR_SUCCESS;
}

/* from cache_storage */
static int resume_cache_create_entity(request_rec *r, resume_request_rec *ctx,
                                      apr_off_t size)
{
    char *key;
    cache_handle_t *h;
    cache_provider_list *providers;
    apr_status_t rv;

    rv = resume_generate_key(r, ctx, &key);
    if (rv != APR_SUCCESS) {
        return rv;
    }
    h = apr_pcalloc(r->pool, sizeof(cache_handle_t));

    for (providers = ctx->providers; providers; providers = providers->next) {
        rv = providers->provider->create_entity(h, r, key, size);
        switch(rv) {
            case OK: {
                ctx->handle = h;
                ctx->provider = providers->provider;
                ctx->provider_name = providers->provider_name;
                return OK;
            }
            case DECLINED: {
                continue;
            }
            default: {
                return rv;
            }
        }
    }
    return DECLINED;
}

/* from mod_cache */
/* Cache server response. */
static int resume_save_filter(ap_filter_t *f, apr_bucket_brigade *in)
{
    resume_config *conf = ap_get_module_config(f->r->server->module_config,
                                               &resume_module);
    resume_request_rec *ctx = f->ctx;
    request_rec *r = f->r;
    int rv;
    apr_off_t size;
    cache_info *info = apr_pcalloc(r->pool, sizeof(cache_info));

    /* from mod_cache */
    /* We only set info->status upon the initial creation. */
    info->status = r->status;

    if (!ctx) {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, r->server,
                     "resume: RESUME_SAVE output filter enabled unexpectedly");
        ap_remove_output_filter(f);
        return ap_pass_brigade(f->next, in);
    }

    size = get_size(r, in);

    /* from mod_cache */
    /* no cache handle, create a new entity */
    rv = resume_cache_create_entity(r, ctx, size);
    if (rv != OK) {
        /* Caching layer declined the opportunity to cache the response */
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, r->server,
                     "resume: No cache providers for \"%s\"", ctx->etag);
        ap_remove_output_filter(f);
        return ap_pass_brigade(f->next, in);
    }

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, r->server,
                 "resume: Saving response to: %s", r->unparsed_uri);

    apr_time_t now = apr_time_now();
    info->date = get_date(r);
    if (info->date == APR_DATE_BAD) {
        /* no date header (or bad header)! */
        info->date = now;
    }

    /* from mod_cache */
    /* set response_time for HTTP/1.1 age calculations */
    info->response_time = now;
    /* get the request time */
    info->request_time = r->request_time;

    /* be sure the cache expires _after_ any Expires header previously sent */
    info->expire = now + conf->expiration;

    /* from mod_cache */
    rv = ctx->provider->store_headers(ctx->handle, r, info);
    if(rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, r->server,
                     "resume: store_headers failed");
        ap_remove_output_filter(f);

        return ap_pass_brigade(f->next, in);
    }

    /* from mod_cache */
    rv = ctx->provider->store_body(ctx->handle, r, in);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, r->server,
                     "resume: store_body failed");
        ap_remove_output_filter(f);
    }

    if (ctx->filename && APR_BUCKET_IS_EOS(APR_BRIGADE_LAST(in))) {
        /* Delete the input cache file once it has been consumed. We can't be
         * certain this has happened until the last byte of output is
         * sent.
         */
        apr_file_close(ctx->fd);
        rv = apr_file_remove(ctx->filename, r->pool);
        if (rv == OK) {
            ctx->filename = NULL;
        }
        else {
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                         "resume: Error removing input cache file %s",
                         ctx->filename);
        }
    }

    return ap_pass_brigade(f->next, in);
}

/* from cache_storage */
int resume_cache_select(request_rec *r, resume_request_rec *ctx)
{
    char *key;
    cache_handle_t *h;
    cache_provider_list *providers;
    apr_status_t rv;

    rv = resume_generate_key(r, ctx, &key);
    if (rv != APR_SUCCESS) {
        return rv;
    }
    h = apr_palloc(r->pool, sizeof(cache_handle_t));

    for (providers = ctx->providers; providers; providers = providers->next) {
        rv = providers->provider->open_entity(h, r, key);
        switch(rv) {
            case OK: {
                if (providers->provider->recall_headers(h, r) != APR_SUCCESS) {
                    ap_log_error(APLOG_MARK, APLOG_ERR, rv, r->server,
                            "resume: recall_headers from %s failed",
                            providers->provider_name);
                    return DECLINED;
                }

                ctx->handle = h;
                ctx->provider = providers->provider;
                ctx->provider_name = providers->provider_name;
                return OK;
            }
            case DECLINED: {
                continue;
            }
            default: {
                return rv;
            }
        }
    }
    return DECLINED;
}

/* from mod_cache */
/* Deliver cached responses up the stack. */
static int resume_out_filter(ap_filter_t *f, apr_bucket_brigade *bb)
{
    resume_config *conf = ap_get_module_config(f->r->server->module_config,
                                               &resume_module);
    resume_request_rec *ctx = f->ctx;
    request_rec *r = f->r;

    if (!ctx) {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, r->server,
                     "resume: RESUME_OUT output filter enabled unexpectedly");
        ap_remove_output_filter(f);
        return ap_pass_brigade(f->next, bb);
    }
    if (!APR_BRIGADE_EMPTY(bb)) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server,
                     "resume: RESUME_OUT received unexpected input bytes");
        ap_remove_output_filter(f);
        return ap_pass_brigade(f->next, bb);
    }

    ap_log_error(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, r->server,
                 "resume: running RESUME_OUT filter");

    /* from mod_cache */
    /* restore status of cached response */
    /* This exposes a bug in mem_cache, since it does not
     * restore the status into it's handle. */
    r->status = ctx->handle->cache_obj->info.status;

    /* from mod_cache */
    /* recall_headers() was called in resume_cache_select() */
    ctx->provider->recall_body(ctx->handle, r->pool, bb);

    /* from mod_cache */
    /* This filter is done once it has served up its content */
    ap_remove_output_filter(f);
    ap_log_error(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, r->server,
                 "resume: serving %s", r->uri);
    return ap_pass_brigade(f->next, bb);
}

/*******************************************************************************
 * Configuration methods                                                       *
 ******************************************************************************/

static int resume_post_config(apr_pool_t *p, apr_pool_t *plog,
                              apr_pool_t *ptemp, server_rec *s)
{
    /* TODO(fry): initial garbage collection of file and etag
     *            also perform regular cleanup
     */
    return OK;
}

static void *create_resume_config(apr_pool_t *p, server_rec *s)
{
    resume_config *conf = apr_pcalloc(p, sizeof(resume_config));

    /* set default values */

    conf->resume_enable = apr_array_make(p, 10, sizeof(struct cache_enable));
    conf->resume_disable = apr_array_make(p, 10, sizeof(struct cache_disable));

    /* TODO(fry): is there any way for us to access cache_module? */
    /* Currently we share ResumeEnable with cache_conf for use in output
     * caching
     */
    conf->cache_conf = apr_pcalloc(p, sizeof(cache_server_conf));
    memset(conf->cache_conf, 0, sizeof(cache_server_conf));
    conf->cache_conf->cacheenable = conf->resume_enable;
    conf->cache_conf->cachedisable = conf->resume_disable;

    conf->input_cache_root = NULL;
    conf->min_checkpoint = DEFAULT_MIN_CHECKPOINT;
    conf->min_filesize = DEFAULT_MIN_CHECKPOINT;
    conf->max_filesize = DEFAULT_MAX_FILESIZE;
    conf->expiration = apr_time_from_sec(DEFAULT_EXPIRATION);
    conf->initial_mode = DEFAULT_INITIAL_MODE;
    conf->resume_mode = DEFAULT_RESUME_MODE;

    return conf;
}

/* from mod_cache */
static const char *add_resume_enable(cmd_parms *parms, void *dummy,
                                     const char *type,
                                     const char *url)
{
    resume_config *conf = ap_get_module_config(parms->server->module_config,
                                               &resume_module);
    struct cache_enable *enable;

    if (*type == '/') {
        return apr_psprintf(parms->pool,
          "provider (%s) starts with a '/'.  Are url and provider switched?",
          type);
    }

    enable = apr_array_push(conf->resume_enable);
    enable->type = type;
    if (apr_uri_parse(parms->pool, url, &(enable->url))) {
        return NULL;
    }
    if (enable->url.path) {
        enable->pathlen = strlen(enable->url.path);
    } else {
        enable->pathlen = 1;
        enable->url.path = "/";
    }
    return NULL;
}

/* from mod_cache */
static const char *add_resume_disable(cmd_parms *parms, void *dummy,
                                      const char *url)
{
    resume_config *conf = ap_get_module_config(parms->server->module_config,
                                               &resume_module);
    struct cache_disable *disable;

    disable = apr_array_push(conf->resume_disable);
    if (apr_uri_parse(parms->pool, url, &(disable->url))) {
        return NULL;
    }
    if (disable->url.path) {
        disable->pathlen = strlen(disable->url.path);
    } else {
        disable->pathlen = 1;
        disable->url.path = "/";
    }
    return NULL;
}

static const char *set_input_cache_root(cmd_parms *parms, void *in_struct_ptr,
                                        const char *root)
{
    resume_config *c = ap_get_module_config(parms->server->module_config,
                                             &resume_module);
    c->input_cache_root = root;
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
    else if (!strcasecmp(str, "HOLD_AND_FORWARD")) {
        *mode = HOLD_AND_FORWARD;
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
        return "Invalid ResumeStreamingMode; must be one of: "
            "STREAM, HOLD_AND_FORWARD";
    }
    if (!get_mode(resume, &conf->resume_mode)) {
        return "Invalid ResumeStreamingMode; must be one of: "
            "STREAM, HOLD_AND_FORWARD";
    }

    return NULL;
}


static const command_rec resume_cmds[] =
{
    AP_INIT_TAKE2("ResumeEnable", add_resume_enable, NULL, RSRC_CONF,
                  "A cache type and partial URL prefix below which "
                  "resumability is enabled"),
    AP_INIT_TAKE1("ResumeDisable", add_resume_disable, NULL, RSRC_CONF,
                  "A partial URL prefix below which resumability is disabled"),
    AP_INIT_TAKE1("ResumeInputCacheRoot", set_input_cache_root, NULL, RSRC_CONF,
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
                   "STREAM, HOLD_AND_FORWARD. The default values are STREAM "
                   "for initial requests and HOLD_AND_FORWARD for resume "
                   "requests."),
    {NULL}
};

static void resume_register_hooks(apr_pool_t *p)
{
    /* from mod_cache */

    ap_hook_handler(resume_handler, NULL, NULL, APR_HOOK_FIRST);

    /*
     * Note that input filters are pull-based, and output filters are push-based.
     *
     * Handler <- InputFilter0  <- InputFilter1  <- Network
     * Handler -> OutputFilter0 -> OutputFilter1 -> Network
     *
     * However they are grouped in priority based on their proximity to the
     * handler, as defined in util_filter.h:
     *
     * Handler <-> AP_FTYPE_RESOURCE (10)   <-> AP_FTYPE_CONTENT_SET (20)
     *         <-> AP_FTYPE_PROTOCOL (30)   <-> AP_FTYPE_TRANSCODE (40)
     *         <-> AP_FTYPE_CONNECTION (50) <-> AP_FTYPE_NETWORK (60)
     */

    /* RESUME must go in the filter chain before a possible DEFLATE filter in
     * order in order to make resumes independent of compression. In other
     * words, we want our input filter to see bytes that have already been
     * inflated if mod_deflate's input filter is in the chain.
     * Decrementing filter type by 1 ensures this happens.
     */
    resume_input_filter_handle =
            ap_register_input_filter("RESUME", resume_input_filter, NULL,
                                     AP_FTYPE_CONTENT_SET-1);

    /* Because we re-use mod_cache's providers, we inherit their filter
     * constraints. Specifically, RESUME_SAVE must go into the filter chain
     * after a possible DEFLATE filter to ensure that already compressed cache
     * objects do not get compressed again. In other words, we want our save
     * output filter to save bytes that have already run through mod_deflate's
     * output filter if it is in the chain. Incrementing filter type by 1
     * ensures this happens.
     */
    resume_save_filter_handle =
            ap_register_output_filter("RESUME_SAVE", resume_save_filter, NULL,
                                      AP_FTYPE_CONTENT_SET+1);

    /* RESUME_OUT must be at the same priority in the filter chain as
     * RESUME_SAVE in order to ensure that the replayed output is the identical
     * to the original output.
     */
    resume_out_filter_handle =
            ap_register_output_filter("RESUME_OUT", resume_out_filter, NULL,
                                      AP_FTYPE_CONTENT_SET+1);

    ap_hook_post_config(resume_post_config, NULL, NULL, APR_HOOK_MIDDLE);
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
