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

#include "gears/base/npapi/plugin.h"

#include "gears/base/common/js_types.h"
#include "gears/base/npapi/browser_utils.h"

// static
void PluginBase::Deallocate(NPObject *npobj) {
  delete static_cast<PluginBase *>(npobj);
}

// static
bool PluginBase::HasMethod(NPObject *npobj, NPIdentifier name) {
  PluginBase *plugin = static_cast<PluginBase *>(npobj);
  assert(plugin->dispatcher_);
  return plugin->dispatcher_->HasMethod(name);
}

// static
bool PluginBase::Invoke(NPObject *npobj, NPIdentifier name,
                           const NPVariant *args, uint32_t num_args,
                           NPVariant *result) {
  VOID_TO_NPVARIANT(*result);

  PluginBase *plugin = static_cast<PluginBase *>(npobj);
  assert(plugin->dispatcher_);

  JsCallContext context(plugin->instance_, npobj,
                        static_cast<int>(num_args), args, result);
  BrowserUtils::EnterScope(&context);
  bool retval = plugin->dispatcher_->CallMethod(name, &context);
  BrowserUtils::ExitScope();
  return retval;
}

// static
bool PluginBase::HasProperty(NPObject *npobj, NPIdentifier name) {
  PluginBase *plugin = static_cast<PluginBase *>(npobj);
  assert(plugin->dispatcher_);
  return plugin->dispatcher_->HasPropertyGetter(name) ||
      plugin->dispatcher_->HasPropertySetter(name);
}

// static
bool PluginBase::GetProperty(NPObject *npobj, NPIdentifier name,
                                NPVariant *result) {
  VOID_TO_NPVARIANT(*result);

  PluginBase *plugin = static_cast<PluginBase *>(npobj);
  assert(plugin->dispatcher_);

  JsCallContext context(plugin->instance_, npobj, 0, NULL, result);
  BrowserUtils::EnterScope(&context);
  bool retval = plugin->dispatcher_->GetProperty(name, &context);
  BrowserUtils::ExitScope();
  return retval;
}

// static
bool PluginBase::SetProperty(NPObject *npobj, NPIdentifier name,
                                const NPVariant *value) {
  PluginBase *plugin = static_cast<PluginBase *>(npobj);
  assert(plugin->dispatcher_);

  JsCallContext context(plugin->instance_, npobj, 1, value, NULL);
  BrowserUtils::EnterScope(&context);
  bool retval = plugin->dispatcher_->SetProperty(name, &context);
  BrowserUtils::ExitScope();
  return retval;
}
