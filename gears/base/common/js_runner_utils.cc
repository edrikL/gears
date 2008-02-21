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
#ifdef WINCE
// This function is implemented in wince_compatibility.cc!
#else
#include "gears/base/common/js_runner_utils.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"

void ThrowGlobalError(JsRunnerInterface *js_runner,
                      const std::string16 &message) {
  if (!js_runner) { return; }

  std::string16 string_to_eval(message);

  ReplaceAll(string_to_eval, std::string16(STRING16(L"'")),
            std::string16(STRING16(L"\\'")));
  ReplaceAll(string_to_eval, std::string16(STRING16(L"\r")),
            std::string16(STRING16(L"\\r")));
  ReplaceAll(string_to_eval, std::string16(STRING16(L"\n")),
            std::string16(STRING16(L"\\n")));

  string_to_eval.insert(0, std::string16(STRING16(L"throw new Error('")));
  string_to_eval.append(std::string16(STRING16(L"')")));

  js_runner->Eval(string_to_eval.c_str());
}
#endif  // WINCE
