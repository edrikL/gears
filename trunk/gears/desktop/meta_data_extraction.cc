// Copyright 2009, Google Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "gears/desktop/drag_and_drop_utils_common.h"

#if GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM

#include "gears/desktop/meta_data_extraction.h"

#include "gears/base/common/js_types.h"
#include "gears/blob/blob_interface.h"
#include "third_party/libpng/png.h"

extern "C" {
#include "third_party/libjpeg/jpeglib.h"
}


// Make an arbitrary limit (of 2^16 pixels wide, or 2^16 pixels high) for what
// images we deem reasonable to deal with in Gears.
static const unsigned int kMaxReasonableImageWidthHeight = 65536;


//-----------------------------------------------------------------------------
// JPEG meta-data extraction functions


struct JpegBlobErrorMgr {
  jpeg_error_mgr parent_implementation;
  jmp_buf setjmp_buffer;
};


static void JpegBlobErrorExit(j_common_ptr c) {
  JpegBlobErrorMgr *jbem = reinterpret_cast<JpegBlobErrorMgr*>(c->err);
  longjmp(jbem->setjmp_buffer, 1);
}


static const size_t kJpegBlobReadContextBufferSize = 1024;
struct JpegBlobReadContext {
  BlobInterface *blob;
  int64 offset;
  uint8 buffer[kJpegBlobReadContextBufferSize];
};


static void JpegBlobInitSource(j_decompress_ptr jd) {
  JpegBlobReadContext *context =
      reinterpret_cast<JpegBlobReadContext*>(jd->client_data);
  jd->src->next_input_byte = context->buffer;
  jd->src->bytes_in_buffer = 0;
}


static boolean JpegBlobFillInputBuffer(j_decompress_ptr jd) {
  JpegBlobReadContext *context =
      reinterpret_cast<JpegBlobReadContext*>(jd->client_data);
  int bytes_read = static_cast<int>(context->blob->Read(
      context->buffer, context->offset, kJpegBlobReadContextBufferSize));
  if (bytes_read <= 0) {
    return FALSE;
  }
  assert(bytes_read >= 0 &&
         bytes_read <= static_cast<int>(kJpegBlobReadContextBufferSize));
  context->offset += bytes_read;
  jd->src->next_input_byte = context->buffer;
  jd->src->bytes_in_buffer = bytes_read;
  return TRUE;
}


static void JpegBlobSkipInputData(j_decompress_ptr jd, long num_bytes) {
  if (num_bytes <= 0) {
    return;
  }
  JpegBlobReadContext *context =
      reinterpret_cast<JpegBlobReadContext*>(jd->client_data);
  assert(jd->src->bytes_in_buffer >= 0 &&
         jd->src->bytes_in_buffer <= kJpegBlobReadContextBufferSize);
  int extra_bytes_needed =
      num_bytes - static_cast<int>(jd->src->bytes_in_buffer);
  if (extra_bytes_needed < 0) {
    jd->src->next_input_byte += num_bytes;
    jd->src->bytes_in_buffer -= num_bytes;
  } else {
    context->offset += extra_bytes_needed;
    jd->src->next_input_byte = context->buffer;
    jd->src->bytes_in_buffer = 0;
  }
}


static void JpegBlobTermSource(j_decompress_ptr jd) {
  // No-op.
}


static bool ExtractMetaDataJpeg(BlobInterface *blob, JsObject *result) {
  static const int kJpegMagicHeaderLength = 2;
  if (blob->Length() < kJpegMagicHeaderLength) {
    return false;
  }
  uint8 magic_header[kJpegMagicHeaderLength];
  if (kJpegMagicHeaderLength !=
          blob->Read(magic_header, 0, kJpegMagicHeaderLength)) {
    return false;
  }
  // The JPEG Start-Of-Image (SOI) marker is 0xFFD8.
  if (magic_header[0] != 0xFF || magic_header[1] != 0xD8) {
    return false;
  }

  JpegBlobReadContext context;
  context.blob = blob;
  context.offset = 0;

  jpeg_decompress_struct jd;
  memset(&jd, 0, sizeof(jd));
  jpeg_create_decompress(&jd);
  jd.client_data = &context;

  JpegBlobErrorMgr jbem;
  memset(&jbem, 0, sizeof(jbem));
  jd.err = jpeg_std_error(&jbem.parent_implementation);
  jd.err->error_exit = JpegBlobErrorExit;
  if (setjmp(jbem.setjmp_buffer)) {
    jpeg_destroy_decompress(&jd);
    return false;
  }

  jpeg_source_mgr jsm;
  memset(&jsm, 0, sizeof(jsm));
  jd.src = &jsm;
  jsm.init_source = JpegBlobInitSource;
  jsm.fill_input_buffer = JpegBlobFillInputBuffer;
  jsm.skip_input_data = JpegBlobSkipInputData;
  jsm.resync_to_restart = jpeg_resync_to_restart;
  jsm.term_source = JpegBlobTermSource;

  jpeg_read_header(&jd, TRUE);

  if ((jd.image_width > kMaxReasonableImageWidthHeight) ||
      (jd.image_height > kMaxReasonableImageWidthHeight)) {
    jpeg_destroy_decompress(&jd);
    return false;
  }
  result->SetPropertyInt(
      std::string16(STRING16(L"imageWidth")),
      static_cast<int>(jd.image_width));
  result->SetPropertyInt(
      std::string16(STRING16(L"imageHeight")),
      static_cast<int>(jd.image_height));
  result->SetPropertyString(
      std::string16(STRING16(L"mimeType")),
      std::string16(STRING16(L"image/jpeg")));

  jpeg_destroy_decompress(&jd);
  return true;
}


//-----------------------------------------------------------------------------
// PNG meta-data extraction functions


struct PngBlobReadContext {
  BlobInterface *blob;
  int64 offset;
};


static void PngBlobReadFunction(
    png_structp png_ptr, png_bytep data, png_size_t length) {
  PngBlobReadContext *context =
      reinterpret_cast<PngBlobReadContext*>(png_get_io_ptr(png_ptr));
  int64 read = context->blob->Read(data, context->offset, length);
  if (read < 0 || length != static_cast<uint64>(read)) {
    png_error(png_ptr, "Incomplete read from Blob");
  }
  context->offset += length;
}


static bool ExtractMetaDataPng(BlobInterface *blob, JsObject *result) {
  static const int kPngMagicHeaderLength = 8;
  if (blob->Length() < kPngMagicHeaderLength) {
    return false;
  }
  uint8 magic_header[kPngMagicHeaderLength];
  if (kPngMagicHeaderLength !=
          blob->Read(magic_header, 0, kPngMagicHeaderLength)) {
    return false;
  }
  if (png_sig_cmp(magic_header, 0, kPngMagicHeaderLength) != 0) {
    return false;
  }

  png_structp png_ptr = png_create_read_struct(
      PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    return false;
  }
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return false;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return false;
  }

  PngBlobReadContext context;
  context.blob = blob;
  context.offset = kPngMagicHeaderLength;
  png_set_sig_bytes(png_ptr, kPngMagicHeaderLength);
  png_set_read_fn(png_ptr, &context, PngBlobReadFunction);
  png_read_info(png_ptr, info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type;
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
      NULL, NULL, NULL);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  if ((width > kMaxReasonableImageWidthHeight) ||
      (height > kMaxReasonableImageWidthHeight)) {
    return false;
  }
  result->SetPropertyInt(
      std::string16(STRING16(L"imageWidth")), static_cast<int>(width));
  result->SetPropertyInt(
      std::string16(STRING16(L"imageHeight")), static_cast<int>(height));
  result->SetPropertyString(
      std::string16(STRING16(L"mimeType")),
      std::string16(STRING16(L"image/png")));
  return true;
}


//-----------------------------------------------------------------------------
// Public API


void ExtractMetaData(BlobInterface *blob, JsObject *result) {
  if (ExtractMetaDataJpeg(blob, result)) return;
  if (ExtractMetaDataPng(blob, result)) return;

  // If we didn't match an explicit mime-type, above, then we fall back on
  // the default "application/octet-stream".
  result->SetPropertyString(
      std::string16(STRING16(L"mimeType")),
      std::string16(STRING16(L"application/octet-stream")));
}

#endif  // GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM
