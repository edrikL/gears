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

#ifndef GEARS_LOCALSERVER_FIREFOX_RESOURCE_STORE_FF_H__
#define GEARS_LOCALSERVER_FIREFOX_RESOURCE_STORE_FF_H__

#include <deque>
#include "ff/genfiles/localserver.h" // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/string16.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"
#include "gears/localserver/common/resource_store.h"
#include "gears/localserver/firefox/capture_task_ff.h"


// Object identifiers
extern const char *kGearsResourceStoreClassName;
extern const nsCID kGearsResourceStoreClassId;


class nsIFile;
class nsIScriptContext;


//-----------------------------------------------------------------------------
// GearsResourceStore
//-----------------------------------------------------------------------------
class GearsResourceStore
    : public GearsBaseClass,
      public GearsResourceStoreInterface,
      private AsyncTask::Listener,
      public JsEventHandlerInterface {
 public:
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
  // End boilerplate code. Begin interface.

  GearsResourceStore() : next_capture_id_(0), page_is_unloaded_(false) {}

  NS_IMETHOD GetName(nsAString &name);
  NS_IMETHOD GetRequiredCookie(nsAString &cookie);
  NS_IMETHOD GetEnabled(PRBool *enabled);
  NS_IMETHOD SetEnabled(PRBool enabled);
  NS_IMETHOD Capture(nsIVariant *urls,
                     ResourceCaptureCompletionHandler *completion_callback,
                     PRInt32 *capture_id_retval);
  NS_IMETHOD AbortCapture(PRInt32 capture_id); \
  NS_IMETHOD IsCaptured(const nsAString &url, PRBool *is_captured_retval);
  NS_IMETHOD Remove(const nsAString &url);
  NS_IMETHOD Rename(const nsAString &src_url, const nsAString &dst_url);
  NS_IMETHOD Copy(const nsAString &src_url, const nsAString &dst_url);
  NS_IMETHOD GetHeader(const nsAString &url, const nsAString &header_in,
                       nsAString &retval);
  NS_IMETHOD GetAllHeaders(const nsAString &url, nsAString &retval);
  NS_IMETHOD CaptureFile(nsISupports *file_input_element, const nsAString &url);
  NS_IMETHOD GetCapturedFileName(const nsAString &url,
                                 nsAString &file_name_retval);
  NS_IMETHOD CreateFileSubmitter(GearsFileSubmitterInterface **_retval);

  void HandleEvent(JsEventType event_type);

 protected:
  ~GearsResourceStore();

 private:
  bool ResolveAndAppendUrl(const std::string16 &url, FFCaptureRequest *request);
  bool ResolveUrl(const char16 *url, std::string16 *resolved_url);
  bool StartCaptureTaskIfNeeded(bool fire_events_on_failure);
  void FireFailedEvents(FFCaptureRequest *request);
  void InvokeCompletionCallback(FFCaptureRequest *request,
                                const std::string16 &capture_url,
                                int capture_id,
                                bool succeeded);

  virtual void HandleEvent(int code, int param, AsyncTask *source);
  void OnCaptureUrlComplete(int index, bool success);
  void OnCaptureTaskComplete();

  nsresult CaptureFile(const nsAString &filepath, const char16 *full_url);
  nsresult CaptureFile(nsIFile *file, const char16 *full_url);

  void AbortAllRequests();

  int next_capture_id_;
  std::deque<FFCaptureRequest*> pending_requests_;
  scoped_ptr<FFCaptureRequest> current_request_;
  scoped_ptr<CaptureTask> capture_task_;
  bool page_is_unloaded_;
  scoped_ptr<JsEventMonitor> unload_monitor_;
  std::string16 exception_message_;
  ResourceStore store_;

  friend class GearsLocalServer;

  DISALLOW_EVIL_CONSTRUCTORS(GearsResourceStore);
};


#endif // GEARS_LOCALSERVER_FIREFOX_RESOURCE_STORE_FF_H__
