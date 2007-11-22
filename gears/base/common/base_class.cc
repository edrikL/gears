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

#include <assert.h>

#include "gears/base/common/base_class.h"
#include "gears/base/common/security_model.h" // for kUnknownDomain
#include "gears/base/common/js_runner.h"
#include "gears/base/common/string_utils.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#if BROWSER_FF
#include "gears/base/firefox/dom_utils.h"
#elif BROWSER_IE
#include "gears/base/ie/activex_utils.h"
#elif BROWSER_NPAPI
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/npapi/np_utils.h"
#elif BROWSER_SAFARI
#include "gears/base/safari/browser_utils.h"
#endif


bool ModuleImplBaseClass::InitBaseFromSibling(
                              const ModuleImplBaseClass *other) {
  assert(other->is_initialized_);
  return InitBaseManually(other->env_is_worker_,
#if BROWSER_FF || BROWSER_NPAPI
                          other->env_page_js_context_,
#elif BROWSER_IE
                          other->env_page_iunknown_site_,
#endif
                          other->env_page_origin_,
                          other->js_runner_);
}


#if BROWSER_FF
bool ModuleImplBaseClass::InitBaseFromDOM() {
#elif BROWSER_IE
bool ModuleImplBaseClass::InitBaseFromDOM(IUnknown *site) {
#elif BROWSER_NPAPI
bool ModuleImplBaseClass::InitBaseFromDOM(JsContextPtr instance) {
#elif BROWSER_SAFARI
bool ModuleImplBaseClass::InitBaseFromDOM(const char *url_str) {
#endif
  bool is_worker = false;
  SecurityOrigin security_origin;
#if BROWSER_FF
  JsContextPtr cx;
  bool succeeded = DOMUtils::GetJsContext(&cx) &&
                   DOMUtils::GetPageOrigin(&security_origin);
  return succeeded && InitBaseManually(is_worker, cx, security_origin,
                                       NewDocumentJsRunner(NULL, cx));
#elif BROWSER_IE
  JsRunnerInterface *runner = NewDocumentJsRunner(site, NULL);
#ifdef WINCE
  // Start the local script engine. This engine is used simply to create objects
  // which are passed as parameters to callbacks running in the page's script
  // engine.
  if (!runner->Start(L"")) {
    return false;
  }
#endif
  bool succeeded = ActiveXUtils::GetPageOrigin(site, &security_origin);
  return succeeded && InitBaseManually(is_worker, site, security_origin,
                                       runner);
#elif BROWSER_NPAPI
  bool succeeded = BrowserUtils::GetPageSecurityOrigin(instance,
                                                       &security_origin);
  return succeeded && InitBaseManually(is_worker, instance, security_origin,
                                       NewDocumentJsRunner(NULL, instance));
#elif BROWSER_SAFARI
  bool succeeded = SafariURLUtilities::GetPageOrigin(url_str, &security_origin);
  return succeeded && InitBaseManually(is_worker, security_origin, 0);
#endif
}


bool ModuleImplBaseClass::InitBaseManually(bool is_worker,
#if BROWSER_FF || BROWSER_NPAPI
                                      JsContextPtr cx,
#elif BROWSER_IE
                                      IUnknown *site,
#endif
                                      const SecurityOrigin &page_origin,
                                      JsRunnerInterface *js_runner) {
  assert(!is_initialized_);
  // We want to make sure page_origin is initialized, but that isn't exposed
  // directly.  So access any member to trip its internal 'initialized_' assert.
  assert(page_origin.port() != -1);

  env_is_worker_ = is_worker;
#if BROWSER_FF || BROWSER_NPAPI
  env_page_js_context_ = cx;
#elif BROWSER_IE
  env_page_iunknown_site_ = site;
#endif
  env_page_origin_ = page_origin;
  js_runner_ = js_runner;

#if BROWSER_FF
  worker_js_argc_ = 0;
  worker_js_argv_ = NULL;
#elif BROWSER_IE
  // These do not exist in IE yet.
#endif

  is_initialized_ = true;
  return true;
}


bool ModuleImplBaseClass::EnvIsWorker() const {
  assert(is_initialized_);
  return env_is_worker_;
}

const std::string16& ModuleImplBaseClass::EnvPageLocationUrl() const {
  assert(is_initialized_);
  return env_page_origin_.full_url();
}

#if BROWSER_FF || BROWSER_NPAPI
JsContextPtr ModuleImplBaseClass::EnvPageJsContext() const {
  assert(is_initialized_);
  return env_page_js_context_;
}
#elif BROWSER_IE
IUnknown* ModuleImplBaseClass::EnvPageIUnknownSite() const {
  assert(is_initialized_);
  return env_page_iunknown_site_;
}
#endif

const SecurityOrigin& ModuleImplBaseClass::EnvPageSecurityOrigin() const {
  assert(is_initialized_);
  return env_page_origin_;
}

JsRunnerInterface *ModuleImplBaseClass::GetJsRunner() const {
  assert(is_initialized_);
  return js_runner_;
}

#if BROWSER_NPAPI
void ModuleImplBaseClass::AddRef() {
  js_wrapper_->AddRef();
}

void ModuleImplBaseClass::Release() {
  js_wrapper_->Release();
}
#endif


//-----------------------------------------------------------------------------
#if BROWSER_FF  // the rest of this file only applies to Firefox, for now


void ModuleImplBaseClass::JsWorkerSetParams(int argc, JsToken *argv) {
  assert(is_initialized_);
  worker_js_argc_ = argc;
  worker_js_argv_ = argv;
}

int ModuleImplBaseClass::JsWorkerGetArgc() const {
  assert(is_initialized_);
  assert(EnvIsWorker());
  return worker_js_argc_;
}

JsToken* ModuleImplBaseClass::JsWorkerGetArgv() const {
  assert(is_initialized_);
  assert(EnvIsWorker());
  return worker_js_argv_;
}

#endif // BROWSER_FF
//-----------------------------------------------------------------------------
