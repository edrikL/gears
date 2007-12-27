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
// In IE, JsRunner is a wrapper and most work happens in
// JsRunnerImpl.  The split is necessary so we can do two things:
// (1) provide a simple NewJsRunner/delete interface (not ref-counting), plus
// (2) derive from ATL for the internal implementation (e.g. for ScriptSiteImpl)
//
// JAVASCRIPT ENGINE DETAILS: Internet Explorer uses the JScript engine.
// The interface is IActiveScript, a shared scripting engine interface.
// Communication from the engine to external objects, and communication
// from externally into the engine, is all handled via IDispatch pointers.

#include <assert.h>
#include <dispex.h>
#include <map>
#include <set>
#include <vector>
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#include "gears/base/common/js_runner.h"

#include "gears/base/common/common.h"  // for DISALLOW_EVIL_CONSTRUCTORS
#include "gears/base/common/exception_handler_win32.h"
#include "gears/base/common/html_event_monitor.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"
#include "gears/third_party/AtlActiveScriptSite.h"


// Internal base class used to share some code between DocumentJsRunner and
// JsRunner. Do not override these methods from JsRunner or DocumentJsRunner.
// Either share the code here, or move it to those two classes if it's
// different.
class JsRunnerBase : public JsRunnerInterface {
 public:
  JsRunnerBase() {}

  JsContextPtr GetContext() {
    return NULL;  // not used in IE
  }

  JsObject *NewObject(const char16 *optional_global_ctor_name,
                      bool dump_on_error = false) {
    CComPtr<IDispatch> global_object = GetGlobalObject(dump_on_error);
    if (!global_object) {
      LOG(("Could not get global object from script engine."));
      return NULL;
    }

    CComQIPtr<IDispatchEx> global_dispex = global_object;
    if (!global_dispex) {
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      return NULL;
    }

    DISPID error_dispid;
    CComBSTR ctor_name(optional_global_ctor_name ? optional_global_ctor_name :
                       STRING16(L"Object"));
    HRESULT hr = global_dispex->GetDispID(ctor_name, fdexNameCaseSensitive,
                                          &error_dispid);
    if (FAILED(hr)) {
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      return NULL;
    }

    CComVariant result;
    DISPPARAMS no_args = {NULL, NULL, 0, 0};
    hr = global_dispex->InvokeEx(
                            error_dispid, LOCALE_USER_DEFAULT,
                            DISPATCH_CONSTRUCT, &no_args, &result,
                            NULL,  // receives exception (NULL okay)
                            NULL);  // pointer to "this" object (NULL okay)
    if (FAILED(hr)) {
      LOG(("Could not invoke object constructor."));
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      return NULL;
    }

    scoped_ptr<JsObject> retval(new JsObject);
    if (!retval->SetObject(result, GetContext())) {
      LOG(("Could not assign to JsObject."));
      return NULL;
    }

    return retval.release();
  }

  bool SetPropertyString(JsObject *object, const char16 *name,
                         const char16 *value) {
    return SetObjectProperty(object, name, CComVariant(CComBSTR(value)));
  }

  bool SetPropertyInt(JsObject *object, const char16 *name, int value) {
    return SetObjectProperty(object, name, CComVariant(value));
  }

  bool InvokeCallback(const JsRootedCallback *callback,
                      int argc, JsParamToSend *argv,
                      JsRootedToken **optional_alloc_retval) {
    assert(callback && (!argc || argv));
    if (callback->token().vt != VT_DISPATCH) { return false; }

    // Setup argument array.
    scoped_array<CComVariant> js_engine_argv(new CComVariant[argc]);
    for (int i = 0; i < argc; ++i) {
      int dest = argc - 1 - i;  // args are expected in reverse order!!
      ConvertJsParamToToken(argv[i], NULL, &js_engine_argv[dest]);
    }

    // Invoke the method.
    DISPPARAMS invoke_params = {0};
    invoke_params.cArgs = argc;
    invoke_params.rgvarg = js_engine_argv.get();

    VARIANT retval = {0};
    HRESULT hr = callback->token().pdispVal->Invoke(
        DISPID_VALUE, IID_NULL,  // DISPID_VALUE = default action
        LOCALE_SYSTEM_DEFAULT,   // TODO(cprince): should this be user default?
        DISPATCH_METHOD,         // dispatch/invoke as...
        &invoke_params,          // parameters
        optional_alloc_retval ? &retval : NULL,  // receives result (NULL okay)
        NULL,                    // receives exception (NULL okay)
        NULL);                   // receives badarg index (NULL okay)
    if (FAILED(hr)) { return false; }

    if (optional_alloc_retval) {
      // Note: A valid VARIANT is returned no matter what the js function
      // returns. If it returns nothing, or explicitly returns <undefined>, the
      // variant will contain VT_EMPTY. If it returns <null>, the variant will
      // contain VT_NULL. Always returning a JsRootedToken should allow us to
      // coerce these values to other types correctly in the future.
      *optional_alloc_retval = new JsRootedToken(NULL, retval);
    }

    return true;
  }

  // Add the provided handler to the notification list for the specified event.
  virtual bool AddEventHandler(JsEventType event_type,
                               JsEventHandlerInterface *handler) {
    assert(event_type >= 0 && event_type < MAX_JSEVENTS);

    event_handlers_[event_type].insert(handler);
    return true;
  }

  // Remove the provided handler from the notification list for the specified
  // event.
  virtual bool RemoveEventHandler(JsEventType event_type,
                                  JsEventHandlerInterface *handler) {
    assert(event_type >= 0 && event_type < MAX_JSEVENTS);

    event_handlers_[event_type].erase(handler);
    return true;
  }

#ifdef DEBUG
  void ForceGC() {
    // CollectGarbage is not available through COM, so we call through JS.
    if (!Eval(STRING16(L"CollectGarbage();"))) {
      assert(false);
    }
  }
#endif

 protected:
  // Alert all monitors that an event has occured.
  void SendEvent(JsEventType event_type) {
    assert(event_type >= 0 && event_type < MAX_JSEVENTS);

    // Make a copy of the list of listeners, in case they change during the
    // alert phase.
    std::vector<JsEventHandlerInterface *> monitors;
    monitors.insert(monitors.end(),
                    event_handlers_[event_type].begin(),
                    event_handlers_[event_type].end());

    std::vector<JsEventHandlerInterface *>::iterator monitor;
    for (monitor = monitors.begin();
         monitor != monitors.end();
         ++monitor) {
      // Check that the listener hasn't been removed.  This can occur if a
      // listener removes another listener from the list.
      if (event_handlers_[event_type].find(*monitor) !=
                                     event_handlers_[event_type].end()) {
        (*monitor)->HandleEvent(event_type);
      }
    }
  }

  // TODO(zork): Remove the argument when we find the error.
  virtual IDispatch *GetGlobalObject(bool dump_on_error = false) = 0;

 private:
  bool SetObjectProperty(JsObject *object, const char16 *name,
                         const VARIANT &value) {
    return SetProperty(object->js_object_, name, value);
  }

  bool SetProperty(JsToken object, const char16 *name, const VARIANT &value) {
    if (object.vt != VT_DISPATCH) { return false; }

    CComQIPtr<IDispatchEx> dispatchex = object.pdispVal;
    if (!dispatchex) { return false; }

    DISPID dispid;
    HRESULT hr = dispatchex->GetDispID(CComBSTR(name),
                                       fdexNameCaseSensitive | fdexNameEnsure,
                                       &dispid);
    if (FAILED(hr)) { return false; }

    DISPPARAMS params = {NULL, NULL, 1, 1};
    params.rgvarg = const_cast<VARIANT *>(&value);
    DISPID dispid_put = DISPID_PROPERTYPUT;
    params.rgdispidNamedArgs = &dispid_put;

    hr = object.pdispVal->Invoke(dispid, IID_NULL,
                                 LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
                                 &params, NULL, NULL, NULL);
    if (FAILED(hr)) { return false; }

    return true;
  }

  std::set<JsEventHandlerInterface *> event_handlers_[MAX_JSEVENTS];

  DISALLOW_EVIL_CONSTRUCTORS(JsRunnerBase);
};


// Internal helper COM object that implements IActiveScriptSite so we can host
// new ActiveScript engine instances. This class does the majority of the work
// of the real JsRunner.
class ATL_NO_VTABLE JsRunnerImpl
    : public CComObjectRootEx<CComMultiThreadModel>,
      public IDispatchImpl<IDispatch>,
      public IActiveScriptSiteImpl,
      public IInternetHostSecurityManager,
      public IServiceProviderImpl<JsRunnerImpl> {
 public:
  BEGIN_COM_MAP(JsRunnerImpl)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IActiveScriptSite)
    COM_INTERFACE_ENTRY(IInternetHostSecurityManager)
    COM_INTERFACE_ENTRY(IServiceProvider)
  END_COM_MAP()

  // For IServiceProviderImpl (used to disable ActiveX objects, along with
  // IInternetHostSecurityManager).
  BEGIN_SERVICE_MAP(JsRunnerImpl)
    SERVICE_ENTRY(SID_SInternetHostSecurityManager)
  END_SERVICE_MAP()

  JsRunnerImpl() : coinit_succeeded_(false) {}
  ~JsRunnerImpl();

  // JsRunnerBase implementation
  IDispatch *GetGlobalObject(bool dump_on_error = false) {
    IDispatch *global_object;
    HRESULT hr = javascript_engine_->GetScriptDispatch(
                                         NULL,  // get global object
                                         &global_object);
    if (FAILED(hr)) {
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      return NULL;
    }
    return global_object;
  }

  // JsRunner implementation
  bool AddGlobal(const std::string16 &name, IGeneric *object, gIID iface_id);
  bool Start(const std::string16 &full_script);
  bool Stop();
  bool Eval(const std::string16 &script);
  void SetErrorHandler(JsErrorHandlerInterface *error_handler) {
    error_handler_ = error_handler;
  }

  // IActiveScriptSiteImpl overrides
  STDMETHOD(LookupNamedItem)(const OLECHAR *name, IUnknown **item);
  STDMETHOD(HandleScriptError)(EXCEPINFO *ei, ULONG line, LONG pos, BSTR src);

  // Implement IInternetHostSecurityManager. We use this to prevent the script
  // engine from creating ActiveX objects, using Java, using scriptlets and
  // various other questionable activities.
  STDMETHOD(GetSecurityId)(BYTE *securityId, DWORD *securityIdSize,
                           DWORD_PTR reserved);
  STDMETHOD(ProcessUrlAction)(DWORD action, BYTE *policy, DWORD policy_size,
                              BYTE *context, DWORD context_size, DWORD flags,
                              DWORD reserved);
  STDMETHOD(QueryCustomPolicy)(REFGUID guid_key, BYTE **policy,
                               DWORD *policy_size, BYTE *context,
                               DWORD context_size, DWORD reserved);

 private:
  CComPtr<IActiveScript> javascript_engine_;
  JsErrorHandlerInterface *error_handler_;

  typedef std::map<std::string16, IGeneric *> NameToObjectMap;
  NameToObjectMap global_name_to_object_;

  bool coinit_succeeded_;

  DISALLOW_EVIL_CONSTRUCTORS(JsRunnerImpl);
};


JsRunnerImpl::~JsRunnerImpl() {
  // decrement all refcounts incremented by AddGlobal()
  JsRunnerImpl::NameToObjectMap::const_iterator it;
  const JsRunnerImpl::NameToObjectMap &map = global_name_to_object_;
  for (it = map.begin(); it != map.end(); ++it) {
    it->second->Release();
  }

  if (coinit_succeeded_) {
    CoUninitialize();
  }
}


bool JsRunnerImpl::AddGlobal(const std::string16 &name,
                             IGeneric *object,
                             gIID iface_id) {
  if (!object) { return false; }

  // We AddRef() to make sure the object lives as long as the JS engine.
  // This gets removed in ~JsRunnerImpl.
  object->AddRef();
  global_name_to_object_[name] = object;

  // Start() will insert any globals added before the JS engine started.
  // But we must call AddNamedItem() here if the JS engine is already running.
  if (javascript_engine_) {
    HRESULT hr = javascript_engine_->AddNamedItem(name.c_str(),
                                                  SCRIPTITEM_ISVISIBLE);
    if (FAILED(hr)) { return false; }
  }

  return true;
}


bool JsRunnerImpl::Start(const std::string16 &full_script) {
  HRESULT hr;

  coinit_succeeded_ = SUCCEEDED(CoInitializeEx(NULL,
                                               GEARS_COINIT_THREAD_MODEL));
  if (!coinit_succeeded_) { return false; }
  // CoUninitialize is handled in dtor

  //
  // Instantiate a JavaScript engine
  //

  CLSID id;
  hr = CLSIDFromProgID(L"JScript", &id);
  if (FAILED(hr)) { return false; }

  hr = javascript_engine_.CoCreateInstance(id);
  if (FAILED(hr)) { return false; }

  // Set the engine's site (which the engine queries when it encounters
  // an unknown name).
  hr = javascript_engine_->SetScriptSite(this);
  if (FAILED(hr)) { return false; }


  // Tell the script engine up to use a custom security manager implementation.
  CComQIPtr<IObjectSafety> javascript_engine_safety;
  javascript_engine_safety = javascript_engine_;
  if (!javascript_engine_safety) { return false; }

  hr = javascript_engine_safety->SetInterfaceSafetyOptions(
                                     IID_IDispatch,
                                     INTERFACE_USES_SECURITY_MANAGER,
                                     INTERFACE_USES_SECURITY_MANAGER);
  if (FAILED(hr)) { return false; }


  //
  // Tell the engine about named globals (set earlier via AddGlobal).
  //

  JsRunnerImpl::NameToObjectMap::const_iterator it;
  const JsRunnerImpl::NameToObjectMap &map = global_name_to_object_;
  for (it = map.begin(); it != map.end(); ++it) {
    const std::string16 &name = it->first;
    hr = javascript_engine_->AddNamedItem(name.c_str(), SCRIPTITEM_ISVISIBLE);
    // TODO(cprince): see if need |= SCRIPTITEM_GLOBALMEMBERS
    if (FAILED(hr)) { return false; }
  }


  //
  // Add script code to the engine instance
  //

  CComQIPtr<IActiveScriptParse> javascript_engine_parser;

  javascript_engine_parser = javascript_engine_;
  if (!javascript_engine_parser) { return false; }

  hr = javascript_engine_parser->InitNew();
  if (FAILED(hr)) { return false; }
  // why does ParseScriptText also AddRef the object?
  hr = javascript_engine_parser->ParseScriptText(full_script.c_str(),
                                                 NULL, NULL, NULL, 0, 0,
                                                 SCRIPTITEM_ISVISIBLE,
                                                 NULL, NULL);
  if (FAILED(hr)) { return false; }

  //
  // Start the engine running
  //

  // TODO(cprince): do we need STARTED *and* CONNECTED? (Does it matter?)
  // CONNECTED "runs the script and blocks until the script is finished"
  hr = javascript_engine_->SetScriptState(SCRIPTSTATE_STARTED);
  if (FAILED(hr)) { return false; }
  hr = javascript_engine_->SetScriptState(SCRIPTSTATE_CONNECTED);
  if (FAILED(hr)) { return false; }

  // NOTE: at this point, the JS code has returned, and it should have set
  // an onmessage handler.  (If it didn't, the worker is most likely cut off
  // from other workers.  There are ways to prevent this, but they are poor.)

  return true;  // succeeded
}

bool JsRunnerImpl::Eval(const std::string16 &script) {
  CComQIPtr<IActiveScriptParse> javascript_engine_parser;

  // Get the parser interface
  javascript_engine_parser = javascript_engine_;
  if (!javascript_engine_parser) { return false; }

  // Execute the script
  HRESULT hr = javascript_engine_parser->ParseScriptText(script.c_str(),
                                                         NULL, NULL, 0, 0, 0,
                                                         SCRIPTITEM_ISVISIBLE,
                                                         NULL, NULL);
  if (FAILED(hr)) { return false; }
  return true;
}

bool JsRunnerImpl::Stop() {
  // Check pointer because dtor calls Stop() regardless of whether Start()
  // happened.
  if (javascript_engine_) {
    javascript_engine_->Close();
  }
  return S_OK;
}

STDMETHODIMP JsRunnerImpl::LookupNamedItem(const OLECHAR *name,
                                           IUnknown **item) {
  IGeneric *found_item = global_name_to_object_[name];
  if (!found_item) { return TYPE_E_ELEMENTNOTFOUND; }

  // IActiveScript expects items coming into it to already be AddRef()'d. It
  // will Release() these references on IActiveScript.Close().
  // For an example of this, see: http://support.microsoft.com/kb/q221992/.
  found_item->AddRef();
  *item = found_item;
  return S_OK;
}


STDMETHODIMP JsRunnerImpl::HandleScriptError(EXCEPINFO *ei, ULONG line,
                                             LONG pos, BSTR src) {
  if (!error_handler_) { return E_FAIL; }

  const JsErrorInfo error_info = {
    line + 1,  // Reported lines start at zero.
    static_cast<char16 *>(ei->bstrDescription)
  };

  error_handler_->HandleError(error_info);
  return S_OK;
}


STDMETHODIMP JsRunnerImpl::ProcessUrlAction(DWORD action, BYTE *policy,
                                            DWORD policy_size, BYTE *context,
                                            DWORD context_size, DWORD flags,
                                            DWORD reserved) {
  // There are many different values of action that could conceivably be
  // asked about: http://msdn2.microsoft.com/en-us/library/ms537178.aspx.
  // Many of them probably don't apply to the scripting host alone, but there
  // is no documentation on which are used, so we just say no to everything to
  // be paranoid. We can whitelist things back if we find that necessary.
  //
  // TODO(aa): Consider whitelisting XMLHttpRequest. IE7 now has a global
  // XMLHttpRequest object, like Safari and Mozilla. I don't believe this
  // object "counts" as an ActiveX Control. If so, it seems like whitelisting
  // the ActiveX version might not matter. In any case, doing this would
  // require figuring out the parallel thing for Mozilla and figuring out how
  // to get XMLHttpRequest the context it needs to make decisions about same-
  // origin.
  *policy = URLPOLICY_DISALLOW;

  // MSDN says to return S_FALSE if you were successful, but don't want to
  // allow the action: http://msdn2.microsoft.com/en-us/library/ms537151.aspx.
  return S_FALSE;
}


STDMETHODIMP JsRunnerImpl::QueryCustomPolicy(REFGUID guid_key, BYTE **policy,
                                             DWORD *policy_size, BYTE *context,
                                             DWORD context_size,
                                             DWORD reserved) {
  // This method is only used when ProcessUrlAction allows ActiveXObjects to be
  // created. Since we always say no, we do not need to implement this method.
  return E_NOTIMPL;
}


STDMETHODIMP JsRunnerImpl::GetSecurityId(BYTE *security_id,
                                         DWORD *security_id_size,
                                         DWORD_PTR reserved) {
  // Again, not used in the configuration we use.
  return E_NOTIMPL;
}


// A wrapper class to AddRef/Release the internal COM object,
// while exposing a simpler new/delete interface to users.
class JsRunner : public JsRunnerBase {
 public:
  JsRunner() {
    HRESULT hr = CComObject<JsRunnerImpl>::CreateInstance(&com_obj_);
    if (com_obj_) {
      com_obj_->AddRef();  // MSDN says call AddRef after CreateInstance
    }
  }
  virtual ~JsRunner() {
    // Alert modules that the engine is unloading.
    SendEvent(JSEVENT_UNLOAD);

    if (com_obj_) {
      com_obj_->Stop();
      com_obj_->Release();
    }
  }

  IDispatch *GetGlobalObject(bool dump_on_error = false) {
    return com_obj_->GetGlobalObject(dump_on_error);
  }
  bool AddGlobal(const std::string16 &name, IGeneric *object, gIID iface_id) {
    return com_obj_->AddGlobal(name, object, iface_id);
  }
  bool Start(const std::string16 &full_script) {
    return com_obj_->Start(full_script);
  }
  bool Stop() {
    return com_obj_->Stop();
  }
  bool Eval(const std::string16 &script) {
    return com_obj_->Eval(script);
  }
  void SetErrorHandler(JsErrorHandlerInterface *error_handler) {
    return com_obj_->SetErrorHandler(error_handler);
  }

 private:
  CComObject<JsRunnerImpl> *com_obj_;

  DISALLOW_EVIL_CONSTRUCTORS(JsRunner);
};


// This class is a stub that is used to present a uniform interface to
// common functionality to both workers and the main thread.
class DocumentJsRunner : public JsRunnerBase {
 public:
  DocumentJsRunner(IGeneric *site) : site_(site) {
  }

  virtual ~DocumentJsRunner() {
  }

  IDispatch *GetGlobalObject(bool dump_on_error = false) {
    IDispatch *global_object;
    HRESULT hr = ActiveXUtils::GetScriptDispatch(site_, &global_object,
                                                 dump_on_error);
    if (FAILED(hr)) {
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      return NULL;
    }
    return global_object;
  }

  bool AddGlobal(const std::string16 &name, IGeneric *object, gIID iface_id) {
    // TODO(zork): Add this functionality to DocumentJsRunner.
    return false;
  }

  bool Start(const std::string16 &full_script) {
    assert(false);  // Should not be called on the DocumentJsRunner.
    return false;
  }

  bool Stop() {
    assert(false);  // Should not be called on the DocumentJsRunner.
    return false;
  }

  bool Eval(const std::string16 &script) {
    // There appears to be no way to execute script code directly on WinCE.
    // - IPIEHTMLWindow2 does not provide execScript.
    // - We have the script engine's IDispatch pointer but there's no way to get
    //   back to the IActiveScript object to use
    //   IActiveScriptParse::ParseScriptText.
    // Instead, we pass the script as a parameter to JavaScript's 'eval'
    // function. We use this approach on both Win32 and WinCE to prevent
    // branching.
    //
    // TODO(steveblock): Test the cost of using GetDispatchMemberId vs
    // execScript using Stopwatch.

    CComPtr<IDispatch> javascript_engine_dispatch = GetGlobalObject();
    if (!javascript_engine_dispatch) { return false; }
    DISPID function_iid;
    if (FAILED(ActiveXUtils::GetDispatchMemberId(javascript_engine_dispatch,
                                                 STRING16(L"eval"),
                                                 &function_iid))) {
      return false;
    }
    CComVariant script_variant(script.c_str());
    DISPPARAMS parameters = {0};
    parameters.cArgs = 1;
    parameters.rgvarg = &script_variant;
    return SUCCEEDED(javascript_engine_dispatch->Invoke(
        function_iid,           // member to invoke
        IID_NULL,               // reserved
        LOCALE_SYSTEM_DEFAULT,  // TODO(cprince): should this be user default?
        DISPATCH_METHOD,        // dispatch/invoke as...
        &parameters,            // parameters
        NULL,                   // receives result (NULL okay)
        NULL,                   // receives exception (NULL okay)
        NULL));                 // receives badarg index (NULL okay)
  }

  void SetErrorHandler(JsErrorHandlerInterface *handler) {
    assert(false);  // Should not be called on the DocumentJsRunner.
  }

  bool AddEventHandler(JsEventType event_type,
                       JsEventHandlerInterface *handler) {
    if (event_type == JSEVENT_UNLOAD) {
      // Create an HTML event monitor to send the unload event when the page
      // goes away.
      if (unload_monitor_ == NULL) {
        unload_monitor_.reset(new HtmlEventMonitor(kEventUnload,
                                                   HandleEventUnload, this));
#ifdef WINCE
        // TODO(steveblock): Investigate whether IPIEHTMLWindow2 will meet our
        // needs here.
        return false;
#else
        CComPtr<IHTMLWindow3> event_source;
        if (FAILED(ActiveXUtils::GetHtmlWindow3(site_, &event_source))) {
          return false;
        }

        unload_monitor_->Start(event_source);
#endif
      }
    }

    return JsRunnerBase::AddEventHandler(event_type, handler);
  }

 private:
  static void HandleEventUnload(void *user_param) {
    // Callback for 'onunload'
    static_cast<DocumentJsRunner*>(user_param)->SendEvent(JSEVENT_UNLOAD);
  }

  scoped_ptr<HtmlEventMonitor> unload_monitor_;  // For 'onunload' notifications
  CComPtr<IUnknown> site_;

  DISALLOW_EVIL_CONSTRUCTORS(DocumentJsRunner);
};


JsRunnerInterface* NewJsRunner() {
  return static_cast<JsRunnerInterface*>(new JsRunner());
}

JsRunnerInterface* NewDocumentJsRunner(IGeneric *base, JsContextPtr context) {
  return static_cast<JsRunnerInterface*>(new DocumentJsRunner(base));
}
