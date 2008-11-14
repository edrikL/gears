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

#ifndef GEARS_BASE_ANDROID_JAVA_EXCEPTION_SCOPE_H__
#define GEARS_BASE_ANDROID_JAVA_EXCEPTION_SCOPE_H__

// Java exception scope wrapper. This is used to reset any uncaught
// exceptions that may have been throwed during the lifetime of this
// object. On destruction, any exceptions are cleared. JNI will
// complain and possibly abort if we call another method while an
// exception is raised. Sample usage:
//   {
//     JavaExceptionScope ex;
//     bool ok = env->CallBooleanMethod(...); // throws an exception
//     if (ok)
//       ...
//     // Exception is cleared when leaving the block.
//   }
class JavaExceptionScope {
 public:
  JavaExceptionScope() {
    // Work around compiler warning about this object being unused.
    (void) this;
  }

  // The destructor logs a message if an exception is pending. It also
  // clears the exception to allow future JNI calls.
  ~JavaExceptionScope();
  // Returns true if an exception occurred. No side-effect.
  static bool Occurred();
  // Check and clear any thrown exception. Returns true if an
  // exception occurred. Also prints a LOG message when an exception
  // occurs for convenience.
  static bool Clear();
};

#endif // GEARS_BASE_ANDROID_JAVA_EXCEPTION_SCOPE_H__
