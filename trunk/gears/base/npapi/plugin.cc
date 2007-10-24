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

#include "plugin.h"

// static
void PluginBase::ClassDeallocate(NPObject *npobj) {
  PluginBase* plugin = static_cast<PluginBase *>(npobj);
  delete plugin;
}

// static
void PluginBase::ClassInvalidate(NPObject *npobj) {
  PluginBase* plugin = static_cast<PluginBase *>(npobj);
  plugin->Invalidate();
}

// static
bool PluginBase::ClassHasMethod(NPObject *npobj, NPIdentifier name) {
  PluginBase* plugin = static_cast<PluginBase *>(npobj);
  const IDList& methods = plugin->GetMethodList();
  for (size_t i = 0; i < methods.size(); ++i) {
    if (methods[i] == name)
      return true;
  }

  return false;
}

// static
bool PluginBase::ClassInvoke(NPObject *npobj, NPIdentifier name,
                             const NPVariant *args, uint32_t num_args,
                             NPVariant *result) {
  PluginBase* plugin = static_cast<PluginBase *>(npobj);
  return plugin->Invoke(name, args, num_args, result);
}

// static
bool PluginBase::ClassInvokeDefault(NPObject *npobj, const NPVariant *args,
                                    uint32_t num_args, NPVariant *result) {
  PluginBase* plugin = static_cast<PluginBase *>(npobj);
  return plugin->InvokeDefault(args, num_args, result);
}

// static
bool PluginBase::ClassHasProperty(NPObject * npobj, NPIdentifier name) {
  PluginBase* plugin = static_cast<PluginBase *>(npobj);
  const IDList& properties = plugin->GetPropertyList();
  for (size_t i = 0; i < properties.size(); ++i) {
    if (properties[i] == name)
      return true;
  }

  return false;
}

// static
bool PluginBase::ClassGetProperty(NPObject *npobj, NPIdentifier name,
                                  NPVariant *result) {
  PluginBase* plugin = static_cast<PluginBase *>(npobj);
  return plugin->GetProperty(name, result);
}

// static
bool PluginBase::ClassSetProperty(NPObject *npobj, NPIdentifier name,
                                  const NPVariant *value) {
  PluginBase* plugin = static_cast<PluginBase *>(npobj);
  return plugin->SetProperty(name, value);
}

// static
bool PluginBase::ClassRemoveProperty(NPObject *npobj, NPIdentifier name) {
  PluginBase* plugin = static_cast<PluginBase *>(npobj);
  return plugin->RemoveProperty(name);
}


PluginBase::PluginBase(NPP pNPInstance) :
    instance_(pNPInstance)
{
}

PluginBase::~PluginBase()
{
}

void PluginBase::Invalidate() {
}

bool PluginBase::Invoke(NPIdentifier name, const NPVariant *args,
                        uint32_t num_args, NPVariant *result) {
  return false;
}

bool PluginBase::InvokeDefault(const NPVariant *args,
                               uint32_t num_args, NPVariant *result) {
  return false;
}

bool PluginBase::GetProperty(NPIdentifier name, NPVariant *result) {
  return false;
}

bool PluginBase::SetProperty(NPIdentifier name, const NPVariant *value) {
  return false;
}

bool PluginBase::RemoveProperty(NPIdentifier name) {
  return false;
}


void PluginBase::RegisterProperty(NamedIdentifier* id) {
  id->id = NPN_GetStringIdentifier(id->name);
  GetPropertyList().push_back(id->id);
}

void PluginBase::RegisterMethod(NamedIdentifier* id) {
  id->id = NPN_GetStringIdentifier(id->name);
  GetMethodList().push_back(id->id);
}