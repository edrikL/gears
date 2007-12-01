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

// Don't actually include plugin.h, since this is a template file and will be
// included by the .h file.  If we do, mkdepend.py explodes in a whirlwind of
// angry recursion as it tries to follow the infinite include chain.

#include "gears/base/npapi/browser_utils.h"
#include "gears/base/common/js_types.h"

// static
template<class T>
NPObject* PluginBase<T>::Allocate(NPP npp, NPClass *npclass) {
  // Initialize property and method mappings for the derived class.
  static bool did_init = false;
  if (!did_init) {
    did_init = true;
    PluginClass::InitClass();
  }

  return new PluginClass(npp);
}

// static
template<class T>
void PluginBase<T>::Deallocate(NPObject *npobj) {
  delete static_cast<PluginClass *>(npobj);
}

// static
template<class T>
bool PluginBase<T>::HasMethod(NPObject *npobj, NPIdentifier name) {
  const IDList &methods = GetMethodList();
  return methods.find(name) != methods.end();
}

// static
template<class T>
bool PluginBase<T>::Invoke(NPObject *npobj, NPIdentifier name,
                           const NPVariant *args, uint32_t num_args,
                           NPVariant *result) {
  PluginClass *plugin = static_cast<PluginClass *>(npobj);
  ImplClass *impl = plugin->GetImplObject();

  const IDList &methods = GetMethodList();
  IDList::const_iterator method = methods.find(name);
  if (method == methods.end())
    return false;
  ImplCallback callback = method->second;

  JsCallContext context(plugin->instance_, npobj,
                        static_cast<int>(num_args), args, result);
  BrowserUtils::EnterScope(&context);
  (impl->*callback)(&context);
  BrowserUtils::ExitScope();
  return true;
}

// static
template<class T>
bool PluginBase<T>::HasProperty(NPObject * npobj, NPIdentifier name) {
  const IDList &properties = GetPropertyGetterList();
  return properties.find(name) != properties.end();
}

// static
template<class T>
bool PluginBase<T>::GetProperty(NPObject *npobj, NPIdentifier name,
                                NPVariant *result) {
  PluginClass *plugin = static_cast<PluginClass *>(npobj);
  ImplClass *impl = plugin->GetImplObject();

  const IDList &properties = GetPropertyGetterList();
  IDList::const_iterator property = properties.find(name);
  if (property == properties.end())
    return false;
  ImplCallback callback = property->second;

  JsCallContext context(plugin->instance_, npobj, 0, NULL, result);
  BrowserUtils::EnterScope(&context);
  (impl->*callback)(&context);
  BrowserUtils::ExitScope();
  return true;
}

// static
template<class T>
bool PluginBase<T>::SetProperty(NPObject *npobj, NPIdentifier name,
                                const NPVariant *value) {
  PluginClass *plugin = static_cast<PluginClass *>(npobj);
  ImplClass *impl = plugin->GetImplObject();

  const IDList &properties = GetPropertySetterList();
  IDList::const_iterator property = properties.find(name);
  if (property == properties.end() || property->second == NULL)
    return false;
  ImplCallback callback = property->second;

  JsCallContext context(plugin->instance_, npobj, 1, value, NULL);
  BrowserUtils::EnterScope(&context);
  (impl->*callback)(&context);
  BrowserUtils::ExitScope();
  return true;
}

// static
template<class T>
void PluginBase<T>::RegisterProperty(const char *name,
                                     ImplCallback getter, ImplCallback setter) {
  assert(getter);
  NPIdentifier id = NPN_GetStringIdentifier(name);
  GetPropertyGetterList()[id] = getter;
  GetPropertySetterList()[id] = setter;
}

// static
template<class T>
void PluginBase<T>::RegisterMethod(const char *name, ImplCallback callback) {
  NPIdentifier id = NPN_GetStringIdentifier(name);
  GetMethodList()[id] = callback;
}
