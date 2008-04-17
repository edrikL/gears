// Copyright 2008, Google Inc.
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

#ifdef DEBUG

#ifndef GEARS_CCTESTS_TEST_H__
#define GEARS_CCTESTS_TEST_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

class GearsTest : public ModuleImplBaseClassVirtual {
 public:
   GearsTest() : ModuleImplBaseClassVirtual("GearsTest") {}

  // IN: nothing
  // OUT: bool
  void RunTests(JsCallContext *context);


  // JsObject and JsArray tests

  // IN: object value
  // OUT: void
  // throws exception on failure
  void TestPassObject(JsCallContext *context);

  // IN: function test_function
  // OUT: void
  // throws exception on failure
  void TestCreateObject(JsCallContext *context);


  // Argument passing tests

  // IN: bool bool_value, int int_value, int64 int64_value, double double_value,
  //     string string_value
  // OUT: void
  void TestPassArguments(JsCallContext *context);

  // IN: int value1, optional int value2, optional int value3
  // OUT: void
  void TestPassArgumentsOptional(JsCallContext *context);


  // Coercion tests

  // IN: variant value, bool expected_value
  // OUT: bool
  void TestCoerceBool(JsCallContext *context);
  // IN: variant value, int expected_value
  // OUT: bool
  void TestCoerceInt(JsCallContext *context);
  // IN: variant value, double expected_value
  // OUT: bool
  void TestCoerceDouble(JsCallContext *context);
  // IN: variant value, string expected_value
  // OUT: bool
  void TestCoerceString(JsCallContext *context);
  // IN: string type, variant value
  // OUT: bool
  void TestGetType(JsCallContext *context);

  // Module creation test

  // IN: void
  // OUT: module
  void TestCreateModule(JsCallContext *context);

 private:

  DISALLOW_EVIL_CONSTRUCTORS(GearsTest);
};

// Dispatcher module that is created by GearsTest::TestPassModule()
class GearsTestModule
    : public ModuleImplBaseClassVirtual,
      public JsEventHandlerInterface {
 public:
  GearsTestModule() : ModuleImplBaseClassVirtual("GearsTestModule") {}

  // IN: callback
  // OUT: void
  void TestSetCallback(JsCallContext *context);
  // to handle the unload event
  virtual void HandleEvent(JsEventType event_type);
 private:
  ~GearsTestModule();
  void RegisterUnloadHandler();
  scoped_ptr<JsEventMonitor> unload_monitor_;
  scoped_ptr<JsRootedCallback> callback_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsTestModule);
};

#endif // GEARS_CCTESTS_TEST_H__

#endif // DEBUG
