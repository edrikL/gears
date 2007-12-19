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

struct JSContext; // must declare this before including nsIJSContextStack.h
class nsIFile; // must declare this before including nsDirectoryServiceUtils.h
#include <gecko_sdk/include/nsDirectoryServiceDefs.h>
#include <gecko_sdk/include/nsDirectoryServiceUtils.h>
#include <gecko_sdk/include/nsIDOMHTMLInputElement.h>
#include <gecko_sdk/include/nsXPCOM.h>
#include <gecko_internal/nsIDOMClassInfo.h>
#include <gecko_internal/nsIFileStreams.h>
#include <gecko_internal/nsIJSContextStack.h>
#include <gecko_internal/nsIVariant.h>

#include "gears/localserver/firefox/file_submitter_ff.h"

#include "gears/base/common/file.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/base/firefox/ns_file_utils.h"

static const char16 *kMissingFilename = STRING16(L"Unknown");


// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_THREADSAFE_ADDREF(GearsFileSubmitter)
NS_IMPL_THREADSAFE_RELEASE(GearsFileSubmitter)
NS_INTERFACE_MAP_BEGIN(GearsFileSubmitter)
  //NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface) ModuleImplBaseClass not reqd
  NS_INTERFACE_MAP_ENTRY(GearsFileSubmitterInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsFileSubmitterInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsFileSubmitter)
NS_INTERFACE_MAP_END


// Object identifiers
const char *kGearsFileSubmitterClassName = "GearsFileSubmitter";
const nsCID kGearsFileSubmitterClassId = {0x529f7a0e, 0x80fc, 0x4548, {0xaf, 0xba,
                                          0x36, 0x4, 0x82, 0xba, 0xc7, 0x28}};
                                         // {529F7A0E-80FC-4548-AFBA-360482BAC728}


//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
GearsFileSubmitter::~GearsFileSubmitter() {
  // Delete our temp directory and all it contains
  if (temp_directory_) {
    temp_directory_->Remove(PR_TRUE); // PR_TRUE = recursive
    //temp_directory_ = NULL; // should do this, but can skip in dtor
  }
}

//------------------------------------------------------------------------------
// SetFileInputElement
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsFileSubmitter::SetFileInputElement(
                                      nsISupports *file_input_element,
                                      const nsAString &captured_url_key) {
  nsresult nr = NS_OK;

  // Verify the input element is actually what we're expecting.
  // <input type=file>
  nsCOMPtr<nsIDOMHTMLInputElement> input;
  nr = DOMUtils::VerifyAndGetFileInputElement(file_input_element,
                                              getter_AddRefs(input));
  NS_ENSURE_SUCCESS(nr, nr);

  // An empty url means clear the file input value
  if (captured_url_key.IsEmpty()) {
    nsString empty;
    return input->SetValue(empty);
  }

  std::string16 full_url;
  nsString url_concrete(captured_url_key); // nsAString doesn't have get()
  if (!ResolveAndNormalize(EnvPageLocationUrl().c_str(), url_concrete.get(),
                           &full_url)) {
    RETURN_EXCEPTION(STRING16(L"Failed to resolve url."));
  }
  if (!EnvPageSecurityOrigin().IsSameOriginAsUrl(full_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Url is not from the same origin"));
  }

  // Read data from our local store
  ResourceStore::Item item;
  if (!store_.GetItem(full_url.c_str(), &item)) {
    RETURN_EXCEPTION(STRING16(L"Failed to GetItem."));
  }

  std::string16 filename_str;
  item.payload.GetHeader(HttpConstants::kXCapturedFilenameHeader,
                         &filename_str);
  if (filename_str.empty()) {
    // This handles the case where the URL didn't get into the database
    // using the captureFile API. It's arguable whether we should support this.
    filename_str = kMissingFilename;
  }

  // Create a temp file
  nsString filename(filename_str.c_str());
  nsCOMPtr<nsIFile> temp_file;
  nr = CreateTempFile(filename, getter_AddRefs(temp_file));
  NS_ENSURE_SUCCESS(nr, nr);

  // Write the data for this captured local file into the temp file
  nsCOMPtr<nsIOutputStream> out;
  nr = NSFileUtils::NewLocalFileOutputStream(getter_AddRefs(out), temp_file);
  NS_ENSURE_SUCCESS(nr, nr);
  NS_ENSURE_TRUE(out, NS_ERROR_FAILURE);
  item.payload.data->size();
  PRUint32 length = item.payload.data->size();
  const uint8 *buf = &(item.payload.data->at(0));
  while (length > 0) {
    PRUint32 written = 0;
    nr = out->Write(reinterpret_cast<const char*>(buf), length, &written);
    NS_ENSURE_SUCCESS(nr, nr);
    NS_ENSURE_TRUE(written > 0, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(written <= length, NS_ERROR_FAILURE);
    length -= written;
    buf += written;
  }
  out->Close();
  out = nsnull;

  // Set the filepath of the input element to the temp file we just created
  nsString filepath;
  nr = temp_file->GetPath(filepath);
  NS_ENSURE_SUCCESS(nr, nr);

  // Security rights are gleaned from the JavaScript context.
  // "UniversalFileRead" is required to set the value we want to set.
  // Temporarily pushing NULL onto the JavaScript context stack lets
  // the system know that native code is running and all rights are granted.
  //
  // Because this code relies on XPConnect (which doesn't exist in worker
  // threads), this code should generally be avoided.  That's why we don't
  // expose it as a shared 'TempJsContext' object in /base.

  nsCOMPtr<nsIJSContextStack> stack =
      do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  if (!stack) { RETURN_EXCEPTION(STRING16(L"failed to get context stack.")); }
  stack->Push(NULL);

  nr = input->SetValue(filepath);
  // always pop the context first, since NS_ENSURE_SUCCESS might return
  JSContext *context;
  stack->Pop(&context);

  NS_ENSURE_SUCCESS(nr, nr);

  return NS_OK;
}


//------------------------------------------------------------------------------
// CreateTempFile
// The server will see the local filename, so we try to use the original
// filename, using a temp directory per class instance to avoid name
// conflicts.
//------------------------------------------------------------------------------
nsresult GearsFileSubmitter::CreateTempFile(const nsAString &desired_name,
                                            nsIFile **file) {
  NS_PRECONDITION(nsnull != file, "file argument is NULL");
  nsresult nr;

  // Create a temporary directory if we haven't already
  if (!temp_directory_) {
    std::string16 temp_directory_name;
    if (!File::CreateNewTempDirectory(&temp_directory_name)) {
      return NS_ERROR_FAILURE;
    }

    // open the created directory using the Firefox interfaces
    nsString ns_directory_name(temp_directory_name.c_str());
    nr = NS_NewLocalFile(ns_directory_name, PR_FALSE,
                         getter_AddRefs(temp_directory_));
    NS_ENSURE_SUCCESS(nr, nr);
  }

  // Create a new object for a nested temporary file
  // (Is there a better way to setup a nsILocalFile? I wish we could
  // just use InitWithFile.)
  nsString tmpdir;
  nr = temp_directory_->GetPath(tmpdir);
  NS_ENSURE_SUCCESS(nr, nr);

  nsCOMPtr<nsILocalFile> tmpfile;
  nr = NS_NewLocalFile(tmpdir, PR_FALSE, getter_AddRefs(tmpfile));
  NS_ENSURE_SUCCESS(nr, nr);
  NS_POSTCONDITION(nsnull != tmpfile,
                   "NS_NewLocalFile succeeded but tmpfile is invalid");

  // Supply our desired filename
  nr = tmpfile->Append(desired_name);
  NS_ENSURE_SUCCESS(nr, nr);

  // Create the temporary file. This may adjust the filename.
  nr = tmpfile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0700);
  NS_ENSURE_SUCCESS(nr, nr);

  *file = tmpfile.get();
  NS_ADDREF(*file);
  return NS_OK;
}
