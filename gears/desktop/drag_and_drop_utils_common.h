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

#ifndef GEARS_DESKTOP_DRAG_AND_DROP_UTILS_COMMON_H__
#define GEARS_DESKTOP_DRAG_AND_DROP_UTILS_COMMON_H__

#if BROWSER_FF || BROWSER_IE || BROWSER_SAFARI
  #define GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM 1
#elif BROWSER_CHROME && WIN32
  #define GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM 1
#endif

#if GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM


#if defined(WIN32)
#if BROWSER_IE
#define GEARS_DRAG_AND_DROP_USES_INTERCEPTOR 1
#elif BROWSER_FF
// Whilst GEARS_DRAG_AND_DROP_USES_INTERCEPTOR mostly works for Firefox/Win32,
// some bugs remain. In particular, if the gears_init.js script is invoked in
// the HTML HEAD, instead of the HTML BODY, then during the Gears Factory
// construction, the HWND that we want to intercept doesn't actually exist yet
// (and is probably generated when the <body> tag is parsed).
// The interceptor instead latches on to an incorrect HWND (since there are
// multiple matches to EnumChildWindows, and we're simply using last-one-wins,
// and we get weird behavior, such as the first drag-and-drop of files onto a
// web page works, but subsequent ones only yield the first DnD's files.
//
// Thus, on Firefox/Windows, we do not use a DropTargetInterceptor. Ideally,
// we would want to use one, since that would give us cursor control, but
// until we get the HWND selection 100% correct, especially in the script-in-
// head-tag case, then we don't use one. For now, of our two imperfect options,
// we take (1) the inability to call setDragCursor with anything other than
// 'none' or 'move' ('copy' and 'link' show 'move'), instead of (2) returning
// the wrong fileset from getDragData.
//
//#define GEARS_DRAG_AND_DROP_USES_INTERCEPTOR 1
#endif
#endif


#include <set>
#include <vector>
#include "gears/base/common/js_types.h"

enum DragAndDropCursorType {
  DRAG_AND_DROP_CURSOR_NONE,
  DRAG_AND_DROP_CURSOR_COPY,
  DRAG_AND_DROP_CURSOR_INVALID
};

enum DragAndDropEventType {
  DRAG_AND_DROP_EVENT_INVALID = 0,  // To match the equivalent in Chrome.
  DRAG_AND_DROP_EVENT_DRAGENTER,
  DRAG_AND_DROP_EVENT_DRAGOVER,
  DRAG_AND_DROP_EVENT_DRAGLEAVE,
  DRAG_AND_DROP_EVENT_DROP
};

enum DragAndDropFlavorType {
  DRAG_AND_DROP_FLAVOR_FILES,
  DRAG_AND_DROP_FLAVOR_TEXT,
  DRAG_AND_DROP_FLAVOR_URL,
  DRAG_AND_DROP_FLAVOR_INVALID
};

class FileDragAndDropMetaData {
 public:
  FileDragAndDropMetaData();

  bool IsEmpty();
  void Reset();
  void SetFilenames(std::vector<std::string16> &filenames);
  bool ToJsObject(ModuleEnvironment *module_environment,
                  bool is_in_a_drop_event,
                  JsObject *object_out,
                  std::string16 *error_out);

 private:
  std::vector<std::string16> filenames_;
  std::set<std::string16> extensions_;
  std::set<std::string16> mime_types_;
  int64 total_bytes_;

  // Setting has_files to true means that the clipboard had files on it,
  // whether or not those files were readable (e.g. they weren't directories).
  // If they weren't readable, then has_files_ will be true but filenames_
  // will be empty.
  bool has_files_;

  DISALLOW_EVIL_CONSTRUCTORS(FileDragAndDropMetaData);
};

#endif  // GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM
#endif  // GEARS_DESKTOP_DRAG_AND_DROP_UTILS_COMMON_H__

