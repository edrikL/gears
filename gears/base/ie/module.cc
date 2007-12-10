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

#include "ie/genfiles/interfaces.h" // from OUTDIR
#include "gears/base/common/thread_locals.h"
#include "gears/base/ie/resource.h" // for .rgs resource ids (IDR_*)

class DllModule : public CAtlDllModuleT< DllModule > {
 public:
  DECLARE_LIBID(LIBID_GearsTypelib)
  DECLARE_REGISTRY_APPID_RESOURCEID(IDR_SCOURIE, \
  "{B56936F7-0433-4E0B-921B-D095E7142B6D}")
};

DllModule atl_module;
#ifdef WINCE
int _charmax = 255;
#endif

inline BOOL MyDllMain(HANDLE instance, DWORD reason, LPVOID reserved) {
  switch (reason) {
    case DLL_THREAD_DETACH:
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
  return atl_module.DllRegisterServer();
}

STDAPI DllUnregisterServer(void) {
  return atl_module.DllUnregisterServer();
}
