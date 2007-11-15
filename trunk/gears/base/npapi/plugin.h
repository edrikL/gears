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

#ifndef GEARS_BASE_NPAPI_PLUGIN_H__
#define GEARS_BASE_NPAPI_PLUGIN_H__

#include <map>

#include "gears/base/common/base_class.h"

// Associates a Gears class with its corresponding NPAPI bridge class.
template<class Plugin> struct PluginTraits {};

// This is a base class for the bridge between the JavaScript engine and the
// Gears class implementations.  See GearsFactoryBridge for an example.
//
// Note: This class assumes its template parameter has the following methods:
// // Called once per class type to initialize properties and methods.
// static void InitClass();
// // Returns a pointer to the class that handles the property/method callbacks.
// ImplClass *GetImplObject();
template<class T>
class PluginBase : public NPObject {
 public:
  // The NPAPI scriptable plugin interface class.
  typedef T PluginClass;

  // The corresponding ImplClass that implements the functionality for our
  // bridge.
  typedef typename PluginTraits<T>::ImplClass ImplClass;

  // Callback function used for property and method invokations.
  typedef void (ImplClass::*ImplCallback)();

  // NPClass callbacks.  The browser calls these functions when JavaScript
  // interacts with a Gears object.
  static NPObject* Allocate(NPP npp, NPClass *npclass);
  static void Deallocate(NPObject *npobj);
  static bool HasMethod(NPObject *npobj, NPIdentifier name);
  static bool Invoke(NPObject *npobj, NPIdentifier name,
                     const NPVariant *args, uint32_t num_args,
                     NPVariant *result);
  static bool HasProperty(NPObject * npobj, NPIdentifier name);
  static bool GetProperty(NPObject *npobj, NPIdentifier name,
                          NPVariant *result);

  PluginBase(NPP instance) : instance_(instance) {}

 protected:
  // Register JavaScript property/methods.
  static void RegisterProperty(const char *name, ImplCallback callback);
  static void RegisterMethod(const char *name, ImplCallback callback);

  NPP instance_;

 private:
  typedef std::map<NPIdentifier, ImplCallback> IDList;

  static IDList& GetPropertyList() {
    static IDList properties;
    return properties;
  }
  static IDList& GetMethodList() {
    static IDList methods;
    return methods;
  }

  DISALLOW_EVIL_CONSTRUCTORS(PluginBase<T>);
};

// Get the NPClass for a Plugin type (the type must derive from PluginBase).
template<class Plugin>
NPClass* GetNPClass() {
  static NPClass plugin_class = {
    NP_CLASS_STRUCT_VERSION,
    Plugin::Allocate,
    Plugin::Deallocate,
    NULL,  // Plugin::Invalidate,
    Plugin::HasMethod,
    Plugin::Invoke,
    NULL,  // Plugin::InvokeDefault,
    Plugin::HasProperty,
    Plugin::GetProperty,
    NULL,  // Plugin::SetProperty,
    NULL,  // Plugin::RemoveProperty,
  };

  return &plugin_class;
}

// Used to define the association between a Gears class and its NPAPI bridge.
#define DECLARE_GEARS_BRIDGE(ImplClassType, PluginClass) \
class PluginClass; \
template<> \
struct PluginTraits<PluginClass> { \
  typedef ImplClassType ImplClass; \
}

// Need to include .cc for template definitions.
#include "gears/base/npapi/plugin.cc"

#endif // GEARS_BASE_NPAPI_PLUGIN_H__
