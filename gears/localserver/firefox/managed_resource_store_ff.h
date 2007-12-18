// Copyright 2005, Google Inc.
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

#ifndef GEARS_LOCALSERVER_FIREFOX_MANAGED_RESOURCE_STORE_FF_H__
#define GEARS_LOCALSERVER_FIREFOX_MANAGED_RESOURCE_STORE_FF_H__

#include "ff/genfiles/localserver.h" // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/message_service.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"
#include "gears/localserver/common/managed_resource_store.h"
#include "gears/localserver/firefox/update_task_ff.h"

// Object identifiers
extern const char *kGearsManagedResourceStoreClassName;
extern const nsCID kGearsManagedResourceStoreClassId;

//-----------------------------------------------------------------------------
// GearsManagedResourceStore
//-----------------------------------------------------------------------------
class GearsManagedResourceStore
    : public ModuleImplBaseClass,
      public GearsManagedResourceStoreInterface,
      private AsyncTask::Listener,
      public MessageObserverInterface,
      public JsEventHandlerInterface  {
 public:
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
  // End boilerplate code. Begin interface.

  GearsManagedResourceStore() {}
  void HandleEvent(JsEventType event_type);
  virtual void OnNotify(MessageService *service,
                        const char16 *topic,
                        const NotificationData *data);

  NS_IMETHOD GetName(nsAString &name);
  NS_IMETHOD GetRequiredCookie(nsAString &cookie);
  NS_IMETHOD GetEnabled(PRBool *enabled);
  NS_IMETHOD SetEnabled(PRBool enabled);
  NS_IMETHOD GetManifestUrl(nsAString &url_out);
  NS_IMETHOD SetManifestUrl(const nsAString &url);
  NS_IMETHOD GetLastUpdateCheckTime(PRInt32 *time);
  NS_IMETHOD GetUpdateStatus(PRInt32 *status);
  NS_IMETHOD GetLastErrorMessage(nsAString &error_message_out);
  NS_IMETHOD SetOnerror(nsIVariant *in_handler);
  NS_IMETHOD GetOnerror(nsIVariant **out_handler);
  NS_IMETHOD SetOnprogress(nsIVariant *in_handler);
  NS_IMETHOD GetOnprogress(nsIVariant **out_handler);
  NS_IMETHOD SetOncomplete(nsIVariant *in_handler);
  NS_IMETHOD GetOncomplete(nsIVariant **out_handler);
  NS_IMETHOD CheckForUpdate();
  NS_IMETHOD GetCurrentVersion(nsAString &ver);

 protected:
  ~GearsManagedResourceStore();

 private:
  virtual void HandleEvent(int code, int param, AsyncTask *source);
  void GetAppVersionString(WebCacheDB::VersionReadyState state,
                           nsAString &ver_out);
  void InitUnloadMonitor();

  scoped_ptr<FFUpdateTask> update_task_;
  ManagedResourceStore store_;
  scoped_ptr<JsRootedCallback> onerror_handler_;
  scoped_ptr<JsRootedCallback> onprogress_handler_;
  scoped_ptr<JsRootedCallback> oncomplete_handler_;
  std::string16 observer_topic_;
  scoped_ptr<JsEventMonitor> unload_monitor_;

  friend class GearsLocalServer;

  DISALLOW_EVIL_CONSTRUCTORS(GearsManagedResourceStore);
};


#endif // GEARS_LOCALSERVER_FIREFOX_MANAGED_RESOURCE_STORE_FF_H__
