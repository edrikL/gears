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

#ifdef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#else

#include "gears/blob/join_blob.h"

#include <limits>

JoinBlob::JoinBlob(const List &blob_list) : length_(0) {
  List::const_iterator itr(blob_list.begin());
  List::const_iterator end(blob_list.end());
  for (; itr != end; ++itr) {
    scoped_refptr<BlobInterface> blob(*itr);
    int64 blob_length = blob->Length();
    blob_map_.insert(std::make_pair(length_, blob));
    assert(std::numeric_limits<int64>::max() - blob_length > length_);
    length_ += blob_length;
  }
}

// This implementation of Read attempts to cross member Blob boundaries
// within a single call.  But that only occurs when the member Blob returns
// all of its remaining bytes.  If it returns fewer than it has remaining,
// then this function will return at that point.
int64 JoinBlob::Read(uint8 *destination, int64 offset, int64 max_bytes) const {
  assert(offset >= 0);
  Map::const_iterator itr(blob_map_.upper_bound(offset));
  // The first entry should always have key 0, so it should never be returned
  // by upper_bound unless the map is empty.
  if (itr == blob_map_.begin()) return 0;
  --itr;
  int64 bytes_read = 0;
  while (bytes_read < max_bytes) {
    uint8 *part_destination = destination + bytes_read;
    int64 part_offset = (itr->first < offset) ? offset - itr->first : 0;
    int64 part_max_bytes = max_bytes - bytes_read;
    int64 part_read = itr->second->Read(part_destination,
                                        part_offset, part_max_bytes);
    if (part_read == -1) return -1;
    bytes_read += part_read;
    ++itr;
    if (itr == blob_map_.end()) break;
    if (itr->first != offset + bytes_read) break;
  }
  return bytes_read;
}

#endif  // not OFFICIAL_BUILD
