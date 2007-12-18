// Copyright 2007, Google Inc.
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
//
// The methods of the File class implemented for use in Firefox.
// Some methods implementations are browser neutral and can be found
// in file.cc.

#include <assert.h>
class nsIFile;  // must declare this before including nsDirectoryServiceUtils.h
#include <nsDirectoryServiceUtils.h>
#include <nsDirectoryServiceDefs.h>
#include <nsIInputStream.h>
#include <nsIIOService.h>
#include <nsILocalFile.h>
#include <nsISimpleEnumerator.h>
#include <nsXPCOM.h>
#include "common/genfiles/product_constants.h"  // from OUTDIR
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/string_utils.h"
#include "gecko_internal/nsIFileProtocolHandler.h"
#include "gecko_internal/nsIFileStreams.h"
#include "gears/base/firefox/ns_file_utils.h"


static const PRBool kRecursive = PR_TRUE;
static const PRBool kNonRecursive = PR_FALSE;


bool File::CreateNewFile(const char16 *path) {
  nsDependentString filename(path);
  nsCOMPtr<nsILocalFile> file;
  nsresult nr = NS_NewLocalFile(filename, PR_FALSE, getter_AddRefs(file));
  if (NS_SUCCEEDED(nr)) {
    nr = file->Create(nsIFile::NORMAL_FILE_TYPE, 0700);

// There is a bug in OSX where creating a file with initial permissions doesn't
// stick. You need to reset it.
#ifdef OS_MACOSX
    if (NS_SUCCEEDED(nr)) {
      nr = file->SetPermissions(0700);
    }
#endif    
  }
  
  if (NS_FAILED(nr)) {
    SetLastFileError(kCreateFileFailedMessage, path, nr);
    return false;
  }
  return true;
}


bool File::Exists(const char16 *full_filepath) {
  nsDependentString filename(full_filepath);
  nsCOMPtr<nsILocalFile> file;
  nsresult nr = NS_NewLocalFile(filename, PR_FALSE, getter_AddRefs(file));
  if (NS_FAILED(nr)) return false;
  PRBool exists = PR_FALSE;
  nr = file->Exists(&exists);
  if (NS_FAILED(nr)) return false;
  if (!exists) return false;
  PRBool is_file = PR_FALSE;
  nr = file->IsFile(&is_file);
  if (NS_FAILED(nr)) return false;
  return is_file ? true : false;
}


bool File::DirectoryExists(const char16 *full_dirpath) {
  nsDependentString filename(full_dirpath);
  nsCOMPtr<nsILocalFile> file;
  nsresult nr = NS_NewLocalFile(filename, PR_FALSE, getter_AddRefs(file));
  if (NS_FAILED(nr)) return false;
  PRBool exists = PR_FALSE;
  nr = file->Exists(&exists);
  if (NS_FAILED(nr)) return false;
  if (!exists) return false;
  PRBool is_dir = PR_FALSE;
  nr = file->IsDirectory(&is_dir);
  if (NS_FAILED(nr)) return false;
  return is_dir ? true : false;
}


bool File::ReadFileToVector(const char16 *full_filepath,
                            std::vector<uint8> *data) {
  // Create a file object
  nsDependentString filename(full_filepath);
  nsCOMPtr<nsILocalFile> file_obj;
  nsresult nr = NS_NewLocalFile(filename, PR_FALSE, getter_AddRefs(file_obj));
  if (NS_FAILED(nr)) return false;

  // Get the file size
  PRInt64 file_size = 0;
  nr = file_obj->GetFileSize(&file_size);
  if (NS_FAILED(nr)) return false;
  if (file_size < 0) return false;
  // We don't support very large files yet
  // TODO(michaeln): support large files
  if (file_size > PR_UINT32_MAX) return false;
  PRUint32 data_len = static_cast<PRUint32>(file_size);

  // Get the file input stream
  nsCOMPtr<nsIInputStream> stream;
  nr = NSFileUtils::NewLocalFileInputStream(getter_AddRefs(stream),
                                            file_obj, -1, -1,
                                            nsIFileInputStream::CLOSE_ON_EOF);
  if (NS_FAILED(nr)) return false;
  if (!stream) return false;

  // Read the file data into memory
  data->resize(data_len);
  if (data->size() != data_len) return false;
  if (data_len > 0) {
    PRUint32 data_actually_read = 0;
    nr = stream->Read(reinterpret_cast<char*>(&(data->at(0))), 
                      data_len, &data_actually_read);
    if (NS_FAILED(nr)) return false;
    if (data_len != data_actually_read) return false;
  }
  return true;
}

bool File::WriteVectorToFile(const char16 *full_filepath,
                             const std::vector<uint8> *data) {
  const uint8 *first_byte = data->size() ? &(data->at(0)) : NULL;
  return WriteBytesToFile(full_filepath, first_byte, data->size());
}


bool File::WriteBytesToFile(const char16 *full_filepath, const uint8 *buf,
                            int length) {
  // Create a file object
  nsDependentString filename(full_filepath);
  nsCOMPtr<nsILocalFile> file_obj;
  nsresult nr = NS_NewLocalFile(filename, PR_FALSE, getter_AddRefs(file_obj));
  if (NS_FAILED(nr)) return false;

  PRBool exists = PR_FALSE;
  nr = file_obj->Exists(&exists);
  if (NS_FAILED(nr)) return false;
  if (!exists) return false;

  // Write the data into it using an output stream
  nsCOMPtr<nsIOutputStream> stream;
  nr = NSFileUtils::NewLocalFileOutputStream(getter_AddRefs(stream), file_obj);
  if (NS_FAILED(nr)) return false;
  if (!stream) return false;
  while (length > 0) {
    // We loop since write can consume less than we want, in theory at least
    PRUint32 written = 0;
    nr = stream->Write(reinterpret_cast<const char*>(buf), length, &written);
    if (NS_FAILED(nr)) return false;
    if (written <= 0) return false;
    if (written > static_cast<uint>(length)) return false;
    length -= written;
    buf += written;
  }
  return true;
}


bool File::Delete(const char16 *full_filepath) {
  // Create a file object
  nsDependentString filename(full_filepath);
  nsCOMPtr<nsILocalFile> file_obj;
  nsresult nr = NS_NewLocalFile(filename, PR_FALSE, getter_AddRefs(file_obj));
  if (NS_FAILED(nr)) return false;

  // Delete the underlying file
  nr = file_obj->Remove(kNonRecursive);
  if (NS_FAILED(nr)) return false;
  return true;
}


int File::GetDirectoryFileCount(const char16 *full_dirpath) {
  // Create a file object
  nsDependentString filename(full_dirpath);
  nsCOMPtr<nsILocalFile> dir_obj;
  nsresult nr = NS_NewLocalFile(filename, PR_FALSE, getter_AddRefs(dir_obj));
  if (NS_FAILED(nr)) return 0;

  // Enumerate the contents of the directory
  nsCOMPtr<nsISimpleEnumerator> entries;
  nr = dir_obj->GetDirectoryEntries(getter_AddRefs(entries));
  if (NS_FAILED(nr)) return 0;  // expected if the directory does not exist

  int count = 0;
  PRBool more = PR_FALSE;
  while ((entries->HasMoreElements(&more) == NS_OK) && more) {
    nsCOMPtr<nsISupports> isupports;
    nr = entries->GetNext(getter_AddRefs(isupports));
    if (NS_FAILED(nr)) return 0;
    ++count;
  }
  return count;
}


static bool CreateNewTempObject(std::string16 *path, PRUint32 type) {
  static const char16 *kTempFilePrefix = STRING16(PRODUCT_SHORT_NAME L"Temp");

  // Get the system temporary directory.
  nsresult nr;
  nsCOMPtr<nsIFile> temp_dir;
  nr = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(temp_dir));
  if (NS_FAILED(nr)) {
    return false;
  }

  // Create a unique file in that directory.
  // Note: CreateUnique may adjust the filename.
  nsCOMPtr<nsIFile> temp_file(temp_dir);
  nsString ns_filename_prefix(kTempFilePrefix);
  nr = temp_file->Append(ns_filename_prefix);
  if (NS_FAILED(nr)) {
    return false;
  }

  nr = temp_file->CreateUnique(type, 0700);
  if (NS_FAILED(nr)) {
    return false;
  }

  nsString ns_path;
  temp_file->GetPath(ns_path);
  (*path) = ns_path.get();
  return true;
}


bool File::CreateNewTempFile(std::string16 *full_filepath) {
  return CreateNewTempObject(full_filepath, nsIFile::NORMAL_FILE_TYPE);
}


bool File::CreateNewTempDirectory(std::string16 *full_filepath) {
  return CreateNewTempObject(full_filepath, nsIFile::DIRECTORY_TYPE);
}


bool File::RecursivelyCreateDir(const char16 *full_dirpath) {
  nsCOMPtr<nsILocalFile> directory_obj;
  nsDependentString ns_directory(full_dirpath);
  nsresult nr = NS_NewLocalFile(ns_directory, PR_FALSE,
                                getter_AddRefs(directory_obj));
  if (NS_FAILED(nr)) {
    return false;
  }

  nr = directory_obj->Create(nsIFile::DIRECTORY_TYPE, 0700);
  if (NS_FAILED(nr) && nr != NS_ERROR_FILE_ALREADY_EXISTS) {
    return false;
  }

  return true;
}


bool File::DeleteRecursively(const char16 *full_dirpath) {
  nsCOMPtr<nsILocalFile> directory_obj;
  nsDependentString ns_directory(full_dirpath);
  nsresult nr = NS_NewLocalFile(ns_directory, PR_FALSE,
                                getter_AddRefs(directory_obj));
  if (NS_FAILED(nr)) {
    return false;
  }
  return NS_SUCCEEDED(directory_obj->Remove(kRecursive));
}


bool File::MoveDirectory(const char16 *src_path, const char16 *dest_path) {
  nsCOMPtr<nsILocalFile> src_dir;
  nsCOMPtr<nsILocalFile> dest_dir;
  nsCOMPtr<nsIFile> dest_parent;
  nsresult nr;

  nr = NS_NewLocalFile(nsDependentString(src_path), PR_FALSE,
                       getter_AddRefs(src_dir));
  if (NS_FAILED(nr)) return false;
  nr = NS_NewLocalFile(nsDependentString(dest_path), PR_FALSE,
                       getter_AddRefs(dest_dir));
  if (NS_FAILED(nr)) return false;
  nr = dest_dir->GetParent(getter_AddRefs(dest_parent));
  if (NS_FAILED(nr)) return false;
  
  nsString dest_name;
  nr = dest_dir->GetLeafName(dest_name);
  if (NS_FAILED(nr)) return false;

  return NS_SUCCEEDED(src_dir->MoveTo(dest_parent, dest_name));
}
