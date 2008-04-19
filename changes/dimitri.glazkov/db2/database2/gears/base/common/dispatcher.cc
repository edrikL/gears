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

// Don't actually include dispatcher.h, since this is a template file and will
// be included by the .h file.  If we do, mkdepend.py explodes in a whirlwind of
// angry recursion as it tries to follow the infinite include chain.

#include "gears/base/common/js_types.h"
#include "gears/base/common/thread_locals.h"

// Returns the DispatchId associated with the given string.  Used for looking
// up methods and properties.
inline DispatchId GetStringIdentifier(const char *str) {
#if BROWSER_NPAPI
  return reinterpret_cast<DispatchId>(NPN_GetStringIdentifier(str));
#else
  // TODO(mpcomplete): Figure out what we need for other ports.
  // This only works if str is a static string.
  return reinterpret_cast<DispatchId>(const_cast<char *>(str));
#endif
}

template<class T>
Dispatcher<T>::Dispatcher(ImplClass *impl) : impl_(impl) {
  // Ensure that property and method mappings are initialized.
  ThreadLocalVariables &locals = GetThreadLocals();
  if (!locals.did_init_class) {
    locals.did_init_class = true;
    Init();
  }
}

template<class T>
bool Dispatcher<T>::HasMethod(DispatchId method_id) {
  const IDList &methods = GetMethodList();
  return methods.find(method_id) != methods.end();
}

template<class T>
bool Dispatcher<T>::HasPropertyGetter(DispatchId property_id) {
  const IDList &properties = GetPropertyGetterList();
  return properties.find(property_id) != properties.end();
}

template<class T>
bool Dispatcher<T>::HasPropertySetter(DispatchId property_id) {
  const IDList &properties = GetPropertySetterList();
  return properties.find(property_id) != properties.end();
}

template<class T>
bool Dispatcher<T>::CallMethod(DispatchId method_id, JsCallContext *context) {
  const IDList &methods = GetMethodList();
  typename IDList::const_iterator method = methods.find(method_id);
  if (method == methods.end())
    return false;
  ImplCallback callback = method->second;

  (impl_->*callback)(context);
  return true;
}

template<class T>
bool Dispatcher<T>::GetProperty(DispatchId property_id,
                                JsCallContext *context) {
  const IDList &properties = GetPropertyGetterList();
  typename IDList::const_iterator property = properties.find(property_id);
  if (property == properties.end())
    return false;
  ImplCallback callback = property->second;

  (impl_->*callback)(context);
  return true;
}

template<class T>
bool Dispatcher<T>::SetProperty(DispatchId property_id,
                                JsCallContext *context) {
  const IDList &properties = GetPropertySetterList();
  typename IDList::const_iterator property = properties.find(property_id);
  if (property == properties.end()) {
    return false;
  }
  ImplCallback callback = property->second;
  if (callback == NULL) { // Read only property.
    context->SetException(
                 STRING16(L"Cannot assign value to a read only property."));
    return true;
  }

  (impl_->*callback)(context);
  return true;
}

template<class T>
const DispatcherNameList &Dispatcher<T>::GetMemberNames() {
  return GetThreadLocals().members;
}

template<class T>
DispatchId Dispatcher<T>::GetDispatchId(const std::string &member_name) {
  DispatcherNameList member_names = GetMemberNames();
  DispatcherNameList::iterator result = member_names.find(member_name);
  if (result != member_names.end()) {
    return result->second;
  } else {
    return NULL;
  }
}

// static
template<class T>
void Dispatcher<T>::RegisterProperty(const char *name,
                                     ImplCallback getter, ImplCallback setter) {
  assert(getter);
  DispatchId id = GetStringIdentifier(name);
  GetPropertyGetterList()[id] = getter;
  GetPropertySetterList()[id] = setter;
  GetThreadLocals().members[name] = id;
}

// static
template<class T>
void Dispatcher<T>::RegisterMethod(const char *name, ImplCallback callback) {
  DispatchId id = GetStringIdentifier(name);
  GetMethodList()[id] = callback;
  GetThreadLocals().members[name] = id;
}

// static
template<class T>
void Dispatcher<T>::DeleteThreadLocals(void *context) {
  ThreadLocalVariables *locals =
      reinterpret_cast<ThreadLocalVariables*>(context);
  delete locals;
}

// static
template<class T>
typename Dispatcher<T>::ThreadLocalVariables &Dispatcher<T>::GetThreadLocals() {
  const std::string &key = kThreadLocalsKey;
  ThreadLocalVariables *locals =
      reinterpret_cast<ThreadLocalVariables*>(ThreadLocals::GetValue(key));
  if (!locals) {
    locals = new ThreadLocalVariables;
    ThreadLocals::SetValue(key, locals, &DeleteThreadLocals);
  }
  return *locals;
}
