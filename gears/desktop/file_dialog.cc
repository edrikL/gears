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

#ifdef OFFICIAL_BUILD
// File picker is not ready for official builds
#else

#include "gears/desktop/file_dialog.h"

#include "gears/desktop/file_dialog_gtk.h"
#include "gears/desktop/file_dialog_osx.h"
#include "gears/desktop/file_dialog_win32.h"

#if BROWSER_FF
#include "gears/base/firefox/dom_utils.h"
#elif BROWSER_IE
#include "gears/base/ie/activex_utils.h"
#include "gears/base/common/base_class.h"
#endif

#if defined(LINUX) && !defined(OS_MACOSX)

static FileDialog* NewFileDialogGtk(const FileDialog::Mode mode) {
  // If the parent is null the the dialog will not be modal.
  // The user will still be able interact with the browser window.
  GtkWindow* parent = NULL;

#if BROWSER_FF
  if (NS_FAILED(DOMUtils::GetNativeWindow(&parent)))
    return NULL;
#endif

  FileDialog* dialog = NULL;

  switch (mode) {
    case FileDialog::MULTIPLE_FILES:
      dialog = new FileDialogGtk(parent);
      break;

    default:
      dialog = NULL;
      break;
  }

  return dialog;
}

#elif defined(WIN32)

static FileDialog* NewFileDialogWin32(const FileDialog::Mode mode,
                                      const ModuleImplBaseClass& module) {
  // If the parent is null the the dialog will not be modal.
  // The user will still be able interact with the browser window.
  HWND parent = NULL;

#if BROWSER_FF

  if (NS_FAILED(DOMUtils::GetNativeWindow(&parent)))
    return NULL;

#elif BROWSER_IE

  IWebBrowser2* web_browser = NULL;
  HRESULT hr = ActiveXUtils::GetWebBrowser2(module.EnvPageIUnknownSite(),
                                            &web_browser);
  if (FAILED(hr))
    return NULL;

  hr = web_browser->get_HWND(reinterpret_cast<long*>(&parent));
  if (FAILED(hr)) {
    web_browser->Release();
    return NULL;
  }

  web_browser->Release();

#endif  // BROWSER_xyz

  FileDialog* dialog = NULL;

  switch (mode) {
    case FileDialog::MULTIPLE_FILES:
      dialog = new FileDialogWin32(parent);
      break;

    default:
      dialog = NULL;
      break;
  }

  return dialog;
}

#elif defined(OS_MACOSX)

static FileDialog* NewFileDialogCarbon(const FileDialog::Mode mode) {
  FileDialog* dialog = NULL;

  switch (mode) {
    case FileDialog::MULTIPLE_FILES:
      dialog = new FileDialogCarbon();
      break;

    default:
      dialog = NULL;
      break;
  }

  return dialog;
}

#endif  // PLATFORM_xyz

FileDialog* NewFileDialog(const FileDialog::Mode mode,
                          const ModuleImplBaseClass& module) {
#if defined(LINUX) && !defined(OS_MACOSX)
  return NewFileDialogGtk(mode);
#elif defined(WIN32)
  return NewFileDialogWin32(mode, module);
#elif defined(OS_MACOSX)
  return NewFileDialogCarbon(mode);
#else
  return NULL;
#endif  // PLATFORM_xyz
}

FileDialog::FileDialog() {
}

FileDialog::~FileDialog() {
}

#endif  // OFFICIAL_BUILD

