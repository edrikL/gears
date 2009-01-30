// Copyright 2008, Google Inc.
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

#include "gears/base/opera/opera_utils.h"

#include <assert.h>

#include "gears/base/npapi/module_wrapper.h"
#include "gears/factory/factory_impl.h"
#include "gears/factory/factory_np.h"
#include "gears/ui/common/settings_dialog.h"
#include "third_party/opera/opera_callback_api.h"
#include "third_party/opera/opera_worker_interface.h"

#ifdef OS_WINCE
#include "gears/installer/opera/cab_updater_op.h"
static const char16* kGuid = L"%7B000C0320-09CE-4D7E-B3C9-66B2ACB7FF80%7D";
#endif

// Implementation of the wrapper class for the native opera worker threads
class OperaWorkerThreadImpl : public OperaWorkerThreadInterface {
 public:
  OperaWorkerThreadImpl() : js_runner_(NULL) {
    factory_.reset(NULL);
  }

  virtual ~OperaWorkerThreadImpl() {
    factory_.reset(NULL);
    delete js_runner_;
  }

  virtual void Destroy() {
    delete this;
  }

  virtual NPObject *CreateWorkerFactory(NPP instance,
                                        NPObject *global_object,
                                        const unsigned short *url) {
    if (!js_runner_) {
      // TODO(stighal@opera.com): Add required overload of NewJsRunner().
      //js_runner_ = NewJsRunner(instance, global_object);
      assert(false);
    }
    if (!js_runner_)
      return NULL;

    if (!factory_.get()) {
      SecurityOrigin security_origin;
      scoped_refptr<BrowsingContext> browsing_context;
      if (!security_origin.InitFromUrl(url))
        return NULL;
      BrowserUtils::GetPageBrowsingContext(instance, &browsing_context);

      ModuleEnvironment *module_env =
        new ModuleEnvironment(security_origin,
                              true,  // is_worker
                              js_runner_,
                              browsing_context.get());
      if (!module_env)
        return NULL;

      JsCallContext context(instance, global_object, 0, NULL, NULL);

      BrowserUtils::EnterScope(&context);
      bool created = CreateModule<GearsFactoryImpl>(module_env,
                                                    &context,
                                                    &factory_);
      BrowserUtils::ExitScope();

      if (!created) { return NULL; }
    }

    if (!NPVARIANT_IS_OBJECT(factory_.get()->GetWrapperToken()))
      return NULL;

    NPObject *factory_object =
        NPVARIANT_TO_OBJECT(factory_.get()->GetWrapperToken());
    NPN_RetainObject(factory_object);

    return factory_object;
  }

  virtual void SuspendObjectCreation() {
    if (factory_.get())
      factory_.get()->SuspendObjectCreation();
  }

  virtual void ResumeObjectCreationAndUpdatePermissions() {
    if (factory_.get())
      factory_.get()->ResumeObjectCreationAndUpdatePermissions();
  }

 private:
  JsRunnerInterface *js_runner_;
  scoped_refptr<GearsFactoryImpl> factory_;

  DISALLOW_EVIL_CONSTRUCTORS(OperaWorkerThreadImpl);
};

// static
OperaGearsApiInterface *OperaUtils::opera_api_ = NULL;
// static
ThreadId OperaUtils::browser_thread_ = 0;

//static
void OperaUtils::Init(OperaGearsApiInterface *opera_api,
                      ThreadId browser_thread) {
  opera_api_ = opera_api;
  browser_thread_ = browser_thread;

#ifdef OS_WINCE
  HWND opera_window = ::FindWindow(L"Opera_MainWndClass", NULL);

  // TODO(deepak@opera.com): Move this code to a location where we have access
  // to a valid browsing context.
  BrowsingContext *browsing_context = NULL;
  static OPCabUpdater updater(browsing_context);
  updater.Start(opera_window, kGuid);
#endif
}

// static
OperaGearsApiInterface *OperaUtils::GetBrowserApiForGears() {
  assert(opera_api_ != NULL);
  return opera_api_;
}

// static
ThreadId OperaUtils::GetBrowserThreadId() {
  assert(browser_thread_ != NULL);
  return browser_thread_;
}

// static
OperaWorkerThreadInterface *OperaUtils::CreateWorkerThread() {
  OperaWorkerThreadImpl *worker_thread = new OperaWorkerThreadImpl();
  return worker_thread;
}

// static
void OperaUtils::OpenSettingsDialog() {
  SettingsDialog::Run(NULL);
}
