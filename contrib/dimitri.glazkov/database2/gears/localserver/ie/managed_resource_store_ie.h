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

#ifndef GEARS_LOCALSERVER_IE_MANAGED_RESOURCE_STORE_IE_H__
#define GEARS_LOCALSERVER_IE_MANAGED_RESOURCE_STORE_IE_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/message_service.h"
#include "gears/localserver/common/managed_resource_store.h"
#include "gears/localserver/ie/update_task_ie.h"
#include "genfiles/interfaces.h"

//------------------------------------------------------------------------------
// GearsManagedResourceStore
//------------------------------------------------------------------------------
class ATL_NO_VTABLE GearsManagedResourceStore
    : public ModuleImplBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsManagedResourceStore>,
      public CWindowImpl<GearsManagedResourceStore>,
      public IDispatchImpl<GearsManagedResourceStoreInterface>,
      public MessageObserverInterface,
      public JsEventHandlerInterface {
 public:
  BEGIN_COM_MAP(GearsManagedResourceStore)
    COM_INTERFACE_ENTRY(GearsManagedResourceStoreInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsManagedResourceStore)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  HRESULT FinalConstruct();
  void FinalRelease();
  // End boilerplate code. Begin interface.

  // need a default constructor to CreateInstance objects in IE
  GearsManagedResourceStore() {}

  void HandleEvent(JsEventType event_type);
  virtual void OnNotify(MessageService *service,
                        const char16 *topic,
                        const NotificationData *data);

  // GearsManagedResourceStoreInterface
  // This is the interface we expose to JavaScript.

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_name(
      /* [retval][out] */ BSTR *name);

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_requiredCookie(
      /* [retval][out] */ BSTR *retval);

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_enabled(
      /* [retval][out] */ VARIANT_BOOL *enabled);

  virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_enabled(
      /* [in] */ VARIANT_BOOL enabled);

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_manifestUrl(
      /* [retval][out] */ BSTR *retval);

  virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_manifestUrl(
      /* [in] */ const BSTR manifest_url);

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_lastUpdateCheckTime(
      /* [retval][out] */ long *time);

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_updateStatus(
      /* [retval][out] */ int *status);

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_lastErrorMessage(
      /* [retval][out] */ BSTR *last_error_message);

  virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_onerror(
      /* [in] */ const VARIANT *in_value);

  virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_onprogress(
      /* [in] */ const VARIANT *in_value);

  virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_oncomplete(
      /* [in] */ const VARIANT *in_value);

  virtual HRESULT STDMETHODCALLTYPE checkForUpdate(void);

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_currentVersion(
      /* [retval][out] */ BSTR *ver);

 private:
  // CWindowImpl to receive window messages sent to us from IEUpdateTask
  static const int kUpdateTaskMessageBase = 1224;
  static const int 
      WM_UPDATE_TASK_COMPLETE = IEUpdateTask::UPDATE_TASK_COMPLETE
                                + kUpdateTaskMessageBase;

  BEGIN_MSG_MAP(GearsManagedResourceStore)
    MESSAGE_HANDLER(WM_UPDATE_TASK_COMPLETE, OnUpdateTaskComplete)
  END_MSG_MAP()

  LRESULT OnUpdateTaskComplete(UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam,
                               BOOL &bHandled);

  HRESULT CreateWindowIfNeeded();
  void InitUnloadMonitor();

  // other private data members
  HRESULT GetVersionString(WebCacheDB::VersionReadyState state, BSTR *ver_out);

  scoped_ptr<IEUpdateTask> update_task_;
  ManagedResourceStore store_;
  scoped_ptr<JsRootedCallback> onerror_handler_;
  scoped_ptr<JsRootedCallback> onprogress_handler_;
  scoped_ptr<JsRootedCallback> oncomplete_handler_;
  std::string16 observer_topic_;
  scoped_ptr<JsEventMonitor> unload_monitor_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsManagedResourceStore);
};


#endif  // GEARS_LOCALSERVER_IE_MANAGED_RESOURCE_STORE_IE_H__
