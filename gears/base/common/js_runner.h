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


// The JsParam* types define values for sending and receiving JS parameters.
enum JsParamType {
  JSPARAM_BOOL,
  JSPARAM_INT,
  JSPARAM_STRING16
};

struct JsParamToSend {
  JsParamType type;
  const void *value_ptr;
};

struct JsParamToRecv {
  JsParamType type;
  void *value_ptr;
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
  virtual bool Eval(const std::string16 &script) = 0;
  virtual void SetErrorHandler(JsErrorHandlerInterface *error_handler) = 0;

  // Creates a new object in the JavaScript engine using the specified
  // constructor. If the constructor is NULL, it defaults to "Object". The
  // caller takes ownership of the returned value.
  virtual JsRootedToken *NewObject(
                             const char16 *optional_global_ctor_name) = 0;

  // SetProperty*() overwrites the existing named property or adds a new one if
  // none exists.
  virtual bool SetPropertyString(JsToken object, const char16 *name,
                                 const char16 *val) = 0;
  virtual bool SetPropertyInt(JsToken object, const char16 *name,
                              int val) = 0;
  // TODO(aa): SetPropertyBool, SetPropertyObject (to build trees), etc...
  // TODO(aa): Support for building arrays?

  virtual bool InvokeCallback(const JsRootedCallback *callback,
                              int argc, JsParamToSend *argv) = 0;

#ifdef DEBUG
  virtual void ForceGC() = 0;
#endif

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
