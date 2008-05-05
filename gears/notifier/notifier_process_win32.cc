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

#ifdef WIN32

#include <atlbase.h>
#include <atlwin.h>
#include <objbase.h>
#include "gears/notifier/notifier_process.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

static const wchar_t kNotifierDummyWndClassName[] =
    L"DAAC6F7D-4BAB-403b-AC79-D09E165BC509";

// Dummy window
class NotifierDummyWindow :
    public CWindowImpl<NotifierDummyWindow> {
 public:
  DECLARE_WND_CLASS(kNotifierDummyWndClassName);

  NotifierDummyWindow() : CWindowImpl() {}

  ~NotifierDummyWindow() {
    if (m_hWnd) {
      DestroyWindow();
    }
  }

  bool NotifierDummyWindow::Initialize() {
    return Create(NULL, 0, L"GearsNotifier", WS_POPUP) != NULL;
  }

  BEGIN_MSG_MAP(NotifierDummyWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
  END_MSG_MAP()

  LRESULT OnCreate(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    return 0;
  }

  LRESULT OnDestroy(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    ::PostQuitMessage(0);
    return 0;
  }
};

static scoped_ptr<NotifierDummyWindow> g_dummy_wnd;

bool NotifierProcess::StartProcess() {
  // TODO (jianli): to be implemented.
  return false;
}

bool NotifierProcess::RegisterProcess() {
  assert(g_dummy_wnd.get() == NULL);
  g_dummy_wnd.reset(new NotifierDummyWindow);
  return g_dummy_wnd->Initialize();
}

unsigned int NotifierProcess::FindProcess() {
  unsigned int process_id = 0;
  HWND window = ::FindWindow(kNotifierDummyWndClassName, NULL);
  if (window) {
    ::GetWindowThreadProcessId(window, reinterpret_cast<DWORD*>(&process_id));
  }

  return process_id;
}

#endif  // defined(WIN32)
