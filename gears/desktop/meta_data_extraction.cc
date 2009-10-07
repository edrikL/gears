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


struct MarkerReader {
  uint8 *data;
  int offset;
  int length;
  bool bigEndian;

  MarkerReader(jpeg_marker_struct *jm);
  uint8 NextU8();
  uint16 NextU16();
  uint32 NextU32();
};


MarkerReader::MarkerReader(jpeg_marker_struct *jm) {
  data = jm->data;
  offset = 0;
  length = jm->data_length;
  bigEndian = false;
}


uint8 MarkerReader::NextU8() {
  if (offset >= length) {
    return 0;
  }
  return data[offset++];
}


uint16 MarkerReader::NextU16() {
  uint8 a = NextU8();
  uint8 b = NextU8();
  return bigEndian ? ((a<<8) | b) : ((b<<8) | a);
}


uint32 MarkerReader::NextU32() {
  uint8 a = NextU8();
  uint8 b = NextU8();
  uint8 c = NextU8();
  uint8 d = NextU8();
  return bigEndian ? ((a<<24) | (b<<16) | (c<<8) | d)
      : ((d<<24) | (c<<16) | (b<<8) | a);
}


// Returns 0 if it fails to find an Exif Orientation tag.
static int ParseExifOrientation(jpeg_marker_struct *jm) {
  MarkerReader r(jm);
  // Read "Exif\0\0".
  if (r.NextU8() != 0x45 || r.NextU8() != 0x78 ||
      r.NextU8() != 0x69 || r.NextU8() != 0x66 ||
      r.NextU8() != 0x00 || r.NextU8() != 0x00) {
    return 0;
  }
  // Read "II" (little endian) or "MM" (big endian).
  uint8 e0 = r.NextU8();
  uint8 e1 = r.NextU8();
  if (e0 == 0x49 && e1 == 0x49) {
    r.bigEndian = false;
  } else if (e0 == 0x4D && e1 == 0x4D) {
    r.bigEndian = true;
  } else {
    return 0;
  }
  // Read the TIFF header, in the previously specified endianness.
  if (r.NextU16() != 0x002A || r.NextU32() != 0x00000008) {
    return 0;
  }
  int numTags = r.NextU16();
  for (int i = 0; i < numTags; i++) {
    int tag = r.NextU16();
    int type = r.NextU16();
    int count = r.NextU32();

    // As per the Exif specification (http://www.exif.org/Exif2-2.PDF),
    // sections 4.6.2 to 4.6.4, tag 0x0112 means Orientation, type 3 means
    // 16-bit int, and count 1 means that the 16-bit int value is read
    // in-place, rather than from an offset.
    if (tag == 0x0112 && type == 0x03 && count == 0x01) {
      return r.NextU16();
    } else {
      r.offset += 4;
    }
  }
  return 0;
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
  // Exif metadata is marked with JPEG_APP1.
  jpeg_save_markers(&jd, JPEG_APP0 + 1, 0xFFFF);
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

  if (jd.marker_list != NULL) {
    // Valid Exif Orientation values are from 1 to 8 inclusive.
    int orientation = ParseExifOrientation(jd.marker_list);
    if (orientation >= 1 && orientation <= 8) {
      result->SetPropertyInt(
          std::string16(STRING16(L"exifOrientation")),
          static_cast<int>(orientation));
    }
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
