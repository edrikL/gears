// Copyright 2009, Google Inc.
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

#include <tchar.h>
#include <windows.h>
#include <shellapi.h>

#include "gears/base/common/common.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int) {
  CoInitializeEx(NULL, GEARS_COINIT_THREAD_MODEL);

  // usage: run_gears_dll.exe <exported_function_name>
  int argc;
  char16 **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (!argv)
    return __LINE__;  // return line as a ghetto error code
  if (argc != 2)
    return __LINE__;
  std::string function_name(String16ToUTF8(argv[1]));
  LocalFree(argv);

  // The gears.dll must reside in the same directory and be named "gears.dll"
  char16 module_path[MAX_PATH] = {0};
  if (0 == ::GetModuleFileName(instance, module_path, MAX_PATH)) {
    return false;
  }
  PathRemoveFileSpec(module_path);
  PathAppend(module_path, L"gears.dll");

  HMODULE module = LoadLibraryW(module_path);
  if (!module)
    return __LINE__;
  PROC function = GetProcAddress(module, function_name.c_str());
  if (!function)
    return __LINE__;

  (*function)();

  return 0;
}
