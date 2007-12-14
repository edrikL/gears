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
//
// We follow a convention for the layout of user data files:
//   <DATA ROOT>/host/scheme_port/dataname#module
// For example:
//   <DATA ROOT>/www.google.com/http_80/foo#database
//   <DATA ROOT>/www.google.com/http_80/foo#localserver/...
//   <DATA ROOT>/www.google.com/http_80/bar#database


#ifndef GEARS_BASE_COMMON_PATHS_H__
#define GEARS_BASE_COMMON_PATHS_H__

#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"


// The path separator character for the current platform.
extern const char16 kPathSeparator;

// All data for a given security origin lives in a single directory.
// Here are the unique suffixes each module uses to identify its files/dirs.
extern const char16 *kDataSuffixForDatabase;
extern const char16 *kDataSuffixForDesktop;
extern const char16 *kDataSuffixForLocalServer;

// Longest filename we're prepared to create based on user input.
extern const size_t kUserPathComponentMaxChars;

// The maximum number of characters we allow in a file's extension *including*
// the '.' delimiting the extension.
extern const size_t kFileExtensionMaxChars;

// Hard Limit on path component length, used for sanity checking.
extern const size_t kGeneratedPathComponentMaxChars;


// Determines the current user's Scour data directory, for the given origin,
// and returns the full path in 'path'.  There is no trailing path separator.
// Does not try to create the directory.
//
// Returns true if the function succeeds.  'path' is unmodified on failure.
bool GetDataDirectory(const SecurityOrigin &origin, std::string16 *path);

// Appends a module-specific name to 'path', suitable for a data file or dir.
// There is no trailing path separator.
//
// The user-defined 'name' can be the empty string, but 'module_suffix' should
// use one of our named constants.  
bool AppendDataName(const char16 *name, const char16 *module_suffix,
                    std::string16 *path);


// Determines the base directory for Scour components files
// and returns the full path in 'path'.  There is no trailing path separator.
//
// Returns true if the function succeeds.  'path' is unmodified on failure.
bool GetBaseComponentsDirectory(std::string16 *path);

// [NOTE: Prefer GetDataDirectory() over this function.]
// Determines the base directory for all Scour user data for the current user,
// and returns the full path in 'path'.  There is no trailing path separator.
// Does not try to create the directory.
//
// Returns true if the function succeeds.  'path' is unmodified on failure.
bool GetBaseDataDirectory(std::string16 *path);

// Checks that an unsanitized string from the user is valid for use as part
// of a path component. 'error_message' is optional and can be NULL.
// 
// Returns true if the function succeeds. 'error_message' is unmodified on 
// success.
bool IsUserInputValidAsPathComponent(const std::string16 &user_input,
                                     std::string16 *error_message);


#endif // GEARS_BASE_COMMON_PATHS_H__
