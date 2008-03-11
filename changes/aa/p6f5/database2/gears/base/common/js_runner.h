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
// Provides a platform-independent interface for instantiating a JavaScript
// execution engine and running user-provided code.

#ifndef GEARS_BASE_COMMON_JS_RUNNER_H__
#define GEARS_BASE_COMMON_JS_RUNNER_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"


// To insert C++ objects into JavaScript, most (all?) JS engines take a C++
// pointer implementing some baseline interface.  The implementation differs
// across platforms, but the concept is consistent, so we define IGeneric here.
//
// TODO(cprince): move this definition to its own file if somebody else needs it
#if defined BROWSER_IE
  #include <windows.h>
  typedef IUnknown IGeneric;
  typedef int gIID;
#elif defined BROWSER_FF
  typedef nsISupports IGeneric;
  typedef nsIID gIID;
#elif defined BROWSER_NPAPI
  typedef void IGeneric;
  typedef int gIID;
#elif defined BROWSER_SAFARI
  // Just placeholder values for Safari since the created workers use a separate
  // process for JS execution.
  typedef void IGeneric;
  typedef int gIID;
#endif


// Represents an error that occured inside a JsRunner.
struct JsErrorInfo {
  int line;
  std::string16 message;
  // TODO(aa): code, so that people can detect certain errors programatically.
  // TODO(aa): file, when workers can have multiple files?
};


// Interface for clients of JsRunner to implement if they want to handle errors.
class JsErrorHandlerInterface {
 public:
  virtual void HandleError(const JsErrorInfo &error_info) = 0;
};

enum JsEventType {
  JSEVENT_UNLOAD,
  MAX_JSEVENTS
};

// Interface for clients of JsRunner to implement if they want to handle events.
class JsEventHandlerInterface {
 public:
  virtual void HandleEvent(JsEventType event) = 0;
};

// Declares the platform-independent interface that Gears internals require
// for running JavaScript code.
class JsRunnerInterface {
 public:
  virtual ~JsRunnerInterface() {};
  // increments refcount
  virtual bool AddGlobal(const std::string16 &name,
                         IGeneric *object,
                         gIID iface_iid) = 0;
  virtual bool Start(const std::string16 &full_script) = 0;
  virtual bool Stop() = 0;
  virtual JsContextPtr GetContext() = 0;
  // Only used by Firefox. Gets the JsContextWrapper instance associated with
  // the current JsContext.
  // TODO(aa): Once we no longer need the XPCOM support in JsContextWrapper,
  // maybe roll back into JsRunner so that this isn't needed?
  virtual JsContextWrapperPtr GetContextWrapper() { return NULL; }
  virtual bool Eval(const std::string16 &script) = 0;
  virtual void SetErrorHandler(JsErrorHandlerInterface *error_handler) = 0;

  // Creates a new object in the JavaScript engine using the specified
  // constructor. If the constructor is NULL, it defaults to "Object". The
  // caller takes ownership of the returned value.
  virtual JsObject *NewObject(const char16 *optional_global_ctor_name,
                              // TODO(zork): Remove this when we find the error.
                              bool dump_on_error = false) = 0;
  
  // Creates a new Array object in JavaScript engine. 
  // The caller takes ownership of the returned value.
  virtual JsArray* NewArray() = 0;

  // Invokes a callback. If optional_alloc_retval is specified, this method will
  // create a new JsRootedToken that the caller is responsible for deleting.
  virtual bool InvokeCallback(const JsRootedCallback *callback,
                              int argc, JsParamToSend *argv,
                              JsRootedToken **optional_alloc_retval) = 0;

  virtual bool AddEventHandler(JsEventType event_type,
                               JsEventHandlerInterface *handler) = 0;
  virtual bool RemoveEventHandler(JsEventType event_type,
                                  JsEventHandlerInterface *handler) = 0;

#ifdef DEBUG
  virtual void ForceGC() = 0;
#endif

};

// Wraps the calls for adding and removing event handlers.  This is designed to
// be used with scoped_ptr<>.
class JsEventMonitor {
 public:
  JsEventMonitor(JsRunnerInterface *js_runner,
                 JsEventType event_type,
                 JsEventHandlerInterface *handler)
    : js_runner_(js_runner), event_type_(event_type), handler_(handler) {
    js_runner_->AddEventHandler(event_type_, handler_);
  }

  ~JsEventMonitor() {
    js_runner_->RemoveEventHandler(event_type_, handler_);
  }
 private:
  JsRunnerInterface *js_runner_;
  JsEventType event_type_;
  JsEventHandlerInterface *handler_;
};


// Callers: create instances using the following instead of 'new JsRunner'.
// The latter would require a full class description for JsRunner in this file,
// which is not always possible due to platform-specific implementation details.
// Destroy the object using regular 'delete'.
//
// This interface plays nicely with scoped_ptr, which was a design goal.

// This creates a JsRunner that is used in a worker.
JsRunnerInterface* NewJsRunner();

// This creates a JsRunner that can be used with the script engine running in an
// document.
JsRunnerInterface* NewDocumentJsRunner(IGeneric *base, JsContextPtr context);


#endif  // GEARS_BASE_COMMON_JS_RUNNER_H__
