// Copyright 2006, Google Inc.
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

#ifndef GEARS_LOCALSERVER_FIREFOX_FILE_SUBMITTER_FF_H__
#define GEARS_LOCALSERVER_FIREFOX_FILE_SUBMITTER_FF_H__

#include <nsILocalFile.h>
#include "ff/genfiles/localserver.h" // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/localserver/common/resource_store.h"


// Object identifiers
extern const char *kGearsFileSubmitterClassName;
extern const nsCID kGearsFileSubmitterClassId;


//------------------------------------------------------------------------------
// GearsFileSubmitter
// Facilitates the inclusion of captured local files in form submissions by
// manipulating <input type=file> elements to refer to local files that were
// previously captured via store.CaptureFile().
//------------------------------------------------------------------------------
class GearsFileSubmitter
    : public GearsBaseClass,
      public GearsFileSubmitterInterface {
 public:
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
  // End boilerplate code. Begin interface.

  GearsFileSubmitter() {}

  NS_IMETHOD SetFileInputElement(nsISupports *file_input_element,
                                 const nsAString & captured_url_key);

 protected:
  ~GearsFileSubmitter();

 private:
  nsresult CreateTempFile(const nsAString &desired_name, nsIFile **file);

  // The store we can read files for submission from
  ResourceStore store_;

  // Subdirectory under system temp dir where we place temp files
  nsCOMPtr<nsILocalFile> temp_directory_;

  friend class GearsResourceStore;

  DISALLOW_EVIL_CONSTRUCTORS(GearsFileSubmitter);
};

#endif // GEARS_LOCALSERVER_FIREFOX_FILE_SUBMITTER_FF_H__
