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

#include <vector>

#include "gears/base/common/base_class.h"

// Name/ID pair used for JavaScript properties and methods.
struct NamedIdentifier {
  const char* name;
  NPIdentifier id;

  NamedIdentifier(const char* name) : name(name), id(0) {}
};

// This is a base class for the bridge between the JavaScript engine and the
// Gears class implementations.  See GearsFactoryPlugin for an example.
class PluginBase : public NPObject {
 public:
  // Get a NPClass for a Plugin type (the type must derive from PluginBase).
  template<class Plugin>
  static NPClass* GetClass();

  // NPClass callbacks.  Most of these will simply call through to the virtual
  // versions below.
  static void ClassDeallocate(NPObject *npobj);
  static void ClassInvalidate(NPObject *npobj);
  static bool ClassHasMethod(NPObject *npobj, NPIdentifier name);
  static bool ClassInvoke(NPObject *npobj, NPIdentifier name,
                          const NPVariant *args, uint32_t num_args,
                          NPVariant *result);
  static bool ClassInvokeDefault(NPObject *npobj, const NPVariant *args,
                                 uint32_t num_args, NPVariant *result);
  static bool ClassHasProperty(NPObject * npobj, NPIdentifier name);
  static bool ClassGetProperty(NPObject *npobj, NPIdentifier name,
                               NPVariant *result);
  static bool ClassSetProperty(NPObject *npobj, NPIdentifier name,
                               const NPVariant *value);
  static bool ClassRemoveProperty(NPObject *npobj, NPIdentifier name);

  PluginBase(NPP instance);
  virtual ~PluginBase();

  // Overrideable versions of the NPClass callbacks.
  virtual void Invalidate();
  virtual bool Invoke(NPIdentifier name, const NPVariant *args,
                      uint32_t num_args, NPVariant *result);
  virtual bool InvokeDefault(const NPVariant *args, uint32_t num_args,
                             NPVariant *result);
  virtual bool GetProperty(NPIdentifier name, NPVariant *result);
  virtual bool SetProperty(NPIdentifier name, const NPVariant *value);
  virtual bool RemoveProperty(NPIdentifier name);

  virtual int HandleEvent(void* event) { return 0; }

 protected:
  NPP instance_;

  typedef std::vector<NPIdentifier> IDList;

  // Derived classes should override these methods so that each class has its
  // own static instance of the property and method lists.
  virtual IDList& GetPropertyList() = 0;
  virtual IDList& GetMethodList() = 0;

  // Register JavaScript property/methods.
  void RegisterProperty(NamedIdentifier* id);
  void RegisterMethod(NamedIdentifier* id);
};

// static
template<class Plugin>
NPClass* PluginBase::GetClass() {
  static NPClass plugin_class = {
    NP_CLASS_STRUCT_VERSION,
    Plugin::ClassAllocate,
    Plugin::ClassDeallocate,
    Plugin::ClassInvalidate,
    Plugin::ClassHasMethod,
    Plugin::ClassInvoke,
    Plugin::ClassInvokeDefault,
    Plugin::ClassHasProperty,
    Plugin::ClassGetProperty,
    Plugin::ClassSetProperty,
    Plugin::ClassRemoveProperty
  };

  return &plugin_class;
}

#endif // GEARS_BASE_NPAPI_PLUGIN_H__
