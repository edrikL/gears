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

#include "gears/desktop/desktop.h"

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
#if BROWSER_IE
#include "gears/base/common/vista_utils.h"
#endif
#include "gears/desktop/shortcut_utils_win32.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"


// Creates the icon file which contains the various different sized icons.	
static bool CreateIcoFile(const std::string16 &icons_path,
                          const GearsDesktop::ShortcutInfo &shortcut) {
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

  std::vector<const GearsDesktop::IconData *> icons_to_write;

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

    // Increase data_size by size of the image and mask data.

    // Note: in the .ico format, each *row* of image and mask data must be
    // a multiple of 4 bytes.  Pad if necessary.
    int row_bytes;

    // 32 bits per pixel for the image data.
    row_bytes = icons_to_write[i]->width * 4;  // already a multiple of 4
    data_size += row_bytes * icons_to_write[i]->height;

    // 1 bit per pixel for the XOR mask, then 1 bit per pixel for the AND mask.
    row_bytes = icons_to_write[i]->width / 8;
    row_bytes = ((row_bytes + 3) / 4) * 4;  // round up to multiple of 4
    data_size += row_bytes * icons_to_write[i]->height;  // XOR mask
    data_size += row_bytes * icons_to_write[i]->height;  // AND mask

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
  int dest_offset =
      sizeof(IcoHeader) +
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

    directory.offset = dest_offset;

    BITMAPINFOHEADER bmp_header;
    memset(&bmp_header, 0, sizeof(bmp_header));
    bmp_header.biSize = sizeof(bmp_header);
    bmp_header.biWidth = icons_to_write[i]->width;
    // 'biHeight' is the combined height of the XOR and AND masks.
    bmp_header.biHeight = icons_to_write[i]->height * 2;
    bmp_header.biPlanes = 1;
    bmp_header.biBitCount = 32;

    // Write the directory entry.
    memcpy(&data[sizeof(IcoHeader) + i * sizeof(IcoDirectory)],
           reinterpret_cast<uint8 *>(&directory),
           sizeof(IcoDirectory));

    // Write the bitmap header to the data segment.
    memcpy(&data[dest_offset],
           reinterpret_cast<uint8 *>(&bmp_header),
           sizeof(BITMAPINFOHEADER));

    // Move the offset past the header.
    dest_offset += sizeof(BITMAPINFOHEADER);

    // Write the color data.
    // Note that icon rows are stored bottom to top, so we flip the row order.
    for (int row = (icons_to_write[i]->height - 1); row >= 0; --row) {
      int src_row_offset = row * icons_to_write[i]->width * 4;

      // Copy a single row.
      memcpy(&data[dest_offset],
             reinterpret_cast<const uint8*>(
                 &icons_to_write[i]->raw_data.at(src_row_offset)),
             4 * icons_to_write[i]->width);

      // Move the write offset forward one row.
      dest_offset += 4 * icons_to_write[i]->width;
    }

    // Compute mask information.
    int mask_row_bytes = icons_to_write[i]->width / 8;
    mask_row_bytes = ((mask_row_bytes + 3) / 4) * 4;  // round up, multiple of 4

    // Write the XOR mask.
    for (int row = (icons_to_write[i]->height - 1); row >= 0; --row) {
      // 'stripe' is an 8-column segment, for grouping 1bpp data into bytes
      for (int stripe = 0; stripe < (icons_to_write[i]->width / 8); ++stripe) {
        int xor_mask_value = 0;
        for (int bit = 0; bit < 8; ++bit) {
          // If alpha is 0x00, make bit transparent (1), else leave opaque (0).
          int raw_pixel_offset =
              (((row * icons_to_write[i]->width) + (stripe * 8) + bit) * 4);
          if (0 == icons_to_write[i]->raw_data.at(raw_pixel_offset + 3)) {
            xor_mask_value |= (0x80 >> bit);
          }
        }
        data[dest_offset + stripe] = static_cast<uint8>(xor_mask_value);
      }
      // Update offset *after* finishing mask row, as it may include padding.
      dest_offset += mask_row_bytes;
    }

    // Move the write offset past the AND mask (unused in WinXP and later).
    dest_offset += mask_row_bytes * icons_to_write[i]->height;
  }

  bool success = File::WriteBytesToFile(icons_path.c_str(),
                                        data,
                                        data_size);

  delete[] data;

  return success;
}

bool GearsDesktop::CreateShortcutPlatformImpl(const SecurityOrigin &origin,
                                              const ShortcutInfo &shortcut,
                                              std::string16 *error) {
  char16 browser_path[MAX_PATH] = {0};

  if (!GetModuleFileName(NULL, browser_path, MAX_PATH)) {
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
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }
#endif

#if BROWSER_IE
  if (VistaUtils::IsRunningOnVista()) {
    char16 gears_dll_path[MAX_PATH] = {0};
    if (!GetModuleFileName(_AtlBaseModule.GetModuleInstance(), gears_dll_path,
                           MAX_PATH)) {
      *error = GET_INTERNAL_ERROR_MESSAGE();
      return false;
    }

    std::string16 broker_path;
    if (!File::GetParentDirectory(gears_dll_path, &broker_path)) {
      *error = GET_INTERNAL_ERROR_MESSAGE();
      return false;
    }

    broker_path += STRING16(L"\\vista_broker.exe");

    // Build up the command line
    const char16 *command_line_parts[] = {
      broker_path.c_str(),
      shortcut.app_name.c_str(),
      browser_path,
      shortcut.app_url.c_str(),
      icons_path.c_str(),
    };

    std::string16 command_line;
    for (int i = 0; i < ARRAYSIZE(command_line_parts); ++i) {
      // Escape any double quotes since we also use them for argument
      // delimiters.
      std::string16 part(command_line_parts[i]);
      ReplaceAll(part, std::string16(STRING16(L"\\")),
                 std::string16(STRING16(L"\\\\")));

      command_line += STRING16(L"\"");
      command_line += part;
      command_line += STRING16(L"\" ");
    }

    STARTUPINFO startup_info = {0};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info = {0};

    // TODO(aa): CreateProcessW is modifying command_line's data in the middle.
    // Is this OK?
    BOOL success = CreateProcessW(NULL,  // get command from command line
                                  const_cast<char16 *>(command_line.c_str()),
                                  NULL,  // process handle not inheritable
                                  NULL,  // thread handle not inheritable
                                  FALSE, // set handle inheritance to FALSE
                                  0,     // no creation flags
                                  NULL,  // use parent's environment block
                                  NULL,  // use parent's starting block
                                  &startup_info,
                                  &process_info);
    if (!success) {
      *error = GET_INTERNAL_ERROR_MESSAGE();
      return false;
    }

    // To make sure that process and thread handles gets destroyed on exit.
    SAFE_HANDLE process_handle(process_info.hProcess);
    SAFE_HANDLE thread_handle(process_info.hThread);

    // Wait for process to exit.
    const int kTimeoutMs = 10000;
    if (WaitForSingleObject(process_info.hProcess,
                            kTimeoutMs) != WAIT_OBJECT_0) {
      *error = GET_INTERNAL_ERROR_MESSAGE();
      return false;
    }

    DWORD return_value = 0;
    if (!GetExitCodeProcess(process_info.hProcess, &return_value)) {
      *error = GET_INTERNAL_ERROR_MESSAGE();
      return false;
    }

    if (return_value != 0) {
      *error = STRING16(L"Could not create shortcut. Broker failed on line: ");
      *error += IntegerToString16(return_value);
      *error += STRING16(L".");
      return false;
    }
    return true;
  }
#endif

  return CreateShortcutFileWin32(shortcut.app_name, browser_path,
                                 shortcut.app_url, icons_path, error);
}
#endif  // #ifdef WIN32
