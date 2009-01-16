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

#include <urlmon.h>

#include "gears/localserver/ie/http_patches.h"

#include "gears/base/ie/iat_patch.h"
#include "gears/base/ie/ie_version.h"
#include "gears/base/ie/vtable_patch.h"
#include "gears/localserver/ie/http_handler_patch.h"
#include "third_party/passthru_app/urlmon_ie7_extras.h"

static const char *kUrlMonDllName = "urlmon.dll";

//------------------------------------------------------------------------------
// IAT patch for CoInternetQueryInfo exported from URLMON. Thia API call is
// used by IE to determine the cachedness of a URL. When IE is explicitly set
// to "work offline" a couple bad behaviors happen around URLs that are in the
// Gears cache but are not in the browser cache.
//
// * Navigating to uncached URLs brings up an alert that prompts users to
//   connect or cancel the navigation and stay offline.
// * Hovering over uncached links show a cursor embellished with a no-smoking
//   glyph indicating that you can't click on it.
//------------------------------------------------------------------------------

typedef HRESULT (WINAPI* CoInternetQueryInfo_Fn)(LPCWSTR url,
                                                 QUERYOPTION option,
                                                 DWORD flags,
                                                 LPVOID buffer,
                                                 DWORD buffer_len,
                                                 DWORD *buffer_len_out,
                                                 DWORD reserved);

static iat_patch::IATPatchFunction ieframe_patch;
static iat_patch::IATPatchFunction mshtml_patch;


STDAPI Hook_CoInternetQueryInfo(LPCWSTR url,
                                QUERYOPTION option,
                                DWORD flags,
                                LPVOID buffer,
                                DWORD buffer_len,
                                DWORD *buffer_len_out,
                                DWORD reserved) {
  HRESULT hr = HttpHandlerBase::CoInternetQueryInfo(url, option, flags, buffer,
                                                    buffer_len, buffer_len_out,
                                                    reserved);
  if (hr == INET_E_DEFAULT_ACTION) {
    // TODO(michaeln): Chaining, for now we call urlmon directly.
    hr = ::CoInternetQueryInfo(url, option, flags, buffer,
                               buffer_len, buffer_len_out,
                               reserved);
  }
  assert(ieframe_patch.check_patch());
  assert(mshtml_patch.check_patch());
  return hr;
}


static HRESULT PatchCoInternetQueryInfoForModule(
                                   const char *module_name,
                                   iat_patch::IATPatchFunction *iat_patch) {
  HMODULE module = GetModuleHandleA(module_name);
  if (!module) {
    module = LoadLibraryA(module_name);
    if (!module) {
      return  AtlHresultFromLastError();
    }
  }
  return iat_patch->Patch(module, kUrlMonDllName, "CoInternetQueryInfo",
                          reinterpret_cast<LPVOID>(Hook_CoInternetQueryInfo));
}


static HRESULT PatchCoInternetQueryInfo() {
  // TODO(michaeln): look more closely at which modules we want to poke at
  const char *module_names[] = { "ieframe.dll", "mshtml.dll" };
  iat_patch::IATPatchFunction *patches[] = { &ieframe_patch, &mshtml_patch };
  assert(ARRAYSIZE(module_names) == ARRAYSIZE(patches));

  if (!IsIEAtLeastVersion(7, 0, 0, 0)) {
    // The anatomy of IE6 vs IE7 is different enough to require patching
    // a different set of modules that import URLMON. In IE7 and IE8,
    // IEFRAME replaced the older SHDOCVW found in IE6.
    module_names[0] = "shdocvw.dll";
  }

  HRESULT hr = S_OK;
  for (size_t i = 0; i < ARRAYSIZE(patches) && hr == S_OK; ++i) {
    hr = PatchCoInternetQueryInfoForModule(module_names[i], patches[i]);
    LOG16((L"PatchCoInternetQueryInfo(%S) = %d\n", module_names[i], hr));
  }

  if (hr != S_OK) {
    for (size_t i = 0; i < ARRAYSIZE(patches); ++i) {
      if (patches[i]->is_patched()) {
        patches[i]->Unpatch();
      }
    }
  }
  return hr;
}


//------------------------------------------------------------------------------
// VTable patches for the default HTTP and HTTPS protocol handlers. We switched
// to using this technique of intercepting http(s) requests after running into
// insoluable problems with with our previous implementation based on
// an Asyncronous Pluggable Protocol namespace handler.
//------------------------------------------------------------------------------

enum Scheme {
  SCHEME_HTTP,
  SCHEME_HTTPS
};

class HttpVTablePatchesBase {
 public:
  HRESULT Initialize();
  void Uninitialize();

 protected:
  // The interfaces we patch
  enum PatchedInterface {
    IFACE_INTERNET_PROTOCOL_EX = 0,
    IFACE_INTERNET_PROTOCOL_ROOT,
    IFACE_INTERNET_PROTOCOL,
    IFACE_WIN_INET_INFO,
    IFACE_WIN_INET_HTTP_INFO,
    PATCHED_IFACE_COUNT  // used to size an array, keep as last entry
  };

  enum UnknownVTableIndex {
    UNK_QUERY_INTERFACE = 0,
    UNK_ADDREF,
    UNK_RELEASE,
  };

  // IInternetProtocolEx : IInternetProtocol : IInternetProtocolRoot : IUnknown
  enum InternetProtocolRootVTableIndex {
    IPR_START = UNK_RELEASE + 1,
    IPR_CONTINUE,
    IPR_ABORT,
    IPR_TERMINATE,
    IPR_SUSPEND,
    IPR_RESUME,
  };
  enum InternetProtocolVTableIndex {
    IP_READ = IPR_RESUME + 1,
    IP_SEEK,
    IP_LOCK_REQUEST,
    IP_UNLOCK_REQUEST,
  };
  enum InternetProtocolExVTableIndex {
    IPX_START_EX = IP_UNLOCK_REQUEST + 1,
  };

  // IWinInetHttpInfo : IWinInetInfo : IUnknown
  enum IWinInetInfoVTableIndex {
    INFO_QUERY_OPTION = UNK_RELEASE + 1,
  };
  enum IWinInetHttpInfoVTableIndex {
    HINFO_QUERY_INFO = INFO_QUERY_OPTION + 1,
  };

  // Function types from IInternetProtocolEx, IInternetProtocol,
  // IInternetProtocolRoot, IWinInetInfo, IWinInetHttpInfo

  typedef HRESULT (WINAPI* StartEx_Fn)(IInternetProtocolEx *me, IUri *uri,
                                       IInternetProtocolSink *sink,
                                       IInternetBindInfo *bindinfo, DWORD flags,
                                       HANDLE_PTR reserved);
  typedef HRESULT (WINAPI* Start_Fn)(IInternetProtocolRoot *me, LPCWSTR url,
                                     IInternetProtocolSink *sink,
                                     IInternetBindInfo *bindinfo,
                                     DWORD flags, HANDLE_PTR reserved);
  typedef HRESULT (WINAPI* Continue_Fn)(IInternetProtocolRoot *me,
                                        PROTOCOLDATA *pProtocolData);
  typedef HRESULT (WINAPI* Abort_Fn)(IInternetProtocolRoot *me,
                                     HRESULT reason, DWORD options);
  typedef HRESULT (WINAPI* Terminate_Fn)(IInternetProtocolRoot *me,
                                         DWORD options);
  typedef HRESULT (WINAPI* Suspend_Fn)(IInternetProtocolRoot *me);
  typedef HRESULT (WINAPI* Resume_Fn)(IInternetProtocolRoot *me);
  typedef HRESULT (WINAPI* Read_Fn)(IInternetProtocol *me, void *buffer,
                                    ULONG byte_count, ULONG *bytes_read);
  typedef HRESULT (WINAPI* Seek_Fn)(IInternetProtocol *me,
                                    LARGE_INTEGER dlibMove,
                                    DWORD dwOrigin,
                                    ULARGE_INTEGER *plibNewPosition);
  typedef HRESULT (WINAPI* LockRequest_Fn)(IInternetProtocol *me,
                                           DWORD dwOptions);
  typedef HRESULT (WINAPI* UnlockRequest_Fn)(IInternetProtocol *me);
  typedef HRESULT (WINAPI* QueryOption_Fn)(IWinInetInfo *me, DWORD option,
                                           LPVOID buffer, DWORD *buffer_len);
  typedef HRESULT (WINAPI* QueryInfo_Fn)(IWinInetHttpInfo *me, DWORD option,
                                         LPVOID buffer, DWORD *buffer_len,
                                         DWORD *flags, DWORD *reserved);

  virtual Scheme GetScheme() = 0;
  virtual PROC GetHookProc_IInternetProtocolEx(
                  InternetProtocolExVTableIndex method_index) = 0;
  virtual PROC GetHookProc_IInternetProtocolRoot(
                  InternetProtocolRootVTableIndex method_index) = 0;
  virtual PROC GetHookProc_IInternetProtocol(
                  InternetProtocolVTableIndex method_index) = 0;
  virtual PROC GetHookProc_IWinInetInfo(
                  IWinInetInfoVTableIndex method_index) = 0;
  virtual PROC GetHookProc_IWinInetHttpInfo(
                  IWinInetHttpInfoVTableIndex method_index) = 0;

  HRESULT Patch_IInternetProtocolEx(IUnknown *unk);
  HRESULT Patch_IInternetProtocolRoot(IUnknown *unk);
  HRESULT Patch_IInternetProtocol(IUnknown *unk);
  HRESULT Patch_IWinInetInfo(IUnknown *unk);
  HRESULT Patch_IWinInetHttpInfo(IUnknown *unk);

  struct PatchInfo {
    int vtable_index;
    VTablePatch *vtable_patch;
    PROC proc;
  };

  HRESULT PatchInterface(PatchedInterface iface, IUnknown *interface_of_object,
                         const PatchInfo *patches, int num_patches);

  // Used to detect when HTTP and HTTPS use the same vtable
  PROC *vtables_[PATCHED_IFACE_COUNT];

  // IInternetProtocolEx
  VTablePatch startex_patch_;

  // IInternetProtocolRoot
  VTablePatch start_patch_;
  VTablePatch continue_patch_;
  VTablePatch abort_patch_;
  VTablePatch terminate_patch_;
  VTablePatch suspend_patch_;
  VTablePatch resume_patch_;

  // IInternetProtocol
  VTablePatch read_patch_;
  VTablePatch seek_patch_;
  VTablePatch lock_patch_;
  VTablePatch unlock_patch_;

  // IWinInetInfo & IWinInetHttpInfo
  VTablePatch query_option_patch_;
  VTablePatch query_info_patch_;

  // We do not patch these related interfaces.
  // IInternetProtocolInfo - the default handler does not implement this
  // IInternetPriority - calling thru to an unstarted instance behaves
  //                     reasonably (no crashes)
  // IInternetThreadSwitch - calling thru to an unstarted instance behaves
  //                         reasonably (no crashes)
  // IUriContainer - calling thru to an unstarted instance returns an IUri
  //                 instance that reflects the requested url.
  //                 TODO(michaeln): What about redirection?
  // IWinInetCacheHints - calling thru to an unstarted instance behaves
  //                      reasonably (no crashes, returns E_FAIL)
  // IWinInetCacheHints2 - calling thru to an unstarted instance behaves
  //                       reasonably (no crashes, returns E_FAIL)
};


// Template class used to define a distinct set of static Hook_Method and
// Original_Method definitions for HTTP and HTTPS.
template<Scheme kScheme>
class HttpVTablePatches : public HttpVTablePatchesBase {
 public:
  static HttpVTablePatches s_instance;

 private:
  // HttpVTablePatchesBase overrides

  virtual Scheme GetScheme() {
    return kScheme;
  }

  virtual PROC GetHookProc_IInternetProtocolEx(
                  InternetProtocolExVTableIndex method_index) {
    switch(method_index) {
      case IPX_START_EX:
        return reinterpret_cast<PROC>(Hook_StartEx);
      default:
        assert(false);
        return NULL;
    }
  }

  virtual PROC GetHookProc_IInternetProtocolRoot(
                  InternetProtocolRootVTableIndex method_index) {
    switch(method_index) {
      case IPR_START:
        return reinterpret_cast<PROC>(Hook_Start);
      case IPR_CONTINUE:
        return reinterpret_cast<PROC>(Hook_Continue);
      case IPR_ABORT:
        return reinterpret_cast<PROC>(Hook_Abort);
      case IPR_TERMINATE:
        return reinterpret_cast<PROC>(Hook_Terminate);
      case IPR_SUSPEND:
        return reinterpret_cast<PROC>(Hook_Suspend);
      case IPR_RESUME:
        return reinterpret_cast<PROC>(Hook_Resume);
      default:
        assert(false);
        return NULL;
    }
  }

  virtual PROC GetHookProc_IInternetProtocol(
                  InternetProtocolVTableIndex method_index) {
    switch(method_index) {
      case IP_READ:
        return reinterpret_cast<PROC>(Hook_Read);
      case IP_SEEK:
        return reinterpret_cast<PROC>(Hook_Seek);
      case IP_LOCK_REQUEST:
        return reinterpret_cast<PROC>(Hook_LockRequest);
      case IP_UNLOCK_REQUEST:
        return reinterpret_cast<PROC>(Hook_UnlockRequest);
      default:
        assert(false);
        return NULL;
    }
  }

  virtual PROC GetHookProc_IWinInetInfo(
                  IWinInetInfoVTableIndex method_index) {
    switch(method_index) {
      case INFO_QUERY_OPTION:
        return reinterpret_cast<PROC>(Hook_QueryOption);
      default:
        assert(false);
        return NULL;
    }
  }

  virtual PROC GetHookProc_IWinInetHttpInfo(
                  IWinInetHttpInfoVTableIndex method_index) {
    switch(method_index) {
      case HINFO_QUERY_INFO:
        return reinterpret_cast<PROC>(Hook_QueryInfo);
      default:
        assert(false);
        return NULL;
    }
  }

  // Hook function declarations

  static HRESULT WINAPI Hook_StartEx(IInternetProtocolEx *me, IUri *uri,
                                     IInternetProtocolSink *sink,
                                     IInternetBindInfo *bind_info, DWORD flags,
                                     HANDLE_PTR reserved);
  static HRESULT WINAPI Hook_Start(IInternetProtocolRoot *me, LPCWSTR url,
                                   IInternetProtocolSink *sink,
                                   IInternetBindInfo *bind_info,
                                   DWORD flags, HANDLE_PTR reserved);
  static HRESULT WINAPI Hook_Continue(IInternetProtocolRoot *me,
                                      PROTOCOLDATA *pProtocolData);
  static HRESULT WINAPI Hook_Abort(IInternetProtocolRoot *me,
                                   HRESULT reason, DWORD options);
  static HRESULT WINAPI Hook_Terminate(IInternetProtocolRoot *me,
                                       DWORD options);
  static HRESULT WINAPI Hook_Suspend(IInternetProtocolRoot *me);
  static HRESULT WINAPI Hook_Resume(IInternetProtocolRoot *me);
  static HRESULT WINAPI Hook_Read(IInternetProtocol *me, void *buffer,
                                  ULONG byte_count, ULONG *bytes_read);
  static HRESULT WINAPI Hook_Seek(IInternetProtocol *me,
                                  LARGE_INTEGER dlibMove,
                                  DWORD dwOrigin,
                                  ULARGE_INTEGER *plibNewPosition);
  static HRESULT WINAPI Hook_LockRequest(IInternetProtocol *me,
                                         DWORD dwOptions);
  static HRESULT WINAPI Hook_UnlockRequest(IInternetProtocol *me);
  static HRESULT WINAPI Hook_QueryOption(IWinInetInfo *me, DWORD option,
                                         LPVOID buffer, DWORD *buffer_len);
  static HRESULT WINAPI Hook_QueryInfo(IWinInetHttpInfo *me, DWORD option,
                                       LPVOID buffer, DWORD *buffer_len,
                                       DWORD *flags, DWORD *reserved);

  // Helpers to call the original methods in URLMON

  static HRESULT Original_StartEx(IInternetProtocolEx *me, IUri *uri,
                                  IInternetProtocolSink *sink,
                                  IInternetBindInfo *bind_info,
                                  DWORD flags, HANDLE_PTR reserved) {
    StartEx_Fn fn = reinterpret_cast<StartEx_Fn>
                        (s_instance.startex_patch_.original());
    return fn(me, uri, sink, bind_info, flags, reserved);
  }

  static HRESULT Original_Start(IInternetProtocolRoot *me, LPCWSTR url,
                                IInternetProtocolSink *sink,
                                IInternetBindInfo *bind_info,
                                DWORD flags, HANDLE_PTR reserved) {
    Start_Fn fn = reinterpret_cast<Start_Fn>
                      (s_instance.start_patch_.original());
    return fn(me, url, sink, bind_info, flags, reserved);
  }

  static HRESULT Original_Continue(IInternetProtocolRoot *me,
                                   PROTOCOLDATA *pProtocolData) {
    Continue_Fn fn = reinterpret_cast<Continue_Fn>
                        (s_instance.continue_patch_.original());
    return fn(me, pProtocolData);
  }

  static HRESULT Original_Abort(IInternetProtocolRoot *me,
                                HRESULT reason, DWORD options) {
    Abort_Fn fn = reinterpret_cast<Abort_Fn>
                      (s_instance.abort_patch_.original());
    return fn(me, reason, options);
  }

  static HRESULT Original_Terminate(IInternetProtocolRoot *me, DWORD options) {
    Terminate_Fn fn = reinterpret_cast<Terminate_Fn>
                          (s_instance.terminate_patch_.original());
    return fn(me, options);
  }

  static HRESULT Original_Suspend(IInternetProtocolRoot *me) {
    Suspend_Fn fn = reinterpret_cast<Suspend_Fn>
                        (s_instance.suspend_patch_.original());
    return fn(me);
  }

  static HRESULT Original_Resume(IInternetProtocolRoot *me) {
    Resume_Fn fn = reinterpret_cast<Resume_Fn>
                      (s_instance.resume_patch_.original());
    return fn(me);
  }

  static HRESULT Original_Read(IInternetProtocol *me, void *buffer,
                               ULONG buffer_len, ULONG *amount_read) {
    Read_Fn fn = reinterpret_cast<Read_Fn>
                    (s_instance.read_patch_.original());
    return fn(me, buffer, buffer_len, amount_read);
  }

  static HRESULT Original_Seek(IInternetProtocol *me,
                               LARGE_INTEGER dlibMove,
                               DWORD dwOrigin,
                               ULARGE_INTEGER *plibNewPosition) {
    Seek_Fn fn = reinterpret_cast<Seek_Fn>
                    (s_instance.seek_patch_.original());
    return fn(me, dlibMove, dwOrigin, plibNewPosition);
  }

  static HRESULT Original_LockRequest(IInternetProtocol *me, DWORD dwOptions) {
    LockRequest_Fn fn = reinterpret_cast<LockRequest_Fn>
                            (s_instance.lock_patch_.original());
    return fn(me, dwOptions);
  }

  static HRESULT Original_UnlockRequest(IInternetProtocol *me) {
    UnlockRequest_Fn fn = reinterpret_cast<UnlockRequest_Fn>
                              (s_instance.unlock_patch_.original());
    return fn(me);
  }

  static HRESULT Original_QueryOption(IWinInetInfo *me, DWORD option,
                                      LPVOID buffer, DWORD *buffer_len) {
    QueryOption_Fn fn = reinterpret_cast<QueryOption_Fn>
                            (s_instance.query_option_patch_.original());
    return fn(me, option, buffer, buffer_len);
  }

  static HRESULT Original_QueryInfo(IWinInetHttpInfo *me, DWORD option,
                                    LPVOID buffer, DWORD *buffer_len,
                                    DWORD *flags, DWORD *reserved) {
    QueryInfo_Fn fn = reinterpret_cast<QueryInfo_Fn>
                          (s_instance.query_info_patch_.original());
    return fn(me, option, buffer, buffer_len, flags, reserved);
  }
};


HRESULT InitializeHttpPatches() {
  LOG16((L"InitializeHttpPatches\n"));
  HRESULT hr = PatchCoInternetQueryInfo();
  if (FAILED(hr)) {
    assert(false);
    return hr;
  }

  // Initialize the HTTP patches first, the template class for HTTPS will
  // detect duplicate vtables for HTTP and HTTPS and refrain from repatching.
  hr = HttpVTablePatches<SCHEME_HTTP>::s_instance.Initialize();
  if (FAILED(hr)) {
    return hr;
  }
  return HttpVTablePatches<SCHEME_HTTPS>::s_instance.Initialize();
}


HRESULT HttpVTablePatchesBase::Initialize() {
  // Which CLSID we use is a function of which scheme we're patching
  CLSID clsid;
  switch (GetScheme()) {
    case SCHEME_HTTP:
      clsid = CLSID_HttpProtocol;
      break;
    case SCHEME_HTTPS:
      clsid = CLSID_HttpSProtocol;
      break;
    default:
      assert(false);
      return E_FAIL;
  }

  // Create an instance of the default handler in URLMON
  HRESULT hr = S_OK;
  HMODULE module = ::GetModuleHandleA(kUrlMonDllName);
  if (module == NULL) {
    module = LoadLibraryA(kUrlMonDllName);
    if (!module) {
      hr = AtlHresultFromLastError();
    }
  }
  assert(SUCCEEDED(hr) || module == NULL);
  assert(FAILED(hr) || module != NULL);
  if (FAILED(hr)) {
    assert(false);
    return hr;
  }

  typedef HRESULT (WINAPI* FN_DllGetClassObject)(REFCLSID, REFIID, LPVOID*);
  FN_DllGetClassObject fn = reinterpret_cast<FN_DllGetClassObject>(
                                ::GetProcAddress(module, "DllGetClassObject"));
  if (!fn) {
   assert(fn != NULL);
   return E_UNEXPECTED;
  }
  CComPtr<IClassFactory> cf;
  hr = fn(clsid, IID_IClassFactory, reinterpret_cast<LPVOID*>(&cf));
  if (FAILED(hr)) {
    assert(false);
    return hr;
  }
  CComPtr<IInternetProtocol> default_handler_instance;
  hr = cf->CreateInstance(
               NULL, IID_IInternetProtocol,
               reinterpret_cast<void**>(&default_handler_instance));
  assert(default_handler_instance != NULL || FAILED(hr));
  if (FAILED(hr)) {
    assert(false);
    return hr;
  }

  // Apply patches to the default handler
  if (FAILED(hr = Patch_IInternetProtocolEx(default_handler_instance)) ||
      FAILED(hr = Patch_IInternetProtocol(default_handler_instance)) ||
      FAILED(hr = Patch_IInternetProtocolRoot(default_handler_instance)) ||
      FAILED(hr = Patch_IWinInetInfo(default_handler_instance)) ||
      FAILED(hr = Patch_IWinInetHttpInfo(default_handler_instance))) {
    assert(false);
    Uninitialize();
  }
  LOG16((L"HttpVTablePatches<kScheme>::Initialize - %d\n", hr));
  return hr;
}


// Helper that applies several patches to an interface.
HRESULT HttpVTablePatchesBase::PatchInterface(PatchedInterface iface,
                                              IUnknown *interface_of_object,
                                              const PatchInfo *patches,
                                              int num_patches) {
  // We compare the vtables of the HTTP and HTTPS handlers and if found to be
  // one and the same, we don't patch it a second time. Some of the vtables
  // are shared and others are not.
  vtables_[iface] = *reinterpret_cast<PROC**>(interface_of_object);
  if ((GetScheme() == SCHEME_HTTPS) &&
      (vtables_[iface] ==
       HttpVTablePatches<SCHEME_HTTP>::s_instance.vtables_[iface])) {
    LOG16((L"HttpVTablePatches::PatchInterface duplicate vtable for iface %d\n",
           iface));
    return S_OK;
  }

  HRESULT hr = S_OK;
  for (int i = 0; i < num_patches && hr == S_OK; ++i) {
    hr = patches[i].vtable_patch->Patch(interface_of_object,
                                        patches[i].vtable_index,
                                        patches[i].proc);
  }
  return hr;
}


HRESULT HttpVTablePatchesBase::Patch_IInternetProtocolEx(IUnknown *unknown) {
  const PatchInfo patches[] = {
    { IPX_START_EX, &startex_patch_,
      GetHookProc_IInternetProtocolEx(IPX_START_EX) }
  };
  CComQIPtr<IInternetProtocolEx> interface_of_object(unknown);
  if (!interface_of_object) {
    LOG16((L"IInternetProtocolEx not found on default handler\n"));
    return S_OK;  // Interface is not implemented on older systems
  }
  return PatchInterface(IFACE_INTERNET_PROTOCOL_EX,
                        interface_of_object, patches, ARRAYSIZE(patches));
}


HRESULT HttpVTablePatchesBase::Patch_IInternetProtocol(IUnknown *unknown) {
  const PatchInfo patches[] = {
    { IP_READ, &read_patch_,
      GetHookProc_IInternetProtocol(IP_READ) },
    { IP_SEEK, &seek_patch_,
      GetHookProc_IInternetProtocol(IP_SEEK) },
    { IP_LOCK_REQUEST, &lock_patch_,
      GetHookProc_IInternetProtocol(IP_LOCK_REQUEST) },
    { IP_UNLOCK_REQUEST, &unlock_patch_,
      GetHookProc_IInternetProtocol(IP_UNLOCK_REQUEST) }
  };
  CComQIPtr<IInternetProtocol> interface_of_object(unknown);
  if (!interface_of_object) {
    return E_FAIL;
  }
  return PatchInterface(IFACE_INTERNET_PROTOCOL,
                        interface_of_object, patches, ARRAYSIZE(patches));
}


HRESULT HttpVTablePatchesBase::Patch_IInternetProtocolRoot(IUnknown *unknown) {
  const PatchInfo patches[] = {
    { IPR_START, &start_patch_,
      GetHookProc_IInternetProtocolRoot(IPR_START) },
    { IPR_CONTINUE, &continue_patch_,
      GetHookProc_IInternetProtocolRoot(IPR_CONTINUE) },
    { IPR_ABORT, &abort_patch_,
      GetHookProc_IInternetProtocolRoot(IPR_ABORT) },
    { IPR_TERMINATE, &terminate_patch_,
      GetHookProc_IInternetProtocolRoot(IPR_TERMINATE) },
    { IPR_SUSPEND, &suspend_patch_,
      GetHookProc_IInternetProtocolRoot(IPR_SUSPEND) },
    { IPR_RESUME, &resume_patch_,
      GetHookProc_IInternetProtocolRoot(IPR_RESUME) }
  };
  CComQIPtr<IInternetProtocolRoot> interface_of_object(unknown);
  if (!interface_of_object) {
    return E_FAIL;
  }
  return PatchInterface(IFACE_INTERNET_PROTOCOL_ROOT,
                        interface_of_object, patches, ARRAYSIZE(patches));
}


HRESULT HttpVTablePatchesBase::Patch_IWinInetHttpInfo(IUnknown *unknown) {
  const PatchInfo patches[] = {
    { HINFO_QUERY_INFO, &query_info_patch_,
      GetHookProc_IWinInetHttpInfo(HINFO_QUERY_INFO) }
  };
  CComQIPtr<IWinInetHttpInfo> interface_of_object(unknown);
  if (!interface_of_object) {
    return E_FAIL;
  }
  return PatchInterface(IFACE_WIN_INET_HTTP_INFO,
                        interface_of_object, patches, ARRAYSIZE(patches));
}


HRESULT HttpVTablePatchesBase::Patch_IWinInetInfo(IUnknown *unknown) {
  const PatchInfo patches[] = {
    { INFO_QUERY_OPTION, &query_option_patch_,
      GetHookProc_IWinInetInfo(INFO_QUERY_OPTION)}
  };
  CComQIPtr<IWinInetInfo> interface_of_object(unknown);
  if (!interface_of_object) {
    return E_FAIL;
  }
  return PatchInterface(IFACE_WIN_INET_INFO,
                        interface_of_object, patches, ARRAYSIZE(patches));
}


void HttpVTablePatchesBase::Uninitialize() {
  startex_patch_.Unpatch();
  start_patch_.Unpatch();
  continue_patch_.Unpatch();
  abort_patch_.Unpatch();
  terminate_patch_.Unpatch();
  suspend_patch_.Unpatch();
  resume_patch_.Unpatch();
  read_patch_.Unpatch();
  seek_patch_.Unpatch();
  lock_patch_.Unpatch();
  unlock_patch_.Unpatch();
  query_option_patch_.Unpatch();
  query_info_patch_.Unpatch();
}


template<Scheme kScheme>
HttpVTablePatches<kScheme> HttpVTablePatches<kScheme>::s_instance;


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_StartEx(
                 IInternetProtocolEx *me, IUri *uri,
                 IInternetProtocolSink *sink,
                 IInternetBindInfo *bind_info,
                 DWORD flags, HANDLE_PTR reserved) {
  CComPtr<IInternetProtocolSink> sink_replacement;
  if (!HttpHandlerPatch::Find(me)) {
    scoped_refptr<HttpHandlerPatch> handler(new HttpHandlerPatch(me));
    HRESULT hr = handler->StartExMaybe(uri, sink, bind_info, flags, reserved,
                                       &sink_replacement);
    if (hr != INET_E_USE_DEFAULT_PROTOCOLHANDLER) {
      return hr;
    }
    if (sink_replacement) {
      sink = sink_replacement;
    }
  } else {
    assert(false);
    return E_FAIL;
  }
  return Original_StartEx(me, uri, sink, bind_info, flags, reserved);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_Start(
                 IInternetProtocolRoot *me, LPCWSTR url,
                 IInternetProtocolSink *sink,
                 IInternetBindInfo *bind_info,
                 DWORD flags, HANDLE_PTR reserved) {
  CComPtr<IInternetProtocolSink> sink_replacement;
  if (!HttpHandlerPatch::Find(me)) {
    scoped_refptr<HttpHandlerPatch> handler(new HttpHandlerPatch(me));
    HRESULT hr = handler->StartMaybe(url, sink, bind_info, flags, reserved,
                                     &sink_replacement);
    if (hr != INET_E_USE_DEFAULT_PROTOCOLHANDLER) {
      return hr;
    }
    if (sink_replacement) {
      sink = sink_replacement;
    }
  } else {
    assert(false);
    return E_FAIL;
  }
  return Original_Start(me, url, sink, bind_info, flags, reserved);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_Continue(
                                                  IInternetProtocolRoot *me,
                                                  PROTOCOLDATA *pProtocolData) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->Continue(pProtocolData);
  }
  return Original_Continue(me, pProtocolData);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_Abort(IInternetProtocolRoot *me,
                                                    HRESULT reason,
                                                    DWORD options) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->Abort(reason, options);
  }
  return Original_Abort(me, reason, options);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_Terminate(
                                                  IInternetProtocolRoot *me,
                                                  DWORD dwOptions) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->Terminate(dwOptions);
  }
  return Original_Terminate(me, dwOptions);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_Suspend(
                                                  IInternetProtocolRoot *me) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->Suspend();
  }
  return Original_Suspend(me);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_Resume(
                                                  IInternetProtocolRoot *me) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->Resume();
  }
  return Original_Resume(me);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_Read(IInternetProtocol *me,
                                                   void *buffer,
                                                   ULONG byte_count,
                                                   ULONG *bytes_read) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->Read(buffer, byte_count, bytes_read);
  }
  return Original_Read(me, buffer, byte_count, bytes_read);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_Seek(IInternetProtocol *me,
                                                   LARGE_INTEGER move,
                                                   DWORD origin,
                                                   ULARGE_INTEGER *position) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->Seek(move, origin, position);
  }
  return Original_Seek(me, move, origin, position);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_LockRequest(IInternetProtocol *me,
                                                          DWORD dwOptions) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->LockRequest(dwOptions);
  }
  return Original_LockRequest(me, dwOptions);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_UnlockRequest(
                                                  IInternetProtocol *me) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->UnlockRequest();
  }
  return Original_UnlockRequest(me);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_QueryOption(IWinInetInfo *me,
                                                          DWORD option,
                                                          LPVOID buffer,
                                                          DWORD *buffer_len) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->QueryOption(option, buffer, buffer_len);
  }
  return Original_QueryOption(me, option, buffer, buffer_len);
}


template<Scheme kScheme>
STDMETHODIMP HttpVTablePatches<kScheme>::Hook_QueryInfo(IWinInetHttpInfo *me,
                                                        DWORD option,
                                                        LPVOID buffer,
                                                        DWORD *buffer_len,
                                                        DWORD *flags,
                                                        DWORD *reserved) {
  HttpHandlerPatch *handler = HttpHandlerPatch::Find(me);
  if (handler) {
    return handler->QueryInfo(option, buffer, buffer_len, flags, reserved);
  }
  return Original_QueryInfo(me, option, buffer, buffer_len, flags, reserved);
}
