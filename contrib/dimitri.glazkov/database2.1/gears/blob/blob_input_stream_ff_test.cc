// Copyright 2008 Google Inc. All Rights Reserved.
// Author: bgarcia@google.com (Brad Garcia)
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

#include "blob_input_stream_ff.h"
#include "buffer_blob.h"

#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestBlobInputStreamFf - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}

namespace {

// This counts how many times any consumer was called.
int consumer_count = 0;

// This sets the limit for ConsumerFails().
int consumer_fail = 0;

// Size of the buffer used by ReadSegments.
const int rs_buffer_size(1024 * 1024);

// Some data we can use to initialize our blobs.
std::vector<uint8>* data(0);

// Amount of data that we create (8 MB).
const std::vector<uint8>::size_type data_size(1024 * 1024 * 8);

// Given an expected number of bytes, compute the expected count.
int ExpectedCount(int bytes) {
  return ((bytes - 1) / rs_buffer_size) + 1;
}

// This consumer accepts all input.
NS_IMETHODIMP ConsumerIdeal(nsIInputStream *aInStream,
                            void *aClosure,
                            const char *aFromSegment,
                            PRUint32 aToOffset,
                            PRUint32 aCount,
                            PRUint32 *aWriteCount) {
  ++consumer_count;
  *aWriteCount = aCount;
  return NS_OK;
}

// This consumer accepts all input until it is called for the Nth time.
NS_IMETHODIMP ConsumerFails(nsIInputStream *aInStream,
                            void *aClosure,
                            const char *aFromSegment,
                            PRUint32 aToOffset,
                            PRUint32 aCount,
                            PRUint32 *aWriteCount) {
  ++consumer_count;
  if (consumer_count >= consumer_fail) {
    return NS_ERROR_FAILURE;
  }
  *aWriteCount = aCount;
  return NS_OK;
}

// This consumer sets aWriteCount to 0 and returns NS_OK
// NOTE - This behavior is documented as having undefined meaning.
//        Our implementation of ReadSegments should assert.
//        So this cannot be used in a default build of the tests.
NS_IMETHODIMP ConsumerReadsZero(nsIInputStream *aInStream,
                                void *aClosure,
                                const char *aFromSegment,
                                PRUint32 aToOffset,
                                PRUint32 aCount,
                                PRUint32 *aWriteCount) {
  ++consumer_count;
  *aWriteCount = 0;
  return NS_OK;
}

// Describes the type of writer that should be passed into ReadSegments().
enum Writer {
  SUCCEED,
  FAIL1 = 1,
  FAIL2 = 2,
  FAIL3 = 3,
  FAIL4 = 4,
  FAIL5 = 5,
  FAIL6 = 6,
  FAIL7 = 7,
  FAIL8 = 8,
  READ_ZERO,
};

// This function creates a BlobInputStream filled with data_size bytes of data.
// It then calls ReadSegments() on that stream, passing in various writers
// to confirm correct behavior of the stream's ReadSegments function.
bool TestBlobInputStreamFfReadSegments(Writer type,
                                       BlobInterface* blob,
                                       PRUint32 in_bytes) {
  // Initialize all of these variables because some compilers complain that they
  // "may be used uninitialized in this function".
  nsWriteSegmentFun writer(NULL);
  PRUint32 expected_bytes(0);
  nsresult expected_result(NS_OK);
  int expected_count(0);

  switch (type) {
    case SUCCEED:
      writer = &ConsumerIdeal;
      expected_bytes = (in_bytes > data_size) ? data_size : in_bytes;
      expected_count = ExpectedCount(expected_bytes);
      expected_result = NS_OK;
      break;
    case FAIL1:
    case FAIL2:
    case FAIL3:
    case FAIL4:
    case FAIL5:
    case FAIL6:
    case FAIL7:
    case FAIL8:
      writer = &ConsumerFails;
      expected_count = consumer_fail = static_cast<int>(type);
      expected_bytes = rs_buffer_size * (expected_count - 1);
      expected_result = NS_OK;
      // There's no reason to set the failure count too high for the
      // number of bytes we have to read.  Just use SUCCEED instead.
      assert(expected_bytes < in_bytes);
      break;
    case READ_ZERO:
      writer = &ConsumerReadsZero;
      expected_count = -1;
      expected_bytes = 1;
      expected_result = NS_OK;
      break;
  }

  consumer_count = 0;
  PRUint32 out_bytes;
  nsCOMPtr<BlobInputStream> bs(new BlobInputStream(blob));
  nsresult result = bs->ReadSegments(writer, 0, in_bytes, &out_bytes);
  TEST_ASSERT(result == expected_result);
  TEST_ASSERT(out_bytes == expected_bytes);
  TEST_ASSERT(consumer_count == expected_count);
  return true;
}

}  // namespace

bool TestBlobInputStreamFf(std::string16 *error) {
  std::vector<uint8>* data(new std::vector<uint8>(data_size, 79));
  scoped_refptr<BlobInterface> blob(new BufferBlob(data));
  bool ok = true;

  // Test a writer that always succeeds.
  // Try to read all available data.
  ok &= TestBlobInputStreamFfReadSegments(SUCCEED, blob.get(), data_size);
  // Try to read 1KB of data (less than a full buffer).
  ok &= TestBlobInputStreamFfReadSegments(SUCCEED, blob.get(), 1024);
  // Try to read 1MB of data (exactly a full buffer).
  ok &= TestBlobInputStreamFfReadSegments(SUCCEED, blob.get(), 1024 * 1024);
  // Try to read 1MB + 1 byte of data (just over 1 full buffer).
  ok &= TestBlobInputStreamFfReadSegments(SUCCEED, blob.get(), 1024 * 1024 + 1);
  // Try to read 2MB of data (multiple full buffers).
  ok &= TestBlobInputStreamFfReadSegments(SUCCEED, blob.get(), 1024 * 1024 * 2);
  // Try to read 2MB + 1 byte of data (just over 2 full buffers).
  ok &= TestBlobInputStreamFfReadSegments(SUCCEED, blob.get(), 1024*1024*2 + 1);
  // Try to read 10MB of data (more than available).  Expect to return 8MB.
  ok &= TestBlobInputStreamFfReadSegments(SUCCEED, blob.get(), 1024*1024*10);

  // Test a writer failing on the first call.
  // Try to read all available data.
  ok &= TestBlobInputStreamFfReadSegments(FAIL1, blob.get(), data_size);
  // Try to read 1KB of data (less than a full buffer).
  ok &= TestBlobInputStreamFfReadSegments(FAIL1, blob.get(), 1024);
  // Try to read 1MB of data (exactly a full buffer).
  ok &= TestBlobInputStreamFfReadSegments(FAIL1, blob.get(), 1024 * 1024);
  // Try to read 1MB + 1 byte of data (just over 1 full buffer).
  ok &= TestBlobInputStreamFfReadSegments(FAIL1, blob.get(), 1024 * 1024 + 1);
  // Try to read 2MB of data (multiple full buffers).
  ok &= TestBlobInputStreamFfReadSegments(FAIL1, blob.get(), 1024 * 1024 * 2);
  // Try to read 2MB + 1 byte of data (just over 2 full buffers).
  ok &= TestBlobInputStreamFfReadSegments(FAIL1, blob.get(), 1024*1024*2 + 1);
  // Try to read 10MB of data (more than available).
  ok &= TestBlobInputStreamFfReadSegments(FAIL1, blob.get(), 1024 * 1024 * 10);

  // Test a writer that fails on a later call.
  // Try to read all available data, fail on 2.
  ok &= TestBlobInputStreamFfReadSegments(FAIL2, blob.get(), data_size);
  // Try to read all available data, fail on 8.
  ok &= TestBlobInputStreamFfReadSegments(FAIL8, blob.get(), data_size);
  // Try to read 1MB + 1 byte of data (just over 1 full buffer), fail on 2.
  ok &= TestBlobInputStreamFfReadSegments(FAIL2, blob.get(), 1024 * 1024 + 1);
  // Try to read 2MB of data (multiple full buffers), fail on 2.
  ok &= TestBlobInputStreamFfReadSegments(FAIL2, blob.get(), 1024 * 1024 * 2);
  // Try to read 2MB + 1 byte of data (just over 2 full buffers), fail on 2.
  ok &= TestBlobInputStreamFfReadSegments(FAIL2, blob.get(), 1024*1024*2 + 1);
  // Try to read 2MB + 1 byte of data (just over 2 full buffers), fail on 3.
  ok &= TestBlobInputStreamFfReadSegments(FAIL3, blob.get(), 1024*1024*2 + 1);

  // Test a writer that returns NS_OK and a byte count of 0.
  // NOTE - this should result in an assert, so do NOT enable this by default!
  //ok &= TestBlobInputStreamFfReadSegments(READ_ZERO, blob.get(), data_size);

  if (!ok) {
    assert(error);
    *error += STRING16(L"TestBlobInputStreamFf - failed. ");
  }
  return ok;
}
