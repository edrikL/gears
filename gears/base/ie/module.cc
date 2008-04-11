// Copyright 2006, Google Inc.
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

#include "gears/base/common/thread_locals.h"
#include "gears/base/ie/resource.h" // for .rgs resource ids (IDR_*)
#include "genfiles/interfaces.h"
#include "genfiles/registry_strings.h"

// This is defined in gears/base/common/message_queue_ie.h.  It should only be
// called here.
void ShutdownThreadMessageQueue();

class DllModule : public CAtlDllModuleT< DllModule > {
 public:
  DECLARE_LIBID(LIBID_GearsTypelib)
  DECLARE_REGISTRY_APPID_RESOURCEID(IDR_GEARSIE, \
  "{B56936F7-0433-4E0B-921B-D095E7142B6D}")
};

DllModule atl_module;
#ifdef WINCE
int _charmax = 255;
#endif

inline BOOL MyDllMain(HANDLE instance, DWORD reason, LPVOID reserved) {
  switch (reason) {
    case DLL_THREAD_DETACH:
      ShutdownThreadMessageQueue();
      ThreadLocals::HandleThreadDetached();
      break;
    case DLL_PROCESS_DETACH:
      ThreadLocals::HandleProcessDetached();
      break;
  }

  BOOL rv = atl_module.DllMain(reason, reserved);

  if ((reason == DLL_PROCESS_ATTACH) && rv) {
    rv = ThreadLocals::HandleProcessAttached();
  }

  return rv;
}

extern "C"
BOOL WINAPI DllMain(HANDLE instance, DWORD reason, LPVOID reserved) {
 return MyDllMain(instance, reason, reserved);
}


STDAPI DllCanUnloadNow(void) {
  return atl_module.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID class_id, REFIID riid, LPVOID* ppv) {
  return atl_module.DllGetClassObject(class_id, riid, ppv);
}

STDAPI DllRegisterServer(void) {
  HRESULT hr = atl_module.DllRegisterServer();
#ifdef WINCE
  // TODO(zork): Move WinCE part of tools_menu_item.rgs.m4 here as well.
#else
  if (SUCCEEDED(hr)) {
    // rgs scripts don't support Unicode, so we manually add the localized
    // strings to the registry.
    for (int i = 0; i < ARRAYSIZE(registry_keys); ++i) {
      HKEY hkey;

      if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, registry_keys[i], 0, NULL, 0,
                         KEY_WRITE, NULL, &hkey, NULL)) {
        // These keys are the text for the IE 'Tools' menu item.
        RegSetValueEx(
            hkey, STRING16(L"MenuText"), 0, REG_SZ,
            reinterpret_cast<const BYTE *>(registry_menu_text[i]),
            (char16_wcslen(registry_menu_text[i]) + 1) * sizeof(char16));
        RegSetValueEx(
            hkey, STRING16(L"MenuStatusBar"), 0, REG_SZ,
            reinterpret_cast<const BYTE *>(registry_menu_status_bar[i]),
            (char16_wcslen(registry_menu_status_bar[i]) + 1) * sizeof(char16));
      }
    }
  }
#endif

  return hr;
}

STDAPI DllUnregisterServer(void) {
#ifdef WINCE
  // TODO(zork): Move WinCE part of tools_menu_item.rgs.m4 here as well.
#else
  // Remove the localized strings from the registry.
  for (int i = 0; i < ARRAYSIZE(registry_keys); ++i) {
    RegDeleteKey(HKEY_LOCAL_MACHINE, registry_keys[i]);
  }
#endif
  return atl_module.DllUnregisterServer();
}
