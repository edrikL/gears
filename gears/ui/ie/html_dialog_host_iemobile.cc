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

#include "gears/ui/ie/html_dialog_host_iemobile.h"
#include <initguid.h>
#include <imaging.h>

#include "gears/base/common/string_utils.h"

// Some macros defined here to keep changes from original source to a low roar.
// See //depot/googleclient/bar/common/(debugbase|macros).h for original
// definitions.
#ifdef WINCE
#else
#define ASSERT(expr) assert(expr);
#endif

// Checks for HRESULT and if it fails returns. The macro will ASSERT in debug.
#define CHK(cmd) \
do { \
  HRESULT __hr = (cmd); \
  if (FAILED(__hr)) { \
    ASSERT(false); \
    return __hr; \
  } \
} \
while(0);

// Note: we use a VERIFY macro, whichs verify that an operation succeed and 
// ASSERTs on failure. Only asserts in DEBUG. 
// This VERIFY macro is already defined by dbgapi.h on Windows mobile.

#define HTML MAKEINTRESOURCE(23)

// HtmlDialogHost
// The static variable we use to access the dialog from the ActiveX object
HtmlDialogHost* HtmlDialogHost::html_permissions_dialog_;

bool HtmlDialogHost::ShowDialog(const char16 *resource_file_name,
                                   const CSize& size,
                                   const BSTR dialog_arguments,
                                   BSTR *dialog_result) {
  dialog_arguments_ = dialog_arguments;
  desired_size_ = size;
  url_ = resource_file_name;

  DoModal();

  if (dialog_result_ == NULL) {
    (*dialog_result) = NULL;
  } else {
    dialog_result_.CopyTo(dialog_result);
  }

  if (IsWindow()) DestroyWindow();
  return true;
}

LRESULT HtmlDialogHost::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                        BOOL& bHandled) {
  InitBrowserView();
  HtmlDialogHost::html_permissions_dialog_ = this;
  SendMessage(browser_view_, DTM_CLEAR, 0, 0);

  void* html_file;
  int html_size;

  if SUCCEEDED(LoadFromResource(url_, &html_file, &html_size)) {
    char* content = new char[html_size];
    memcpy(content, html_file, html_size);
    CHK(SendMessage(browser_view_, DTM_ADDTEXT, FALSE, (LPARAM)content));
    delete [] content;
    CHK(SendMessage(browser_view_, DTM_ENDOFSOURCE, 0, 0));
  }
  return true;
}

// The OnNotify() function is called upon WM_NOTIFY
// we just process the NM_INLINE_IMAGE and NM_INLINE_STYLE notifications to
// load images or css includes manually from the dll, as by default the HTML
// control will not load anything automatically unless we use the navigate
// message. Of course this does not work on res:// url, so..
LRESULT HtmlDialogHost::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                    BOOL& bHandled) {
  NM_HTMLVIEW* html_view = reinterpret_cast<NM_HTMLVIEW*> (lParam);
  switch (html_view->hdr.code) {
    case NM_INLINE_IMAGE:
    case NM_INLINE_STYLE:
    {
      const char* str = reinterpret_cast<const char*> (html_view->szTarget);
      CString rsc_name(str);
      if (((html_view->hdr.code == NM_INLINE_STYLE)
          && SUCCEEDED(LoadCSS(rsc_name)))
          || SUCCEEDED(LoadImage(rsc_name, html_view->dwCookie))) {
        SetWindowLong(DWL_MSGRESULT, true);
        return true;
      }
      break;
    }
  }
  return false;
}

LRESULT HtmlDialogHost::OnClose(UINT message, WPARAM w, LPARAM l,
                                   BOOL& handled) {
  HtmlDialogHost::html_permissions_dialog_ = NULL;
  return EndDialog(IDCANCEL);
}

void HtmlDialogHost::InitBrowserView() {
  int screen_width = GetSystemMetrics(SM_CXFULLSCREEN);
  int screen_height = GetSystemMetrics(SM_CYFULLSCREEN);
  int menu_height = GetSystemMetrics(SM_CYMENU);
  int desired_width = desired_size_.cx;
  int desired_height = desired_size_.cy;
  if (screen_width - desired_width > (screen_width/10)) {
    desired_width = static_cast<int> (screen_width * 0.9);
  }
  if (screen_height - desired_height > (screen_height/10)) {
    desired_height = static_cast<int> (screen_height * 0.9);
  }
  int pos_x = (screen_width - desired_width) / 2;
  int pos_y = (screen_height - desired_height) / 2;
  pos_y += menu_height;

  MoveWindow(pos_x, pos_y, desired_width, desired_height);
  RECT rc;
  GetClientRect(&rc);

  HINSTANCE module_instance = _AtlBaseModule.GetModuleInstance();
  InitHTMLControl(module_instance);
  browser_view_ = CreateWindow(WC_HTML, NULL,
                               WS_CHILD | WS_BORDER | WS_VISIBLE,
                               2, 2, (rc.right - rc.left) -4,
                               (rc.bottom - rc.top) -4,
                               m_hWnd, NULL,  module_instance, NULL);

  SendMessage(browser_view_, DTM_ENABLESCRIPTING, 0, TRUE);
  SendMessage(browser_view_, DTM_DOCUMENTDISPATCH, 0, (LPARAM) &document_);
}

// This function returns a pointer to a resource contained in the dll
// NOTE: LockResource() does not do anything on Winmo...
HRESULT HtmlDialogHost::LoadFromResource(CString rsc, void** resource,
                                            int* len) {
  HMODULE hmodule = _AtlBaseModule.GetModuleInstance();
  HRSRC rscInfo = FindResource(hmodule, rsc, HTML);
  if (rscInfo) {
    HGLOBAL rscData = LoadResource(hmodule, rscInfo);
    *resource = LockResource(rscData);
    *len = SizeofResource(hmodule, rscInfo);
    return S_OK;
  }
  return E_FAIL;
}

HRESULT HtmlDialogHost::LoadCSS(CString rsc) {
  void* css_file;
  int css_size;

  if SUCCEEDED(LoadFromResource(rsc, &css_file, &css_size)) {
    char* content = new char[css_size];
    memcpy(content, css_file, css_size);
    HRESULT hr = SendMessage(browser_view_, DTM_ADDSTYLE, 0, (LPARAM)content);
    delete [] content;
    return hr;
  } else {
    return E_FAIL;
  }
}

HRESULT HtmlDialogHost::LoadImage(CString rsc, DWORD cookie) {
  IImagingFactory *image_factory = NULL;
  IImage *image = NULL;
  ImageInfo image_info;
  void* image_resource;
  int image_size;
  HRESULT hr;

  hr = CoCreateInstance(CLSID_ImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                        __uuidof(IImagingFactory),
                        reinterpret_cast<void**>(&image_factory));

  if (hr == S_OK) {
    hr = LoadFromResource(rsc, &image_resource, &image_size);
  }

  if (hr == S_OK) {
    image_factory->CreateImageFromBuffer(image_resource,
                                         image_size,
                                         BufferDisposalFlagNone,
                                         &image);
  }

  if (hr == S_OK) {
    hr = image->GetImageInfo(&image_info);
  }

  if (hr == S_OK) {
    HDC dc = GetDC();
    HDC bitmap_dc = CreateCompatibleDC(dc);

    // We need to convert our pImage to a HBITMAP
    HBITMAP bitmap = CreateCompatibleBitmap(dc, image_info.Width,
                                            image_info.Height);

    if (bitmap) {
      HGDIOBJ old_bitmap_dc = SelectObject(bitmap_dc, bitmap);
      HGDIOBJ brush = GetStockObject(WHITE_BRUSH);
      if (brush) {
        FillRect(bitmap_dc, CRect(0, 0, image_info.Width, image_info.Height),
                 (HBRUSH)brush);
      }
      image->Draw(bitmap_dc, CRect(0, 0, image_info.Width,
                  image_info.Height), NULL);
      SelectObject(bitmap_dc, old_bitmap_dc);
    }

    // We can now fill the structure with our image.
    INLINEIMAGEINFO inline_image_info;
    inline_image_info.dwCookie = cookie;
    inline_image_info.iOrigHeight = image_info.Height;
    inline_image_info.iOrigWidth = image_info.Width;
    inline_image_info.hbm = bitmap;
    // the bitmap will be destructed when
    // the page is closed
    inline_image_info.bOwnBitmap = TRUE;

    SendMessage(browser_view_, DTM_SETIMAGE, FALSE, (LPARAM)&inline_image_info);
    DeleteDC(bitmap_dc);
  }

  if (image)
    image->Release();

  if (image_factory)
    image_factory->Release();

  return hr;
}

HRESULT HtmlDialogHost::GetDialogArguments(BSTR *args_string) {
  // For convenience, we interpret a null string the same as "null".
  if (dialog_arguments_ == NULL) {
    (*args_string) = NULL;
    return S_OK;
  } else {
    return dialog_arguments_.CopyTo(args_string);
  }
}

HRESULT HtmlDialogHost::CloseDialog(const BSTR result_string) {
  dialog_result_ = result_string;
  return EndDialog(IDCANCEL);
}

HRESULT HtmlDialogHost::IsPocketIE(VARIANT_BOOL *retval) {
  *retval = VARIANT_TRUE;
  return S_OK;
}
