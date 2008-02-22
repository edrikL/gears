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
  void TestPassObject(JsCallContext* context);

  // IN: function test_function
  // OUT: void
  // throws exception on failure
  void TestCreateObject(JsCallContext* context);


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

 private:

  DISALLOW_EVIL_CONSTRUCTORS(GearsTest);
};

#endif // GEARS_CCTESTS_TEST_H__

#endif // DEBUG
