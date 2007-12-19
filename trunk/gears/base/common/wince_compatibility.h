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

// TODO(andreip): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented
#ifdef WINCE
#ifndef GEARS_BASE_COMMON_WINCE_COMPATIBILITY_H__
#define GEARS_BASE_COMMON_WINCE_COMPATIBILITY_H__

#include <atlsync.h>
#include <connmgr.h>
#include <connmgr_status.h>
#include <crtdefs.h>
#include <windows.h>

#include "gears/localserver/common/critical_section.h"

#define CSIDL_LOCAL_APPDATA CSIDL_APPDATA
#define SHGFP_TYPE_CURRENT 0
#define SHGetFolderPath SHGetFolderPathW

// Windows Mobile doesn't provide _beginthreadex or _beginthread. Note that
// unlike on the desktop, CreateThread for WinCE does support the CRT.
inline uintptr_t _beginthreadex(void *security,
                                unsigned stack_size,
                                unsigned (*start_address)(void*),
                                void *arglist,
                                unsigned initflag,
                                unsigned *thrdaddr) {
  return reinterpret_cast<uintptr_t>(CreateThread(
      reinterpret_cast<SECURITY_ATTRIBUTES*>(security),
      stack_size,
      reinterpret_cast<DWORD (*)(void*)>(start_address),
      arglist,
      initflag,
      reinterpret_cast<DWORD*>(thrdaddr)));
}

void GetSystemTimeAsFileTime(LPFILETIME filetime);

DWORD GetShortPathNameW(LPCTSTR path_long,
                       LPTSTR path_short,
                       DWORD path_short_max_size);

int SHCreateDirectoryEx(HWND window,
                        LPCTSTR full_dirpath,
                        const SECURITY_ATTRIBUTES *security_attributes);

HRESULT SHGetFolderPathW(HWND hwndOwner,
                        int nFolder,
                        HANDLE hToken,
                        DWORD dwFlags,
                        LPTSTR pszPath);

BOOL IsNetworkAlive(LPDWORD lpdwFlags);

class CMutexWince : public ATL::CMutex {
 public:
  // CMutex::Open() in atlsync.h is #ifdef-ed out presumably
  // due to the (bizarre) lack of OpenMutex() win32 API in Windows Mobile.
  BOOL Open(DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName);
 private:
  static HANDLE global_mutex_;
  static CriticalSection lock_;
};

#endif  // GEARS_BASE_COMMON_WINCE_COMPATIBILITY_H__
#endif  // WINCE
