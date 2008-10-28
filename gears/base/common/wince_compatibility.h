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
// sources (e.g. WIN32_CPPSRCS) are implemented.
#ifdef OS_WINCE
#ifndef GEARS_BASE_COMMON_WINCE_COMPATIBILITY_H__
#define GEARS_BASE_COMMON_WINCE_COMPATIBILITY_H__

#include <atlbase.h>
#include <atlsync.h>
#include <connmgr.h>
#include <connmgr_status.h>
#include <crtdefs.h>
#include <windows.h>

#include "gears/base/common/basictypes.h"  // for DISALLOW_EVIL_CONSTRUCTORS
#include "gears/base/common/string16.h"
#include "gears/localserver/common/critical_section.h"

class JsRunnerInterface;

// The Win32 code uses ::PathFindXXXW. These are defined in shlwapi.h, but the
// WinCE SDK does not provide shlwapi.lib. Instead it provides
// ATL::PathFindXXX, which are defined in atlosapice.h (included from
// atlbase.h). However, ATL::PathFindXXX is not provided by the Win32 SDK.
// Therefore we use ATL::PathFindXXX for WinCE only and use a 'using'
// declaration to keep the code identical for the two platforms.
//
// Note that atlosapice.h provides a define from PathFindExtension to
// PathFindExtensionW, but not for PathFindFileNameW. shlwapi.h provides defines
// for both functions, but inclduing this file introduces ambiguity between the
// ATL::PathFindXXXW and ::PathFindXXXW functions.
namespace ATL {
#define PathFindFileNameW PathFindFileName
}
using ATL::PathFindExtensionW;
using ATL::PathFindFileName;

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

#ifdef BROWSER_IE
class BrowserCache {
 public:
  // This method adds a zero-size entry of type EDITED_CACHE_ENTRY to the
  // browser cache. This is used by the LocalServer as a work-around for the
  // fact that WinCE pops up a 'Cannot Connect' dialog when serving resources
  // from the LocalServer when offline.
  static bool EnsureBogusEntry(const char16 *url);
  static bool RemoveBogusEntry(const char16 *url);
  DISALLOW_EVIL_CONSTRUCTORS(BrowserCache);
};

// This should only be called in the context of the main page.
void CallWindowOnerror(JsRunnerInterface *js_runner,
                       const std::string16 &message);

// This is a WinCE specific interface for entities that wish to handle
// page unload events. Implementors of this interface would register with the
// UnloadEventSource class in order to receive the page unload events.
class UnloadEventHandlerInterface {
public:
  virtual ~UnloadEventHandlerInterface() {}
  virtual void HandleFakeEvent() = 0;
};
// This is a WinCE specific class for entities that are capable of
// notifying a concrete implementation of UnloadEventHandlerInterface that
// the page is unloading. Such an entity is the GearsFactoryImpl class which,
// with its PrivateSendUnloadEvent, can be told by the client application to
// manually deliver a fake unload event.
class UnloadEventSource {
 public:
  static void SendUnload();
  static void RegisterHandler(UnloadEventHandlerInterface *handler);
  static void UnregisterHandler();
 private:
  UnloadEventSource();

  static UnloadEventHandlerInterface *handler_;

  DISALLOW_EVIL_CONSTRUCTORS(UnloadEventSource);
};
#endif  // BROWSER_IE

// Localization utilities

// Returns the system local which the phone is operating in.
// On smartphones this will be the locale of the selected language as opposed
// to the selected system locale (which is a good thing).
bool GetCurrentSystemLocale(std::string16 *locale);

#ifdef DEBUG
// ATLTRACE for WinCE takes a wide string, so we can not call it directly.
// Instead we convert the message and call ATL::CTrace so that we can pass the
// the file name and line number from the call site.
//
// TODO(cprince): Remove this class as part of LOG() refactoring.
// Also note that LOG() calls should take string16, so the string conversion
// done by this class can go away at that time.
class GearsTrace {
 public:
  GearsTrace(const char* file_name, int line_no);

  void operator() (const char* format, ...) const;

 private:
  GearsTrace& operator=(const GearsTrace& other);
  const char *const file_name_;
  const int line_no_;
};
#endif

#endif  // GEARS_BASE_COMMON_WINCE_COMPATIBILITY_H__
#endif  // OS_WINCE
