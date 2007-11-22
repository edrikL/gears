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
#if defined(WIN32) && !defined(WINCE)
#include <windows.h>
#include <assert.h>
#include <shlobj.h>
#include <wininet.h>
#include <shlwapi.h>
#include <tchar.h>
#include "gears/base/common/common.h"
#include "gears/base/common/file.h"
#include "gears/base/common/int_types.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/localserver/common/http_constants.h"

static bool CreateShellLink(const char16 *object_path,
                            const char16 *link_path,
                            const char16 *icon_path,
                            const char16 *arguments) {
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

// Creates the icon file which contains the various different sized icons.
static bool CreateIcoFile(const std::string16 &icons_path,
                           const File::DesktopIcons &icons) {
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

  // Initialize to the size of the header.
  int data_size = sizeof(IcoHeader);

  std::vector<const File::IconData *> icons_to_write;

  if (!icons.icon16x16.raw_data.empty()) {
    icons_to_write.push_back(&icons.icon16x16);

    // Increase data_size by size of the icon data.
    data_size += sizeof(BITMAPINFOHEADER);

    // 32 bits per pixel for the image data.
    data_size += 4 * icons.icon16x16.width * icons.icon16x16.height;

    // 2 bits per pixel for the AND mask.
    data_size += icons.icon16x16.height * icons.icon16x16.height / 4;

    // Increase data_size by size of directory entry.
    data_size += sizeof(IcoDirectory);
  }

  if (!icons.icon32x32.raw_data.empty()) {
    icons_to_write.push_back(&icons.icon32x32);

    // Increase data_size by size of the icon data.
    data_size += sizeof(BITMAPINFOHEADER);

    // 32 bits per pixel for the image data.
    data_size += 4 * icons.icon32x32.width * icons.icon32x32.height;

    // 2 bits per pixel for the AND mask.
    data_size += icons.icon32x32.height * icons.icon32x32.height / 4;

    // Increase data_size by size of directory entry.
    data_size += sizeof(IcoDirectory);
  }

  if (!icons.icon48x48.raw_data.empty()) {
    icons_to_write.push_back(&icons.icon48x48);

    // Increase data_size by size of the icon data.
    data_size += sizeof(BITMAPINFOHEADER);

    // 32 bits per pixel for the image data.
    data_size += 4 * icons.icon48x48.width * icons.icon48x48.height;

    // 2 bits per pixel for the AND mask.
    data_size += icons.icon48x48.height * icons.icon48x48.height / 4;

    // Increase data_size by size of directory entry.
    data_size += sizeof(IcoDirectory);
  }

  // Make sure we have at least one icon.
  if (icons_to_write.empty()) {
    return false;
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
  for (unsigned int i = 0; i < icons_to_write.size(); ++i) {
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

bool File::CreateDesktopShortcut(const SecurityOrigin &origin,
                                 const std::string16 &link_name,
                                 const std::string16 &launch_url,
                                 const DesktopIcons &icons,
                                 std::string16 *error) {
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

  if (!AppendDataName(STRING16(L"icons"), kDataSuffixForDesktop, &icons_path)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  icons_path += kPathSeparator;
  icons_path += link_name;
  icons_path += STRING16(L".ico");


  if (!CreateIcoFile(icons_path, icons)) {
    *error = STRING16(L"Could not create desktop icon.");
    return false;
  }

  // Note: We assume that link_name has been validated as a valid filename and
  // that the launch_url has been converted to absolute URL by the caller.
  std::string16 link_path;

  if (GetDesktopPath(&link_path)) {
    if (link_path[link_path.size()] != L'\\') {
      link_path += STRING16(L"\\");
    }
    link_path += link_name;
    link_path += STRING16(L".lnk");
    creation_result = CreateShellLink(process_name, link_path.c_str(),
                                      icons_path.c_str(), launch_url.c_str());
  }

  if (!creation_result) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
  }

  return creation_result;
}
#endif  // #if defined(WIN32) && !defined(WINCE)
