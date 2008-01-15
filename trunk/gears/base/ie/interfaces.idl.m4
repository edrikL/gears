// Copyright 2006, Google Inc.
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
// Instructions for defining a new interface:
// - Generate a GUID; this will be your interface GUID.
// - Copy an existing module's ".idl" file. Replace 'uuid' with your interface
//     GUID. Update the interface name. Define your properties/methods.
//
// If your interface needs to be returned as IDispatch instead of FooInterface:
// - Generate a second GUID; this will be your class GUID.
// - Copy an appropriate "uuid ... coclass" block below. Replace 'uuid' with
//     your class GUID. Update the interface name.
//
// Additionally, if you want to let callers use "new ActiveXObject" to
// instantiate this interface:
// - Copy an existing module's ".rgs" file. Replace the CLSID values with your
//     class GUID. Update the inteface name. But leave the TypeLib value as it
//     is. (There is only one type library for all our interfaces; see below.)
// - Update your class definition, adding the DECLARE_REGISTRY_RESOURCEID and
//     OBJECT_ENTRY_AUTO macros, and adding CLSID_Foo as the second parameter
//     to CComCoClass.  (See GearsFactory for an example.)

import "oaidl.idl";
import "ocidl.idl";

import "ui/ie/html_dialog_host.idl";

import "channel/ie/channel.idl";
import "console/ie/console.idl";
import "database/ie/database.idl";
import "desktop/desktop_ie.idl";
import "factory/ie/factory.idl";
import "httprequest/ie/httprequest.idl";
import "localserver/ie/localserver.idl";
import "timer/timer_ie.idl";
import "workerpool/ie/workerpool.idl";

#ifdef DEBUG
import "cctests/test_ie.idl";
#endif DEBUG

//------------------------------------------------------------------------------
// GearsTypelib
//------------------------------------------------------------------------------
[
  uuid(7708913A-B86C-4D91-B325-657DD5363433),
  version(1.0)
]
library GearsTypelib
{
  importlib("stdole2.tlb");

  [
    uuid(E0FEFE40-FBF9-42AE-BA58-794CA7E3FB53)
  ]
  coclass BrowserHelperObject
  {
    [default] interface IUnknown;
  };

  [
    uuid(619C4FDA-4D52-4C7C-BAF2-5654DA16E675)
  ]
  coclass HtmlDialogHost
  {
    [default] interface HtmlDialogHostInterface;
  };

  [
    uuid(0B4350D1-055F-47A3-B112-5F2F2B0D6F08)
  ]
  coclass ToolsMenuItem
  {
    [default] interface IUnknown;
  };
  
  [
    uuid(09371E80-6AB5-4341-81E8-BFF3FB8CC749)
  ]
  coclass ModuleWrapper
  {
    [default] interface IDispatch;
  };

  // TODO(aa): We should be able to remove most of this COM goop once we
  // implement our own dynamic dispatch in ModuleImplBaseClass. We should test
  // that really carefully though, on a clean machine.
  // NOTE: We might need to keep GearsFactory a little longer than the others to
  // maintain compatibility with existing gears_init.js scripts, which call
  // new ActiveXObject("Gears.Factory"). Later, when we inject gears objects
  // without gears_init.js, we can remove that too.
  [
    uuid(C93A7319-17B3-4504-87CD-03EFC6103E6E)
  ]
  coclass GearsFactory
  {
    [default] interface GearsFactoryInterface;
  };

  [
    uuid(51C2DE73-6A33-4975-8D7D-C521064F8A83)
  ]
  coclass GearsConsole
  {
    [default] interface GearsConsoleInterface;
  };

  [
    uuid(B09AFBD8-FBEE-4E91-AA27-7DC433C978AB)
  ]
  coclass GearsDatabase
  {
    [default] interface GearsDatabaseInterface;
  };
  
  [
    uuid(6761C0EC-BB5C-40fe-92B2-D41686A0CF7E)
  ]
  coclass GearsDesktop
  {
    [default] interface GearsDesktopInterface;
  };
  
#ifdef DEBUG
  [
    uuid(F98360E4-ECE5-4177-971F-639A0A6D353E)
  ]
  coclass GearsTest
  {
    [default] interface GearsTestInterface;
  };
#endif DEBUG

  [
    uuid(B76AFB62-9BA2-43e8-B27F-9F1CAC8148B7)
  ]
  coclass GearsWorkerPool
  {
    [default] interface GearsWorkerPoolInterface;
  };

  [
    uuid(3A826505-92E3-486a-9FB5-37FE89E971F9)
  ]
  coclass GearsLocalServer
  {
    [default] interface GearsLocalServerInterface;
  };

  [
    uuid(D056D8FA-05D8-4575-903F-180C85D2C318)
  ]
  coclass GearsTimer
  {
    [default] interface GearsTimerInterface;
  };

  [
    uuid(AAF5DBC9-70C8-45c2-B7AB-6576428F3CA3)
  ]
  coclass GearsHttpRequest
  {
    [default] interface GearsHttpRequestInterface;
  };

  [
    uuid(3DE5BB83-50C4-4F73-AA69-6DE89C65C1AE)
  ]
  coclass GearsChannel
  {
    [default] interface GearsChannelInterface;
  };

};
