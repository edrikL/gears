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
// Windows-specific implementation of desktop shortcut creation.

// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented
#ifdef WIN32
#include <assert.h>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>
#include <wininet.h>

#include "common/genfiles/product_constants.h"  // from OUTDIR
#include "gears/base/common/common.h"
#include "gears/base/common/file.h"
#include "gears/base/common/int_types.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/png_utils.h"
#include "gears/base/common/scoped_win32_handles.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/desktop/desktop_utils.h"
#include "gears/localserver/common/http_constants.h"


static bool GetLongFilename(const char16 *filename, std::string16 *output) {
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = FindFirstFile(filename, &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  *output = find_data.cFileName;
  FindClose(find_handle);

  return true;
}

// Expand an 8.3 filename to its full name.  *output is set on return, so it's
// safe to pass the same std::string16 in for both parameters.
static bool GetLongPath(const char16 *path, std::string16 *output) {
  std::string16 input(path);
  std::string16 long_path;

  // Windows paths can be delimited by either type of slash.
  int last_slash = input.find_last_of('/');
  int last_backslash = input.find_last_of('\\');
  int slash_pos = last_slash > last_backslash ? last_slash : last_backslash;

  while (input != L"\\" && input != L"/" && slash_pos != std::string16::npos) {
    std::string16 current_path;
    if (!GetLongFilename(input.c_str(), &current_path)) {
      return false;
    }
    long_path = L"\\" + current_path + long_path;
    input.erase(slash_pos, input.length() - slash_pos);

    int last_slash = input.find_last_of('/');
    int last_backslash = input.find_last_of('\\');
    slash_pos = last_slash > last_backslash ? last_slash : last_backslash;
  }

  *output = input + long_path;
  return true;
}

static bool CreateShellLink(const char16 *link_path,
                            const char16 *icon_path,
                            const char16 *object_path,
                            const char16 *arguments) {
#ifdef WINCE
  // For now we simply create a shortcut in the Start Menu.
  // TODO(steveblock): Set the shortcut icon.
  // TODO(steveblock): Look at adding an entry to the home / today screen.
  return SHCreateShortcut(const_cast<char16*>(link_path),
                          const_cast<char16*>(arguments)) == TRUE;
#else
  HRESULT result;
  IShellLink* shell_link;

  result = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            IID_IShellLink, (LPVOID*)&shell_link);
  if (SUCCEEDED(result)) {
    shell_link->SetPath(object_path);
    shell_link->SetArguments(arguments);
    shell_link->SetIconLocation(icon_path, 0);

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
#endif
}

static bool ReadShellLink(const char16 *link_path,
                          std::string16 *icon_path,
                          std::string16 *object_path,
                          std::string16 *arguments) {
#ifdef WINCE
  char16 target[CHAR_MAX];
  bool ret = SHGetShortcutTarget(link_path, target, CHAR_MAX) == TRUE;
  if (object_path) {
    *object_path = target;
  }
  return ret;
#else
  HRESULT result;
  IShellLink* shell_link;

  result = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            IID_IShellLink, (LPVOID*)&shell_link);
  if (SUCCEEDED(result)) {
    IPersistFile* persist_file;
    result = shell_link->QueryInterface(
                             IID_IPersistFile,
                             reinterpret_cast<void**>(&persist_file));

    if (SUCCEEDED(result)) {
      result = persist_file->Load(link_path, TRUE);

      char16 icon_buffer[MAX_PATH];
      if (SUCCEEDED(result) && icon_path) {
        int icon_id;
        result = shell_link->GetIconLocation(icon_buffer, MAX_PATH, &icon_id);
      }
      char16 object_buffer[MAX_PATH];
      if (SUCCEEDED(result) && object_path) {
        result = shell_link->GetPath(object_buffer, MAX_PATH, NULL, 0);
      }
      char16 arg_buffer[MAX_PATH];
      if (SUCCEEDED(result) && arguments) {
        result = shell_link->GetArguments(arg_buffer, MAX_PATH);
      }

      if (SUCCEEDED(result)) {
        // Only set the return values when the function succeeded.
        if (icon_path)
          *icon_path = icon_buffer;
        if (object_path)
          *object_path = object_buffer;
        if (arguments)
          *arguments = arg_buffer;
      }
      persist_file->Release();
    }
    shell_link->Release();
  }
  return SUCCEEDED(result);
#endif
}

#ifdef WINCE
// TODO(steveblock): Implement GetAppPath for WinCE when needed.
#else
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
#endif

static bool GetShortcutLocationPath(std::string16 *shortcut_location_path) {
  assert(shortcut_location_path);
  bool succeeded = false;
  char16 path_buf[MAX_PATH];

  // We use the old version of this function because the new version apparently
  // won't tell you the Desktop folder path.
  BOOL result = SHGetSpecialFolderPath(NULL, path_buf,
#ifdef WINCE
                                       CSIDL_STARTMENU,
#else
                                       CSIDL_DESKTOPDIRECTORY,
#endif
                                       TRUE);

  if (result) {
    *shortcut_location_path = path_buf;
    succeeded = true;
  }
  return succeeded;
}

// Creates the icon file which contains the various different sized icons.
static bool CreateIcoFile(const std::string16 &icons_path,
                          const DesktopUtils::ShortcutInfo &shortcut) {
  struct IcoHeader {
    uint16 reserved;
    uint16 type;
    uint16 count;
  };

  struct IcoDirectory {
    uint8 width;
    uint8 height;
    uint8 color_count;
    uint8 reserved;
    uint16 planes;
    uint16 bpp;
    uint32 data_size;
    uint32 offset;
  };

  std::vector<const DesktopUtils::IconData *> icons_to_write;

  // Add each icon size that has been provided to the icon list.  We ignore
  // 128x128 because it isn't supported by Windows.
  if (!shortcut.icon48x48.raw_data.empty()) {
    icons_to_write.push_back(&shortcut.icon48x48);
  }

  if (!shortcut.icon32x32.raw_data.empty()) {
    icons_to_write.push_back(&shortcut.icon32x32);
  }

  if (!shortcut.icon16x16.raw_data.empty()) {
    icons_to_write.push_back(&shortcut.icon16x16);
  }

  // Make sure we have at least one icon.
  if (icons_to_write.empty()) {
    return false;
  }

  // Initialize to the size of the header.
  int data_size = sizeof(IcoHeader);

  for (size_t i = 0; i < icons_to_write.size(); ++i) {
    // Increase data_size by size of the icon data.
    data_size += sizeof(BITMAPINFOHEADER);

    // 32 bits per pixel for the image data.
    data_size += 4 * icons_to_write[i]->width * icons_to_write[i]->height;

    // 2 bits per pixel for the AND mask.
    data_size += icons_to_write[i]->height * icons_to_write[i]->height / 4;

    // Increase data_size by size of directory entry.
    data_size += sizeof(IcoDirectory);
  }

  File::CreateNewFile(icons_path.c_str());

  // Allocate the space for the icon.
  uint8 *data = new uint8[data_size];
  memset(data, 0, data_size);

  IcoHeader *icon_header = reinterpret_cast<IcoHeader *>(data);
  icon_header->reserved = 0;  // Must be 0;
  icon_header->type = 1;  // 1 for ico.
  icon_header->count = icons_to_write.size();

  // Icon image data starts past the header and the directory.
  int base_offset = sizeof(IcoHeader) +
      icons_to_write.size() * sizeof(IcoDirectory);
  for (size_t i = 0; i < icons_to_write.size(); ++i) {
    IcoDirectory directory;
    directory.width = icons_to_write[i]->width;
    directory.height = icons_to_write[i]->height;
    directory.color_count = 0;
    directory.reserved = 0;
    directory.planes = 1;
    directory.bpp = 32;

    // Size of the header + size of the pixels + size of the hitmask.
    directory.data_size =
        sizeof(BITMAPINFOHEADER) +
        4 * icons_to_write[i]->width * icons_to_write[i]->height +
        icons_to_write[i]->width * icons_to_write[i]->height / 4;

    directory.offset = base_offset;

    BITMAPINFOHEADER bmp_header;
    memset(&bmp_header, 0, sizeof(bmp_header));
    bmp_header.biSize = sizeof(bmp_header);
    bmp_header.biWidth = icons_to_write[i]->width;
    // Windows expects the height to be doubled for the AND mask(unused).
    bmp_header.biHeight = icons_to_write[i]->height * 2;
    bmp_header.biPlanes = 1;
    bmp_header.biBitCount = 32;

    // Write the directory entry.
    memcpy(&data[sizeof(IcoHeader) + i * sizeof(IcoDirectory)],
           reinterpret_cast<uint8 *>(&directory),
           sizeof(IcoDirectory));

    // Write the bitmap header to the data segment.
    memcpy(&data[base_offset],
           reinterpret_cast<uint8 *>(&bmp_header),
           sizeof(BITMAPINFOHEADER));

    // Move the offset past the header
    base_offset += sizeof(BITMAPINFOHEADER);

    // Iterate across the rows and reverse them.  Icons are stored upside down.
    int row_offset = (icons_to_write[i]->height - 1) *
        4 * icons_to_write[i]->width;
    for (int row = 0; row < icons_to_write[i]->height; ++row) {
      // Copy a single row.
      memcpy(&data[base_offset],
             reinterpret_cast<const uint8*>(
                 &icons_to_write[i]->raw_data.at(row_offset)),
             4 * icons_to_write[i]->width);

      // Move the write offset forward one row.
      base_offset += 4 * icons_to_write[i]->width;

      // Move the read offset back one row.
      row_offset -= 4 * icons_to_write[i]->width;
    }

    // Move the write offset past the AND mask.  The AND mask is unused for
    // icons with 32 bits per pixel; the alpha channel is used instead.
    base_offset += icons_to_write[i]->width * icons_to_write[i]->height / 4;
  }

  bool success = File::WriteBytesToFile(icons_path.c_str(),
                                        data,
                                        data_size);

  delete[] data;

  return success;
}

bool DesktopUtils::CreateDesktopShortcut(const SecurityOrigin &origin,
                                         const ShortcutInfo &shortcut,
                                         std::string16 *error) {
  const char16 *kCreationError = STRING16(L"Could not create desktop icon.");
  bool creation_result = false;
  char16 process_name[MAX_PATH] = {0};

  if (!GetModuleFileName(NULL, process_name, MAX_PATH)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  std::string16 icons_path;
  if (!GetDataDirectory(origin, &icons_path)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  AppendDataName(STRING16(L"icons"), kDataSuffixForDesktop, &icons_path);
  icons_path += kPathSeparator;
  icons_path += shortcut.app_name;
  icons_path += STRING16(L".ico");


#ifdef WINCE
  // We don't yet use the icons in the shortcut.
  // Note that CreateIcoFile requires at least one size of icon to be provided,
  // excluding 128x128. Currently we don't rescale to provide missing icon sizes
  // for WinCE, so it's possible that none of the required sizes will be
  // present.
#else
  if (!CreateIcoFile(icons_path, shortcut)) {
    *error = kCreationError;
    return false;
  }
#endif

  // Note: We assume that shortcut.app_name has been validated as a valid
  // filename and that the shortuct.app_url has been converted to absolute URL
  // by the caller.
  std::string16 link_path;

  if (GetShortcutLocationPath(&link_path)) {
    if (link_path[link_path.size()] != L'\\') {
      link_path += STRING16(L"\\");
    }
    link_path += shortcut.app_name;
    link_path += STRING16(L".lnk");

    std::string16 old_icon;
    if (ReadShellLink(link_path.c_str(), &old_icon, NULL, NULL)) {
      if (!GetLongPath(old_icon.c_str(), &old_icon) ||
          old_icon.find(STRING16(L"Google Gears for")) == std::string16::npos) {
        *error = kCreationError;
        return false;
      }
    }
    creation_result = CreateShellLink(link_path.c_str(),
                                      icons_path.c_str(), process_name,
                                      shortcut.app_url.c_str());
  }

  if (!creation_result) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
  }

  return creation_result;
}
#endif  // WIN32
