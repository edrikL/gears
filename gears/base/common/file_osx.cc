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
// The methods of the File class with an OSX-specific implementation.

// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. OSX_CPPSRCS) are implemented
#ifdef OS_MACOSX
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
#include <CoreFoundation/CoreFoundation.h>

#include "gears/base/common/common.h"
#include "gears/base/common/file.h"
#include "gears/base/common/int_types.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/string16.h"
#include "gears/base/safari/scoped_cf.h"

// Creates the 'Info.plist' manifest file that osx looks for inside the package.
static bool CreateInfoPlist(const std::string16 &contents_path,
                            const std::string16 &icons_file,
                            const std::string16 &script_file,
                            const std::string16 &link_name) {
  scoped_cftype<CFStringRef> icons_file_cfstr(
      CFStringCreateWithCharacters(NULL, icons_file.c_str(),
                                   icons_file.length()));
  scoped_cftype<CFStringRef> script_file_cfstr(
      CFStringCreateWithCharacters(NULL, script_file.c_str(),
                                   script_file.length()));
  scoped_cftype<CFStringRef> link_name_cfstr(
      CFStringCreateWithCharacters(NULL, link_name.c_str(),
                                   link_name.length()));

  // Build up a dictionary of the required keys and values
  scoped_cftype<CFMutableDictionaryRef> dict(
      CFDictionaryCreateMutable(NULL,
                                0, // initial capacity
                                &kCFTypeDictionaryKeyCallBacks,
                                &kCFTypeDictionaryValueCallBacks));
  
  CFDictionarySetValue(dict.get(), CFSTR("CFBundleInfoDictionaryVersion"),
                       CFSTR("6.0"));
  CFDictionarySetValue(dict.get(), CFSTR("CFBundlePackageType"),
                       CFSTR("APPL")); // "application" type
  CFDictionarySetValue(dict.get(), CFSTR("CFBundleDevelopmentRegion"),
                       CFSTR("English"));
  CFDictionarySetValue(dict.get(), CFSTR("CFBundleName"),
                       link_name_cfstr.get());
  CFDictionarySetValue(dict.get(), CFSTR("CFBundleIconFile"),
                       icons_file_cfstr.get());
  CFDictionarySetValue(dict.get(), CFSTR("CFBundleExecutable"),
                       script_file_cfstr.get());

  // Save the data
  scoped_cftype<CFDataRef> plist_xml(
      CFPropertyListCreateXMLData(NULL, dict.get()));
  const UInt8 *first_byte = CFDataGetBytePtr(plist_xml.get());
  int length = CFDataGetLength(plist_xml.get());

  std::string16 plist_path(contents_path + STRING16(L"/Info.plist"));
  if (!File::CreateNewFile(plist_path.c_str()))
    return false;

  return File::WriteBytesToFile(plist_path.c_str(), first_byte, length);
}


// Creates the shell script inside the package which is used to actually start
// the browser.
static bool CreateShellScript(const std::string16 &script_path,
                              const std::string16 &launch_url) {
  // Get the command of the currently running process
  ProcessSerialNumber process_serial;
  OSErr error = GetCurrentProcess(&process_serial);
  if (error != noErr) { return false; }

  FSRef process_location;
  error = GetProcessBundleLocation(&process_serial, &process_location);
  if (error != noErr) { return false; }

  UInt8 process_path[MAXPATHLEN] = {0};
  error = FSRefMakePath(&process_location, process_path, MAXPATHLEN);
  if (error != noErr) { return false; }

  // Build up the shell script
  std::string launch_url_utf8;
  if (!String16ToUTF8(launch_url.c_str(), launch_url.size(),
                      &launch_url_utf8)) {
    return false;
  }

  std::string contents("#!/bin/sh\n");
  contents += "open -a ";
  contents += reinterpret_cast<const char *>(process_path);
  contents += " ";
  contents += launch_url_utf8;
  contents += "\n";

  // Write to file
  if (!File::CreateNewFile(script_path.c_str()))
    return false;

  return File::WriteBytesToFile(
                   script_path.c_str(),
                   reinterpret_cast<const uint8 *>(contents.c_str()),
                   contents.length());
}


// Defines the visible icon for a given icon size
static bool SetIconData(IconFamilyHandle family_handle,
                        const File::IconData *icon_data, OSType icon_type) {
  scoped_Handle data_handle(NewHandle(icon_data->bytes.size()));
  if (!data_handle.get()) return false;

  // Copy the icon data into the handle and convert from RGBA to ARGB
  const std::vector<char> *icon_bytes =
    reinterpret_cast<const std::vector<char> *>(&icon_data->bytes);
  char *dest = static_cast<char *>(*data_handle.get());

  for (size_t i = 0; i < icon_bytes->size(); i += 4) {
    *dest = icon_bytes->at(i + 3);
    *(dest + 1) = icon_bytes->at(i);
    *(dest + 2) = icon_bytes->at(i + 1);
    *(dest + 3) = icon_bytes->at(i + 2);
    dest += 4;
  }

  // Add the converted data to the icon family
  OSErr error = SetIconFamilyData(family_handle, icon_type, data_handle.get());
  return error == noErr;
}


// Defines the alpha for a given icon size
static bool SetIconAlphaMask(IconFamilyHandle family_handle,
                             const File::IconData *icon_data,
                             OSType icon_type) {
  scoped_Handle alpha_handle(NewHandle(icon_data->width * icon_data->height));
  if (!alpha_handle.get()) return false;

  // Copy the alpha data into the handle
  const std::vector<char> *icon_bytes =
    reinterpret_cast<const std::vector<char> *>(&icon_data->bytes);
  char *dest = static_cast<char *>(*alpha_handle.get());

  for (size_t i = 0; i < icon_data->bytes.size(); i += 4) {
    *dest = icon_bytes->at(i + 3);
    ++dest;
  }

  // Add the converted data to the icon family
  OSErr error = SetIconFamilyData(family_handle, icon_type, alpha_handle.get());
  return error == noErr;
}


// Defines the clickable area for a given icon size
static bool SetIconHitMask(IconFamilyHandle family_handle,
                           const File::IconData *icon_data, OSType icon_type) {
  // NOTE: It would seem that you only need w * h / 8 bytes for this hit mask,
  // but that doesn't work. I don't understand why, but OSX actually wants twice
  // that much data. I figured this out by looking closely at what the
  // IconFamily open source project does:
  // http://iconfamily.svn.sourceforge.net/viewvc/iconfamily/trunk/source/MakeThumbnail/IconFamily.m
  scoped_Handle hit_handle(NewHandle(
    icon_data->width * icon_data->height / 4));
  if (!hit_handle.get()) return false;

  // Create the hit mask. We consider a pixel clickable if it isn't totally
  // transparent.
  const std::vector<char> *icon_bytes =
    reinterpret_cast<const std::vector<char> *>(&icon_data->bytes);
  char *start = static_cast<char *>(*hit_handle.get());
  char *dest = start;
  
  if (icon_data->bytes.size() < 4) {
    assert(false);
    return false;
  }
  
  size_t i = 3; // Alpha pixel is last of every group of four.
  while (i < icon_bytes->size()) {
    char next_byte = 0;
    next_byte |= (icon_bytes->at(i) & 0xff) ? 0x80 : 0; i += 4;
    next_byte |= (icon_bytes->at(i) & 0xff) ? 0x40 : 0; i += 4;
    next_byte |= (icon_bytes->at(i) & 0xff) ? 0x20 : 0; i += 4;
    next_byte |= (icon_bytes->at(i) & 0xff) ? 0x10 : 0; i += 4;
    next_byte |= (icon_bytes->at(i) & 0xff) ? 0x08 : 0; i += 4;
    next_byte |= (icon_bytes->at(i) & 0xff) ? 0x04 : 0; i += 4;
    next_byte |= (icon_bytes->at(i) & 0xff) ? 0x02 : 0; i += 4;
    next_byte |= (icon_bytes->at(i) & 0xff) ? 0x01 : 0; i += 4;
    *dest = next_byte;
    ++dest;
  }

  // I am not sure what the two different copies of this data get used for, but
  // this is what IconFamily does. Without this copy at least, the hit shape is
  // not correct.
  memcpy(dest, start, icon_data->width * icon_data->height / 8);

  // Add the converted data to the icon family
  OSErr error = SetIconFamilyData(family_handle, icon_type, hit_handle.get());
  return error == noErr;
}


// Creates the icon file which contains the various different sized icons.
static bool CreateIcnsFile(const std::string16 &icons_path,
                           const File::DesktopIcons &icons) {
  scoped_Handle handle(NewHandle(0));
  if (!handle.get()) { return false; }

  IconFamilyHandle family_handle =
      reinterpret_cast<IconFamilyHandle>(handle.get());

  if (!icons.icon16x16.bytes.empty()) {
    if (!SetIconData(family_handle, &icons.icon16x16, kSmall32BitData) ||
        !SetIconAlphaMask(family_handle, &icons.icon16x16, kSmall8BitMask) ||
        !SetIconHitMask(family_handle, &icons.icon16x16, kSmall1BitMask))
      return false;
  }

  if (!icons.icon32x32.bytes.empty()) {
    if (!SetIconData(family_handle, &icons.icon32x32, kLarge32BitData) ||
        !SetIconAlphaMask(family_handle, &icons.icon32x32, kLarge8BitMask) ||
        !SetIconHitMask(family_handle, &icons.icon32x32, kLarge1BitMask))
      return false;
  }

  if (!icons.icon48x48.bytes.empty()) {
    if (!SetIconData(family_handle, &icons.icon48x48, kHuge32BitData) ||
        !SetIconAlphaMask(family_handle, &icons.icon48x48, kHuge8BitMask) ||
        !SetIconHitMask(family_handle, &icons.icon48x48, kHuge1BitMask))
      return false;
  }

  if (!icons.icon128x128.bytes.empty()) {
    // For 128, the hit mask is computed from the alpha mask
    if (!SetIconData(family_handle, &icons.icon128x128, kThumbnail32BitData) ||
        !SetIconAlphaMask(family_handle, &icons.icon128x128,
                          kThumbnail8BitMask))
      return false;
  }

  // Now write the data out. :: sigh ::
  int handle_size = GetHandleSize(
      reinterpret_cast<char **>(family_handle));
  if (!File::CreateNewFile(icons_path.c_str()))
    return false;

  return File::WriteBytesToFile(icons_path.c_str(),
                                reinterpret_cast<const uint8 *>(*family_handle),
                                handle_size);
}


// Gets the full path to the application package on the desktop.
static bool GetApplicationPath(const std::string16 &app_name,
                               std::string16 *path) {
  FSRef desktop_folder;
  OSErr err;
  
  err = FSFindFolder(kUserDomain, kDesktopFolderType, kDontCreateFolder,
                     &desktop_folder);
  if (err != noErr) return false;

  UInt8 path_buffer[MAXPATHLEN] = {0};
  err = FSRefMakePath(&desktop_folder, path_buffer, MAXPATHLEN);
  if (err != noErr) return false;

  if (!UTF8ToString16(reinterpret_cast<char *>(&path_buffer),
                      strlen(reinterpret_cast<char *>(&path_buffer)),
                      path))
    return false;

  // Note: we assume that the caller (the desktop module) has already validated
  // that this user input does not contain any illegal characters.
  path->append(STRING16(L"/"));
  path->append(app_name);
  path->append(STRING16(L".app"));

  return true;
}


// Implements creation of desktop shortcuts to a web application on mac. On mac,
// shortcuts aren't used the same way they are on pc, so this does something
// more appropriate: creates an application package containing a shell script to
// open the browser to the correct URL.
bool File::CreateDesktopShortcut(const SecurityOrigin &origin,
                                 const std::string16 &link_name,
                                 const std::string16 &launch_url,
                                 const DesktopIcons &icons,
                                 std::string16 *error) {
  // Build the bundle in the temp folder so that if something goes wrong we
  // don't leave a partially built bundle on the desktop.
  std::string16 temp_path;
  if (!File::CreateNewTempDirectory(&temp_path)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }
  
  std::string16 contents_path(temp_path + STRING16(L"/Contents"));
  std::string16 mac_os_path(contents_path + STRING16(L"/MacOS"));
  std::string16 resources_path(contents_path + STRING16(L"/Resources"));

  // Create our directory structure
  if (!File::RecursivelyCreateDir(mac_os_path.c_str())) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }
  if (!File::RecursivelyCreateDir(resources_path.c_str())) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  // Create files
  // NOTE: The system implicitly add a ".icns" extension to whatever you specify
  // as the icon file in the plist. The executable (in our case the shell
  // script) is different because it could be any format.
  std::string16 icons_file(STRING16(L"icons"));
  std::string16 script_file(STRING16(L"launch.sh"));

  std::string16 icons_path(resources_path + STRING16(L"/") + icons_file +
                           STRING16(L".icns"));
  std::string16 script_path(mac_os_path + STRING16(L"/") + script_file);

  if (!CreateInfoPlist(contents_path, icons_file, script_file, link_name)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }
  if (!CreateShellScript(script_path, launch_url)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }
  if (!CreateIcnsFile(icons_path, icons)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  // Move to the desktop. Replace any existing icon.
  std::string16 application_path;
  if (!GetApplicationPath(link_name, &application_path)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }
  File::DeleteRecursively(application_path.c_str());
  if (!File::MoveDirectory(temp_path.c_str(), application_path.c_str())) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  return true;
}
#endif  // #ifdef OS_MACOSX
