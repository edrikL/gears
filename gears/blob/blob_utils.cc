// Copyright 2008, Google Inc.
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

#include "gears/blob/blob_utils.h"

#include "gears/base/common/basictypes.h"
#include "gears/base/common/string_utils.h"
#include "gears/blob/blob_interface.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

bool BlobToString16(BlobInterface* blob, const std::string16 &charset,
                    std::string16 *text) {
  assert(blob);
  int64 blob_length(blob->Length());
  assert(blob_length >= 0);
  assert(blob_length <= static_cast<int64>(kuint32max));
  if (blob_length == 0) {
    text->clear();
    return true;
  }
  // TODO(bgarcia): Rewrite this to remove the copy once blobs provide
  // an interface to directly access their internal memory.
  scoped_ptr_malloc<char> utf8_text;
  utf8_text.reset(static_cast<char*>(malloc(static_cast<int>(blob_length))));
  int64 length = blob->Read(reinterpret_cast<uint8*>(utf8_text.get()), 0,
                            blob_length);
  assert(length == blob_length);

#if 0
  if (charset && !charset.empty()) {
    // TODO(michaeln): decode charset using nsICharsetConverterManager
    // for firefox and MLang for ie.

    // TODO(bgarcia): Once this is fixed for firefox & ie, safari can be fixed
    // by returning:
    // ConvertToString16FromCharset(utf8_text.get(), length, charset, text);

    // TODO(bgarcia): To fix firefox & ie, also need to update
    // localserver/<platform>/http_request_XX.cc, GetResponseCharset().
  }
#endif
  // If no charset is provided, assume UTF8.
  return UTF8ToString16(utf8_text.get(), static_cast<int>(length), text);
}

bool BlobToString(BlobInterface *blob, std::string *string_out) {
  assert(blob);
  int64 blob_length(blob->Length());
  assert(blob_length >= 0);
  assert(string_out);
  assert(blob_length <= static_cast<int64>(string_out->max_size()));
  if (blob_length == 0) {
    string_out->clear();
    return true;
  }
  // TODO(bgarcia): Rewrite this to remove the copy once blobs provide
  // an interface to directly access their internal memory.
  scoped_ptr_malloc<char> utf8_text;
  utf8_text.reset(static_cast<char*>(malloc(static_cast<int>(blob_length))));
#ifdef DEBUG
  int64 length = blob->Read(reinterpret_cast<uint8*>(utf8_text.get()), 0,
                            blob_length);
  assert(length == blob_length);
#else
  blob->Read(reinterpret_cast<uint8*>(utf8_text.get()), 0, blob_length);
#endif
  *string_out = utf8_text.get();
  return true;
}

// Convert the blob's contents to a vector.
bool BlobToVector(BlobInterface *blob, std::vector<uint8>* vector_out) {
  assert(blob);
  int64 blob_length(blob->Length());
  assert(blob_length >= 0);
  assert(vector_out);
  assert(blob_length <= static_cast<int64>(vector_out->max_size()));
  if (blob_length == 0) {
    vector_out->clear();
    return true;
  }

  vector_out->resize(static_cast<std::vector<uint8>::size_type>(
      blob_length));
  int64 length = blob->Read(&((*vector_out)[0]), 0, blob_length);

  return (length == blob_length);
}
