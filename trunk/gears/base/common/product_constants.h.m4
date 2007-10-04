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

m4_changequote(`^',`^')m4_dnl
m4_changecom()m4_dnl
#ifndef GEARS_BASE_COMMON_PRODUCT_CONSTANTS_H__
#define GEARS_BASE_COMMON_PRODUCT_CONSTANTS_H__

#include "gears/base/common/string16.h"


#define PRODUCT_VERSION_STRING       L"PRODUCT_VERSION"
#define PRODUCT_VERSION_STRING_ASCII  "PRODUCT_VERSION"

#define ^PRODUCT_VERSION_MAJOR^  PRODUCT_VERSION_MAJOR
#define ^PRODUCT_VERSION_MINOR^  PRODUCT_VERSION_MINOR

#define PRODUCT_FRIENDLY_NAME       L"PRODUCT_FRIENDLY_NAME_UQ"
#define PRODUCT_FRIENDLY_NAME_ASCII  "PRODUCT_FRIENDLY_NAME_UQ"
#define PRODUCT_SHORT_NAME          L"PRODUCT_SHORT_NAME_UQ"
#define PRODUCT_SHORT_NAME_ASCII     "PRODUCT_SHORT_NAME_UQ"


// To change the version of an interface, decide whether it's an incompatible
// change (meaning you should increment the major version number and reset
// the minor to zero), or a backward-compatible change (meaning you should
// increment the minor version by one and leave the major version as-is).
//
// The rest of the codebase does not yet support multiple major versions
// of the same interface, so neither does this file.

#define DECLARE_GEARS_MODULE_VERSION_VARIABLES(MODULE) \
  extern const int kGears##MODULE##VersionMajor; \
  extern const int kGears##MODULE##VersionMinor; \
  extern const char16 *kGears##MODULE##VersionString;

// Need a wrapper so any arguments that are themselves macros get expanded.
// Otherwise we could see "1.MYVAR" instead of "1.2"
#define DEFINE_MODULE_VERSION_VARIABLES(MODULE,MAJOR,MINOR) \
  DEFINE_GEARS_VARS_CORE(MODULE,MAJOR,MINOR)
#define DEFINE_GEARS_VARS_CORE(MODULE,MAJOR,MINOR) \
  const int kGears##MODULE##VersionMajor = MAJOR; \
  const int kGears##MODULE##VersionMinor = MINOR; \
  const char16 *kGears##MODULE##VersionString = \
      STRING16(L###MAJOR L"." L###MINOR);


DECLARE_GEARS_MODULE_VERSION_VARIABLES(Channel);
DECLARE_GEARS_MODULE_VERSION_VARIABLES(Database);
DECLARE_GEARS_MODULE_VERSION_VARIABLES(HttpRequest);
DECLARE_GEARS_MODULE_VERSION_VARIABLES(LocalServer);
DECLARE_GEARS_MODULE_VERSION_VARIABLES(Timer);
DECLARE_GEARS_MODULE_VERSION_VARIABLES(WorkerPool);

#endif  // GEARS_BASE_COMMON_PRODUCT_CONSTANTS_H__
