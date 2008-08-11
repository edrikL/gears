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

#include "gears/notifier/system.h"

#include <string>
#include <assert.h>
#include <atlbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <windows.h>

#include "gears/base/common/basictypes.h"
#include "gears/base/common/common.h"
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
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

bool GetSpecialFolder(DWORD csidl, std::string16 *folder_path) {
  assert(folder_path);

  folder_path->clear();

  // Get the path of a special folder as an ITEMIDLIST structure.
  ITEMIDLIST *folder_location = NULL;
  if (FAILED(::SHGetFolderLocation(NULL, csidl, NULL, 0, &folder_location))) {
    return false;
  }

  // Get an interface to the desktop folder.
  CComPtr<IShellFolder> desktop_folder;
  if (SUCCEEDED(::SHGetDesktopFolder(&desktop_folder))) {
    assert(desktop_folder);

    // Ask the desktop for the display name of the special folder.
    STRRET str_ret = {0};
    str_ret.uType = STRRET_WSTR;
    if (SUCCEEDED(desktop_folder->GetDisplayNameOf(folder_location,
                                                   SHGDN_FORPARSING,
                                                   &str_ret))) {
      wchar_t *folder_name = NULL;
      if (SUCCEEDED(::StrRetToStr(&str_ret, folder_location, &folder_name))) {
        *folder_path = folder_name;
      }
      ::CoTaskMemFree(folder_name);
    }
  }

  ::CoTaskMemFree(folder_location);

  return !folder_path->empty();
}

bool System::GetUserDataLocation(std::string16 *path, bool create_if_missing) {
  assert(path);

  // Get the path to the special folder holding application-specific data.
  int folder_id = CSIDL_APPDATA;
  if (create_if_missing) {
    folder_id |= CSIDL_FLAG_CREATE;
  }
  if (!GetSpecialFolder(folder_id, path)) {
    return false;
  }

  // Add the directory for our product. If the directory does not exist, create
  // it.
  *path += kPathSeparator;
  *path += STRING16(PRODUCT_SHORT_NAME);

  if (create_if_missing && !File::DirectoryExists(path->c_str())) {
    if (!File::RecursivelyCreateDir(path->c_str())) {
      return false;
    }
  }

  return true;
}

void System::GetMainScreenWorkArea(glint::Rectangle *bounds) {
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

// Used in System::GetSystemFontScaleFactor.
struct FontScaleMapping {
  const wchar_t *size_name;
  double scale_factor;
};

// Get the scale factor of current system font to normal system font
// (i.e. DPI has been changed)
double System::GetSystemFontScaleFactor() {
  double factor = 1.0;

  // Count in the scaling due to DPI change
  const double kDefaultDPI = 96.0;
  HDC hdc = ::GetDC(NULL);
  factor *= ::GetDeviceCaps(hdc, LOGPIXELSY) / kDefaultDPI;
  ::ReleaseDC(NULL, hdc);

  const wchar_t kKeyName[] =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager";
  HKEY key;
  if (ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_CURRENT_USER,
                                      kKeyName,
                                      0,
                                      KEY_READ,
                                      &key)) {
    return factor;
  }

  // Count in the scaling due to font size change. Windows stores the
  // current 'font size name' in registry and AFAIK there can be 3 string
  // values for which we should check.
  const int kBufferSizeBytes = 64;
  BYTE buffer[kBufferSizeBytes];
  DWORD buffer_actual_bytes = kBufferSizeBytes;
  if (ERROR_SUCCESS == ::RegQueryValueEx(key,
                                         L"SizeName",
                                         NULL,
                                         NULL,
                                         buffer,
                                         &buffer_actual_bytes)) {
    const FontScaleMapping mappings[] = {
      { L"NormalSize",      1.0 },
      { L"LargeFonts",      13.0 / 11.0 },
      { L"ExtraLargeFonts", 16.0 / 11.0 },
    };
    wchar_t *font_size_name = reinterpret_cast<wchar_t*>(buffer);
    int font_size_name_length = buffer_actual_bytes / sizeof(font_size_name[0]);

    // RegQueryValueEx does not guarantee terminating \0. So check and if there
    // is one, don't include it in wstring.
    if (font_size_name_length >= 1 &&
        font_size_name[font_size_name_length - 1] == 0) {
      --font_size_name_length;
    }

    std::wstring size_name(font_size_name, font_size_name_length);
    for (size_t i = 0; i < ARRAYSIZE(mappings); ++i) {
      if (size_name == mappings[i].size_name) {
        factor *= mappings[i].scale_factor;
        break;
      }
    }
  }

  return factor;
}

#endif  // WIN32
#endif  // OFFICIAL_BUILD
