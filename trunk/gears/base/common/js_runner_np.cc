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
#include <map>
#include <stdio.h>

#ifdef BROWSER_WEBKIT
#include <WebKit/npapi.h>
#else
#include "third_party/npapi/nphostapi.h"
#endif

#include "third_party/scoped_ptr/scoped_ptr.h"

#include "gears/base/common/basictypes.h" // for DISALLOW_EVIL_CONSTRUCTORS
#include "gears/base/common/html_event_monitor.h"
#include "gears/base/common/js_runner_utils.h"  // For ThrowGlobalErrorImpl()
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/npapi/np_utils.h"
#include "gears/base/npapi/scoped_npapi_handles.h"

// We keep a map of active DocumentJsRunners so we can notify them when their
// NP instance is destroyed.
class DocumentJsRunner;
typedef std::map<NPP, DocumentJsRunner*> DocumentJsRunnerList;
static DocumentJsRunnerList *g_document_js_runners = NULL;

static void RegisterDocumentJsRunner(NPP instance, DocumentJsRunner* runner) {
  if (!g_document_js_runners)
    g_document_js_runners = new DocumentJsRunnerList;
  // Right now there is a 1:1 mapping among NPPs, GearsFactorys, and
  // DocumentJsRunners.
  assert((*g_document_js_runners)[instance] == NULL);
  (*g_document_js_runners)[instance] = runner;
}

static void UnregisterDocumentJsRunner(NPP instance) {
  if (!g_document_js_runners)
    return;

  g_document_js_runners->erase(instance);
  if (g_document_js_runners->empty()) {
    delete g_document_js_runners;
    g_document_js_runners = NULL;
  }
}

// Internal base class used to share some code between DocumentJsRunner and
// JsRunner. Do not override these methods from JsRunner or DocumentJsRunner.
// Either share the code here, or move it to those two classes if it's
// different.
class JsRunnerBase : public JsRunnerInterface {
 public:
  JsRunnerBase(NPP instance) : np_instance_(instance), evaluator_(NULL) {
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
                      bool dump_on_error = false) {
    // NOTE: optional_global_ctor_name will be removed. Use NewError and NewDate
    // instead. We support Error for backwards compatibility.
    std::string string_to_eval;
    if (optional_global_ctor_name) {
      if (!String16ToUTF8(optional_global_ctor_name, &string_to_eval)) {
        LOG(("Could not convert constructor name."));
        return NULL;
      }
    } else {
      string_to_eval = "Object";
    }
    assert(string_to_eval == "Object" || string_to_eval == "Error");
    string_to_eval.append("()");
    return NewObjectImpl(string_to_eval);
  }

  JsObject *NewError(const std::string16 &message,
                     bool dump_on_error = false) {
    std::string message_utf8;
    if (!String16ToUTF8(message.c_str(), message.size(), &message_utf8)) {
      LOG(("Could not convert message."));
      return NULL;
    }
    return NewObjectImpl("Error('" + message_utf8 + "')");
  }

  JsObject *NewDate(int64 milliseconds_since_epoch) {
    return NewObjectImpl("new Date(" +
                         Integer64ToString(milliseconds_since_epoch) +
                         ")");
  }

  JsArray* NewArray() {
    scoped_ptr<JsObject> js_object(NewObjectImpl("Array()"));
    if (!js_object.get())
      return NULL;

    scoped_ptr<JsArray> js_array(new JsArray());
    if (!js_array.get())
      return NULL;

    if (!js_array->SetArray(js_object->token(), js_object->context()))
      return NULL;

    return js_array.release();
  }

  bool InvokeCallback(const JsRootedCallback *callback,
                      int argc, JsParamToSend *argv,
                      JsRootedToken **optional_alloc_retval) {
    assert(callback && (!argc || argv));
    if (!NPVARIANT_IS_OBJECT(callback->token())) { return false; }

    NPObject *evaluator = GetEvaluator();
    if (!evaluator) { return false; }

    // Setup argument array.
    scoped_ptr<JsArray> array(NewArray());
    for (int i = 0; i < argc; ++i) {
      ScopedNPVariant np_arg;
      ConvertJsParamToToken(argv[i], GetContext(), &np_arg);
      array->SetElement(i, np_arg);
    }

    // Invoke the method.
    NPVariant args[] = { callback->token(), array->token() };
    ScopedNPVariant result;
    bool rv = NPN_InvokeDefault(GetContext(), evaluator,
                                args, ARRAYSIZE(args), &result);
    if (!rv) { return false; }

    if (!NPVARIANT_IS_OBJECT(result)) {
      assert(false);
      return false;
    }

    JsObject obj;
    obj.SetObject(result, GetContext());

    std::string16 exception;
    if (obj.GetPropertyAsString(STRING16(L"exception"), &exception)) {
      SetException(exception);
      return false;
    }

    ScopedNPVariant retval;
    if (!obj.GetProperty(STRING16(L"retval"), &retval)) {
      retval.Reset();  // Reset to "undefined"
    }

    if (optional_alloc_retval) {
      // Note: A valid NPVariant is returned no matter what the js function
      // returns. If it returns nothing, or explicitly returns <undefined>, the
      // variant will contain VOID. If it returns <null>, the variant will
      // contain NULL. Always returning a JsRootedToken should allow us to
      // coerce these values to other types correctly in the future.
      *optional_alloc_retval = new JsRootedToken(np_instance_, retval);
    }

    return true;
  }

  bool Eval(const std::string16 &script) {
    return EvalImpl(script, true);
  }
  
  bool EvalImpl(const std::string16 &script, bool catch_exceptions) {
    NPObject *global_object = GetGlobalObject();

    std::string script_utf8;
    if (!String16ToUTF8(script.data(), script.length(), &script_utf8)) {
      LOG(("Could not convert script to UTF8."));
      return false;
    }
    
    if (catch_exceptions) {
      const char kEvaluatorTry[] = "try { ";
      const char kEvaluatorCatch[] =
          "; null; } catch (e) { e.message || String(e); }";
      script_utf8 = kEvaluatorTry + script_utf8 + kEvaluatorCatch;
    }

    NPString np_script = {script_utf8.data(), script_utf8.length()};
    ScopedNPVariant result;
    if (!NPN_Evaluate(GetContext(), global_object, &np_script, &result))
      return false;

    if (catch_exceptions && NPVARIANT_IS_STRING(result)) {
      // We caught an exception.
      NPString exception = NPVARIANT_TO_STRING(result);
      std::string16 exception16;
      if (!UTF8ToString16(exception.UTF8Characters, exception.UTF8Length,
                         &exception16)) {
       exception16 = STRING16(L"Could not get exception text");
      }
      SetException(exception16);
      return false;
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
    // TODO(mpcomplete): figure this out.
    //Eval(STRING16(L"CollectGarbage();"));
  }
#endif

  // This function and others (AddEventHandler, RemoveEventHandler etc) do not
  // contain any browser-specific code. They should be implemented in a new
  // class 'JsRunnerCommon', which inherits from JsRunnerInterface.
  virtual void ThrowGlobalError(const std::string16 &message) {
    std::string16 string_to_eval =
        std::string16(STRING16(L"window.onerror('")) +
        EscapeMessage(message) +
        std::string16(STRING16(L"')"));
        EvalImpl(string_to_eval, false);
  }

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

  // Throws an exception, handling the special case that we are not in
  // JavaScript context (in which case we throw the error globally).
  void SetException(const std::string16& exception) {
    JsCallContext *context = BrowserUtils::GetCurrentJsCallContext();
    if (context) {
      context->SetException(exception);
    } else {
      ThrowGlobalError(exception);
    }
  }

  NPP np_instance_;
  NPObject *global_object_;

 private:
  NPObject *GetEvaluator() {
    if (evaluator_.get())
      return evaluator_.get();

    // Wierd Safari bug: if you remove the surrounding parenthesis, this ceases
    // to work.
    const char kEvaluatorScript[] = 
      "(function (fn, args) {"
      "   var result = {};"
      "   try {"
      "     result.retval = fn.apply(null, args);"
      "   } catch (e) {"
      "     result.exception = String(e);"
      "   }"
      "   return result;"
      " })";
    NPObject *global = GetGlobalObject();
    NPString np_script = { kEvaluatorScript, ARRAYSIZE(kEvaluatorScript) - 1};
    ScopedNPVariant evaluator;
    if (!NPN_Evaluate(GetContext(), global, &np_script, &evaluator) ||
        !NPVARIANT_IS_OBJECT(evaluator)) {
      assert(false);
      return NULL;
    }

    evaluator_.reset(NPVARIANT_TO_OBJECT(evaluator));
    evaluator.Release();  // give ownership to evaluator_.
    return evaluator_.get();
  }

  // Creates an object by evaluating a string and getting the return value.
  JsObject *NewObjectImpl(const std::string &string_to_eval) {
    NPObject *global_object = GetGlobalObject();
    if (!global_object) {
      LOG(("Could not get global object from script engine."));
      return NULL;
    }
    // Evaluate javascript code: 'ConstructorName()'
    NPString script = {string_to_eval.c_str(), string_to_eval.length()};
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

  std::set<JsEventHandlerInterface *> event_handlers_[MAX_JSEVENTS];
  ScopedNPObject evaluator_;
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
  DocumentJsRunner(NPP instance) : JsRunnerBase(instance) {
    RegisterDocumentJsRunner(np_instance_, this);
  }

  virtual ~DocumentJsRunner() {
    // TODO(mpcomplete): This never gets called.  When should we delete the
    // DocumentJsRunner?
    UnregisterDocumentJsRunner(np_instance_);
  }

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
    return EvalImpl(script, true);
  }

  void SetErrorHandler(JsErrorHandlerInterface *handler) {
    assert(false); // Should not be called on the DocumentJsRunner.
  }

  void HandleNPInstanceDestroyed() {
    SendEvent(JSEVENT_UNLOAD);
    UnregisterDocumentJsRunner(np_instance_);
  }

  // TODO(mpcomplete): We only support JSEVENT_UNLOAD.  We should rework this
  // API to make it non-generic, and implement unload similar to NPAPI in
  // other browsers.
  bool AddEventHandler(JsEventType event_type,
                       JsEventHandlerInterface *handler) {
    return JsRunnerBase::AddEventHandler(event_type, handler);
  }

 private:
  scoped_ptr<HtmlEventMonitor> unload_monitor_;  // For 'onunload' notifications
  DISALLOW_EVIL_CONSTRUCTORS(DocumentJsRunner);
};


void NotifyNPInstanceDestroyed(NPP instance) {
  if (!g_document_js_runners)
    return;

  DocumentJsRunnerList::iterator it = g_document_js_runners->find(instance);
  if (it != g_document_js_runners->end()) {
    DocumentJsRunner *js_runner = it->second;
    js_runner->HandleNPInstanceDestroyed();
  }
}

JsRunnerInterface *NewJsRunner() {
  return static_cast<JsRunnerInterface *>(new JsRunner());
}

JsRunnerInterface *NewDocumentJsRunner(IGeneric *base, JsContextPtr context) {
  return static_cast<JsRunnerInterface *>(new DocumentJsRunner(context));
}
