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

// Generic functionality used by Desktop module.

#ifndef GEARS_DESKTOP_COMMON_DESKTOP_UTILS_H__
#define GEARS_DESKTOP_COMMON_DESKTOP_UTILS_H__

#include <vector>
#include "gears/base/common/common.h"
#include "gears/base/common/security_model.h"

class DesktopUtils {
 public:
  struct IconData {
    IconData() : width(0), height(0) {}
    int width;
    int height;
    std::string16 url;
    std::vector<uint8> png_data;
    std::vector<uint8> raw_data;
    DISALLOW_EVIL_CONSTRUCTORS(IconData);
  };

  struct ShortcutInfo {
    ShortcutInfo(){}
    std::string16 app_name;
    std::string16 app_url;
    std::string16 app_description;
    IconData icon16x16;
    IconData icon32x32;
    IconData icon48x48;
    IconData icon128x128;
    DISALLOW_EVIL_CONSTRUCTORS(ShortcutInfo);
  };

  // Creates a desktop shortcut that opens the URL in the browser it's currently
  // running in.
  //
  // origin is the security origin of the website trying to create the shortcut.
  // 
  // shortcut contains the parameters for creating the actual shortcut.
  //
  // error is filled with an error string if the function fails.
  static bool CreateDesktopShortcut(const SecurityOrigin &origin,
                                    const ShortcutInfo &shortcut,
                                    std::string16 *error);
 private:
  DesktopUtils();  // Disallow construction, only static methods.
  DISALLOW_EVIL_CONSTRUCTORS(DesktopUtils);
};

#endif  // GEARS_DESKTOP_COMMON_DESKTOP_UTILS_H__
