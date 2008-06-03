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

#ifndef GEARS_OPENSOURCE_GEARS_BLOB_BLOB_BUILDER_H__
#define GEARS_OPENSOURCE_GEARS_BLOB_BLOB_BUILDER_H__

#ifdef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#else

#include <vector>
#include "gears/base/common/basictypes.h"
#include "gears/base/common/scoped_refptr.h"
#include "gears/base/common/string16.h"

class BlobInterface;

// Creates a Blob from multiple data sources.
class BlobBuilder {
 public:
  BlobBuilder();
  ~BlobBuilder();

  // Blobs are always appended to the end of the list.
  // Returns false if the blob could not be added.
  bool AddBlob(BlobInterface *blob);

  // Appends raw data.  Returns false if the data could not be added.
  bool AddData(const void *data, int64 length);

  // String data is assumed to be UTF16 and will be converted to UTF8.
  // Returns false if the data could not be added.
  bool AddString(const std::string16 &data);

  // Returns a Blob instance containing all the data added.
  // It also has the side-effect of resetting the BlobBuilder.
  void CreateBlob(scoped_refptr<BlobInterface> *blob);

 private:
  bool IncrementAndCheckSize(int64 size_increment);
  void PushDataAsBlob();

  std::vector<scoped_refptr<BlobInterface> > blob_list_;
  std::vector<uint8> data_;
  int64 length_;
  DISALLOW_EVIL_CONSTRUCTORS(BlobBuilder);
};

#endif  // not OFFICIAL_BUILD

#endif  // GEARS_OPENSOURCE_GEARS_BLOB_BLOB_BUILDER_H__
