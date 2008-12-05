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

#ifndef GEARS_DESKTOP_DRAG_AND_DROP_UTILS_SF_H__
#define GEARS_DESKTOP_DRAG_AND_DROP_UTILS_SF_H__

#include "gears/base/common/base_class.h"

// Gears Drag and Drop needs to get access to more than what regular JavaScript
// driven drag and drop can see (and hence, what JavaScript via the NPAPI
// interface can see). In particular, for file drag and drop, this includes
// things like the list of filenames during drag enter/over events (not just
// on drop events) and also whether or not we are in a genuine (i.e. OS-level)
// drag and drop event, and not a programatically generated, synthetic event.
// In order to get access, we intercept certain methods on the WebView class.
// We can do this (via a technique called "Method Swizzling") since a
// WebView is an Objective-C class, not a C++ class, and we can re-wire what
// code gets run in response to drag-related messages (i.e. selectors, in
// Objective-C parlance).
// This SwizzleWebViewMethods function sets up that interception, and the
// other functions in this file are more or less getters for what we learn
// when those intercepts are triggered.
// Generallly speaking, method swizzling is a bit of a hack that could
// destabilize the application, but our swizzled methods only track state,
// they always call the original function, and the don't modify the return
// values. See the .mm implementation file for a list of what methods we
// actually swizzle.
bool SwizzleWebViewMethods();

bool GetDroppedFiles(ModuleEnvironment *module_environment,
                     JsArray *files_out,
                     std::string16 *error_out);

// These two are mutually exclusive (although they may be both false) -
// "in a drag" means dragenter, dragover or dragleave, but not drop.
bool IsInADragOperation();
bool IsInADropOperation();

void AcceptDrag(ModuleEnvironment *module_environment,
                JsObject *event,
                bool acceptance,
                std::string16 *error_out);

void GetDragData(ModuleEnvironment *module_environment,
                 JsObject *event,
                 JsObject *data_out,
                 std::string16 *error_out);

#endif  // GEARS_DESKTOP_DRAG_AND_DROP_UTILS_SF_H__
