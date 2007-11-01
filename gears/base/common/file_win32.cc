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
// The methods of the File class with a Windows-specific implementation.

// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented
#ifdef WIN32
#include <windows.h>
#include <assert.h>
#include <shlobj.h>
#include <wininet.h>
#include <shlwapi.h>
#include <tchar.h>
#include "gears/base/common/file.h"
#include "gears/base/common/int_types.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/localserver/common/http_constants.h"

static bool CreateShellLink(const char16 *object_path,
                            const char16 *link_path,
                            const char16 *arguments) {
  HRESULT result;
  IShellLink* shell_link;

  result = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            IID_IShellLink, (LPVOID*)&shell_link);
  if (SUCCEEDED(result)) {
    shell_link->SetPath(object_path);
    shell_link->SetArguments(arguments);

    IPersistFile* persist_file;
    result = shell_link->QueryInterface(IID_IPersistFile,
        reinterpret_cast<void**>(&persist_file));

    if (SUCCEEDED(result)) {
      result = persist_file->Save(link_path, TRUE);
      persist_file->Release();
    }
    shell_link->Release();
  }
  return SUCCEEDED(result);
}

static bool GetAppPath(const char16 *process_name, std::string16 *app_path) {
  bool query_result = false;
  LONG result;
  HKEY hkey;
  std::string16 key_name(STRING16(L"SOFTWARE\\Microsoft\\Windows\\"
                                  L"CurrentVersion\\App Paths\\"));
  
  key_name += process_name;

  result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name.c_str(), 0, KEY_READ,
                        &hkey);
  if (ERROR_SUCCESS == result) {
    char16 app_path_buf[MAX_PATH];
    DWORD app_path_buf_size = MAX_PATH;
    result = RegQueryValueEx(hkey, NULL, NULL, NULL,
                             reinterpret_cast<BYTE *>(app_path_buf),
                             &app_path_buf_size);
    if (ERROR_SUCCESS == result) {
      *app_path = app_path_buf;
      query_result = true;
    }
    RegCloseKey(hkey);
  }
  return query_result;
}

static bool GetDesktopPath(std::string16 *desktop_path) {
  bool succeeded = false;
  char16 path_buf[MAX_PATH];
  
  // We use the old version of this function because the new version apparently
  // won't tell you the Desktop folder path.
  BOOL result = SHGetSpecialFolderPath(NULL, path_buf,
    CSIDL_DESKTOPDIRECTORY, TRUE);
  if (result) {
    *desktop_path = path_buf;
    succeeded = true;
  }
  return succeeded;
}

bool File::CreateDesktopShortcut(BrowserType browser_type,
                                 const char16 *link_name,
                                 const char16 *launch_url,
                                 const char16 *icon_url) {
  bool creation_result = false;
  std::string16 app_path;
  const char16 *process_name;                              

  switch (browser_type) {
    case FIREFOX:
      process_name = STRING16(L"FIREFOX.EXE");
      break;
    case IE:
      process_name = STRING16(L"IEXPLORE.EXE");
      break;
    default:
      return false;
  }

  // Check input URL for safety.
  std::string16 safe_launch_url;
  if (NULL == launch_url) {
    return false;
  }
  if (!UrlIs(launch_url, URLIS_URL)) {
    return false;
  }
  char16 launch_url_buf[INTERNET_MAX_URL_LENGTH];
  DWORD launch_url_buf_size = INTERNET_MAX_URL_LENGTH;
  HRESULT hresult = UrlCanonicalize(launch_url, launch_url_buf,
                                    &launch_url_buf_size,
                                    URL_ESCAPE_PERCENT | URL_ESCAPE_UNSAFE);
  if (FAILED(hresult)) {
    return false;
  }
  safe_launch_url = launch_url_buf;
  launch_url_buf_size = INTERNET_MAX_URL_LENGTH;
  hresult = UrlGetPart(safe_launch_url.c_str(), launch_url_buf,
                       &launch_url_buf_size, URL_PART_SCHEME, 0);
  if (FAILED(hresult)) {
    return false;
  }
  int compare_result = StringCompareIgnoreCase(HttpConstants::kHttpScheme,
                                               launch_url_buf);
  if (0 != compare_result) {
    compare_result = StringCompareIgnoreCase(HttpConstants::kHttpsScheme,
                                             launch_url_buf);
    if (0 != compare_result) {
      return false;
    }
  }

  if (GetAppPath(process_name, &app_path)) {
    std::string16 link_path;

    if (GetDesktopPath(&link_path)) {
      if (link_path[link_path.size()] != L'\\') {
        link_path += STRING16(L"\\");
      }
      link_path += link_name;
      link_path += STRING16(L".lnk");
      creation_result = CreateShellLink(app_path.c_str(), link_path.c_str(),
                                        safe_launch_url.c_str());
    }
  }
  return creation_result;
}
#endif  // #ifdef WIN32
