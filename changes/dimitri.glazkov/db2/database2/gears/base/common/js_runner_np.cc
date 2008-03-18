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

#include "gears/base/common/js_runner.h"

#include <assert.h>
#include <set>
#include <stdio.h>

#include "npapi.h"
#include "npruntime.h"

#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#include "gears/base/common/common.h" // for DISALLOW_EVIL_CONSTRUCTORS
#include "gears/base/common/html_event_monitor.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/npapi/np_utils.h"
#include "gears/base/npapi/scoped_npapi_handles.h"


// Internal base class used to share some code between DocumentJsRunner and
// JsRunner. Do not override these methods from JsRunner or DocumentJsRunner.
// Either share the code here, or move it to those two classes if it's
// different.
class JsRunnerBase : public JsRunnerInterface {
 public:
  JsRunnerBase(NPP instance) : np_instance_(instance) {
    NPN_GetValue(np_instance_, NPNVWindowNPObject, &global_object_);
  }
  ~JsRunnerBase() {
    NPN_ReleaseObject(global_object_);
  }

  JsContextPtr GetContext() {
    return np_instance_;
  }

  NPObject *GetGlobalObject() {
    return global_object_;
  }

  JsObject *NewObject(const char16 *optional_global_ctor_name,
                      bool dump_on_fail = false) {
    NPObject *global_object = GetGlobalObject();
    if (!global_object) {
      LOG(("Could not get global object from script engine."));
      return NULL;
    }

    std::string ctor_name_utf8;
    if (optional_global_ctor_name) {
      if (!String16ToUTF8(optional_global_ctor_name, &ctor_name_utf8)) {
        LOG(("Could not convert constructor name."));
        return NULL;
      }
    } else {
      ctor_name_utf8 = "Object";
    }
    ctor_name_utf8.append("()");

    // Evaluate javascript code: 'ConstructorName()'
    NPString script = {ctor_name_utf8.c_str(), ctor_name_utf8.length()};
    ScopedNPVariant object;
    bool rv = NPN_Evaluate(GetContext(), global_object, &script, &object);
    if (!rv) {
      LOG(("Could not invoke object constructor."));
      return NULL;
    }

    scoped_ptr<JsObject> retval(new JsObject);
    if (!retval->SetObject(object, GetContext())) {
      LOG(("Could not assign to JsObject."));
      return NULL;
    }

    return retval.release();
  }

  JsArray* NewArray() {
    // TODO: Implement
    assert(false);
    return NULL;
  }

  bool InvokeCallback(const JsRootedCallback *callback,
                      int argc, JsParamToSend *argv,
                      JsRootedToken **optional_alloc_retval) {
    assert(callback && (!argc || argv));
    if (!NPVARIANT_IS_OBJECT(callback->token())) { return false; }

    // Setup argument array.
    scoped_array<ScopedNPVariant> js_engine_argv(new ScopedNPVariant[argc]);
    for (int i = 0; i < argc; ++i)
      ConvertJsParamToToken(argv[i], GetContext(), &js_engine_argv[i]);

    // Invoke the method.
    NPVariant retval;
    bool rv = NPN_InvokeDefault(np_instance_,
                                NPVARIANT_TO_OBJECT(callback->token()),
                                js_engine_argv.get(), argc, &retval);
    if (!rv) { return false; }

    if (optional_alloc_retval) {
      // Note: A valid NPVariant is returned no matter what the js function
      // returns. If it returns nothing, or explicitly returns <undefined>, the
      // variant will contain VOID. If it returns <null>, the variant will
      // contain NULL. Always returning a JsRootedToken should allow us to
      // coerce these values to other types correctly in the future.
      *optional_alloc_retval = new JsRootedToken(np_instance_, retval);
    }

    NPN_ReleaseVariantValue(&retval);

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
    // TODO(mpcomplete): figure this out.
    //Eval(STRING16(L"CollectGarbage();"));
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

  NPP np_instance_;
  NPObject *global_object_;

 private:
  std::set<JsEventHandlerInterface *> event_handlers_[MAX_JSEVENTS];

  DISALLOW_EVIL_CONSTRUCTORS(JsRunnerBase);
};

// TODO(mpcomplete): implement me.  We need a browser-independent way of
// creating a JS engine.  Options:
// 1. Bundle spidermonkey and just run that.
// 2. Leave this class as a browser-specific module.
class JsRunner : public JsRunnerBase {
 public:
  JsRunner() : JsRunnerBase(NULL) {
  }
  virtual ~JsRunner() {
    // Alert modules that the engine is unloading.
    SendEvent(JSEVENT_UNLOAD);
  }

  bool AddGlobal(const std::string16 &name, IGeneric *object, gIID iface_id) {
    return false;
  }
  bool Start(const std::string16 &full_script) {
    return false;
  }
  bool Stop() {
    return false;
  }
  bool Eval(const std::string16 &script) {
    return false;
  }
  void SetErrorHandler(JsErrorHandlerInterface *error_handler) {
  }

 private:

  DISALLOW_EVIL_CONSTRUCTORS(JsRunner);
};


// This class is a stub that is used to present a uniform interface to
// common functionality to both workers and the main thread.
class DocumentJsRunner : public JsRunnerBase {
 public:
  DocumentJsRunner(NPP instance) : JsRunnerBase(instance) { }

  virtual ~DocumentJsRunner() {}

  bool AddGlobal(const std::string16 &name, IGeneric *object, gIID iface_id) {
    // TODO(mpcomplete): Add this functionality to DocumentJsRunner.
    return false;
  }

  bool Start(const std::string16 &full_script) {
    assert(false); // Should not be called on the DocumentJsRunner.
    return false;
  }

  bool Stop() {
    assert(false); // Should not be called on the DocumentJsRunner.
    return false;
  }

  bool Eval(const std::string16 &script) {
    NPObject *global_object = GetGlobalObject();

    std::string script_utf8;
    if (!String16ToUTF8(script.data(), script.length(), &script_utf8)) {
      LOG(("Could not convert script to UTF8."));
      return false;
    }

    NPString np_script = {script_utf8.data(), script_utf8.length()};
    NPVariant retval;
    if (!NPN_Evaluate(np_instance_, global_object, &np_script, &retval))
      return false;

    NPN_ReleaseVariantValue(&retval);
    return true;
  }

  void SetErrorHandler(JsErrorHandlerInterface *handler) {
    assert(false); // Should not be called on the DocumentJsRunner.
  }

  static void HandleEventUnload(void *user_param) {
    static_cast<DocumentJsRunner*>(user_param)->SendEvent(JSEVENT_UNLOAD);
  }

  bool AddEventHandler(JsEventType event_type,
                       JsEventHandlerInterface *handler) {
    if (event_type == JSEVENT_UNLOAD) {
      // Monitor 'onunload' to send the unload event when the page goes away.
      if (unload_monitor_ == NULL) {
        // Retrieve retrieve the DOM window.
        NPObject* window;
        if (NPN_GetValue(GetContext(), NPNVWindowNPObject, &window)
            != NPERR_NO_ERROR)
          return false;
        ScopedNPObject window_scoped(window);

        unload_monitor_.reset(new HtmlEventMonitor(kEventUnload,
                                                   HandleEventUnload,
                                                   this));
        unload_monitor_->Start(GetContext(), window);
      }
    }

    return JsRunnerBase::AddEventHandler(event_type, handler);
  }

 private:
  scoped_ptr<HtmlEventMonitor> unload_monitor_;  // For 'onunload' notifications
  DISALLOW_EVIL_CONSTRUCTORS(DocumentJsRunner);
};


JsRunnerInterface *NewJsRunner() {
  return static_cast<JsRunnerInterface *>(new JsRunner());
}

JsRunnerInterface *NewDocumentJsRunner(IGeneric *base, JsContextPtr context) {
  return static_cast<JsRunnerInterface *>(new DocumentJsRunner(context));
}
