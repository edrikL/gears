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

#ifndef GEARS_BASE_COMMON_FACTORY_UTILS_H__
#define GEARS_BASE_COMMON_FACTORY_UTILS_H__

#include "gears/base/common/string16.h"

class SecurityOrigin;
class GearsFactory;


// Given a null-terminated string of the form "X.Y", where X and Y are
// positive decimal integers, parses X and Y and returns them as integers.
//
// On any failure, returns false and leaves *major and *minor untouched.
bool ParseMajorMinorVersion(const char16 *version, int *major, int *minor);

// Appends information about the Gears build to the string provided.
void AppendBuildInfo(std::string16 *s);

// Returns true if the calling security origin has been granted
// access by the user. Will prompt the user for permission only if
// needed. The custom_<<>> parameters are optional and may be NULL.
//
// TODO(cprince): PromptUser doesn't really need to be a member of
// GearsFactory. If we move it out of there, we can remove the parameter
// here, which makes the flow of control cleaner.
bool HasPermissionToUseGears(GearsFactory *factory,
                             const char16 *custom_icon_url,
                             const char16 *custom_name,
                             const char16 *custom_message);

// Shows a warning prompt asking a user if origin should be allowed, and
// saves the result in the PermissionsDB. Returns true if access is allowed.
// The custom_<<>> parameters are optional and may be NULL.
bool ShowPermissionsPrompt(const SecurityOrigin &origin,
                           const char16 *custom_icon_url,
                           const char16 *custom_name,
                           const char16 *custom_message);

// Sets a usage-tracking bit once per instantiation of Gears module. On
// machines that have the Google Update Service available, this bit is
// periodically reported and then reset. Currently Windows-only.
void SetActiveUserFlag();

#endif // GEARS_BASE_COMMON_FACTORY_UTILS_H__
