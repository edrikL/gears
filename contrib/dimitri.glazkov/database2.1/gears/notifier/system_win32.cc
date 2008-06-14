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
  // The notification API has not been finalized for official builds.
#else
#if WIN32

#include <string>
#include <assert.h>
#include <windows.h>
#include "gears/notifier/system.h"
#include "gears/base/common/basictypes.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "third_party/glint/include/rectangle.h"

// Get the current module path. This is the path where the module of the
// currently running code sits.
static std::string16 GetCurrentModuleFilename() {
  // Get the handle to the module containing the given executing address
  MEMORY_BASIC_INFORMATION mbi = {0};
  DWORD result = ::VirtualQuery(&GetCurrentModuleFilename, &mbi, sizeof(mbi));
  assert(result == sizeof(mbi));
  HMODULE module_handle = reinterpret_cast<HMODULE>(mbi.AllocationBase);

  // Get the path of the loaded module
  wchar_t buffer[MAX_PATH + 1] = {0};
  ::GetModuleFileName(module_handle, buffer, ARRAYSIZE(buffer));
  return std::string16(buffer);
}

std::string System::GetResourcePath() {
  std::string16 path = GetCurrentModuleFilename();
  size_t idx = path.rfind(L'\\');
  if (idx != std::string16::npos) {
    path = path.erase(idx);
  }
  path += L"\\";

  std::string result;
  String16ToUTF8(path.c_str(), &result);
  return result;
}

void System::GetMainScreenBounds(glint::Rectangle *bounds) {
  assert(bounds);
  RECT work_area = {0};
  if (::SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0)) {
    bounds->Set(work_area.left,
                work_area.top,
                work_area.right,
                work_area.bottom);
  } else {
    // If failed to call SystemParametersInfo for some reason, we simply get the
    // full screen size as an alternative.
    bounds->Set(0,
                0,
                ::GetSystemMetrics(SM_CXSCREEN) - 1,
                ::GetSystemMetrics(SM_CYSCREEN) - 1);
  }
}

#endif  // WIN32
#endif  // OFFICIAL_BUILD

