// Copyright 2009, Google Inc.
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

#undef NOMINMAX
#include <mshtmdid.h>
#include <windows.h>
#include <gdiplus.h>

#include "gears/canvas/canvas_rendering_element_ie.h"

#include "gears/base/common/base_class.h"
#include "gears/base/common/leak_counter.h"
#include "gears/base/common/scoped_refptr.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/canvas/canvas.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColorPriv.h"


CanvasRenderingElementIE::CanvasRenderingElementIE()
    : canvas_(NULL) {
  LEAK_COUNTER_INCREMENT(CanvasRenderingElementIE);
}


CanvasRenderingElementIE::~CanvasRenderingElementIE() {
  LEAK_COUNTER_DECREMENT(CanvasRenderingElementIE);
}


HRESULT CanvasRenderingElementIE::Detach(void) {
  return S_OK;
}


HRESULT CanvasRenderingElementIE::Init(
    IElementBehaviorSite *pElementBehaviorSite) {
  paint_site_ = pElementBehaviorSite;
  return S_OK;
}


HRESULT CanvasRenderingElementIE::Notify(LONG lEvent, VARIANT *pVar) {
  return S_OK;
}

  
HRESULT CanvasRenderingElementIE::FindBehavior(
    BSTR name, BSTR url, IElementBehaviorSite *site, IElementBehavior **out) {
  if (!name || (_wcsicmp(name, L"CanvasRendering") != 0)) {
    return E_FAIL;
  }
  return QueryInterface(IID_IElementBehavior, reinterpret_cast<void**>(out));
}


HRESULT CanvasRenderingElementIE::Draw(
      RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc,
      LPVOID pvDrawObject) {
  if (!canvas_) {
    return S_OK;
  }
  const SkBitmap &skia_bitmap = canvas_->GetSkBitmap();
  assert(skia_bitmap.config() == SkBitmap::kARGB_8888_Config);
  const int w = skia_bitmap.width();
  const int h = skia_bitmap.height();
  if (w <= 0 || h <= 0) {
    return S_OK;
  }
  SkAutoLockPixels skia_bitmap_lock(skia_bitmap);

#if 0
  // If debugging InvalidateRect, enable this code to make the rendering
  // element's background color change on every call to Draw.
  static int count = 0;
  HBRUSH hBrush = CreateSolidBrush(RGB(
      count == 0 ? 255 : 200,
      count == 1 ? 255 : 200,
      count == 2 ? 255 : 200));
  HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);
  HGDIOBJ hOldPen = SelectObject(hdc, (HPEN)GetStockObject(NULL_PEN));
  Rectangle(hdc, rcBounds.left, rcBounds.top,
      1 + rcBounds.right, 1 + rcBounds.bottom);
  SelectObject(hdc, hOldBrush);
  SelectObject(hdc, hOldPen);
  DeleteObject(hBrush);
  count = (count + 1) % 3;
#endif

  // The GdiplusStartup Function docs at
  // http://msdn.microsoft.com/en-us/library/ms534077(VS.85).aspx say,
  // "If you want to create a DLL that uses GDI+, ... Call GdiplusStartup and
  // GdiplusShutdown in each of your functions that make GDI+ calls."
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  if (Gdiplus::Ok != Gdiplus::GdiplusStartup(
          &gdiplusToken, &gdiplusStartupInput, NULL)) {
    return E_FAIL;
  }

  // Scoped by { ... } so that we delete our Gdiplus::Bitmap object *before* we
  // call GdiplusShutdown.
  {
    Gdiplus::Graphics graphics(hdc);
    // As configured by SkUserConfig.h, Skia and GDI+ have the same byte order
    // in interpreting "ARGB", so we can re-use the SkBitmap's buffer for our
    // GDI+ Bitmap. The "P" in PixelFormat32bppPARGB stands for pre-multiplied
    // (which what SkBitmap does).
    Gdiplus::Bitmap gdiplus_bitmap(
        w, h, skia_bitmap.rowBytes(), PixelFormat32bppPARGB,
        reinterpret_cast<uint8*>(skia_bitmap.getAddr32(0, 0)));
    graphics.DrawImage(&gdiplus_bitmap, rcBounds.left, rcBounds.top);
  }

  Gdiplus::GdiplusShutdown(gdiplusToken);
  return S_OK;
}


HRESULT CanvasRenderingElementIE::GetPainterInfo(HTML_PAINTER_INFO *pInfo) {
  // In the future, we should also implement HTMLPAINTER_SUPPORTS_XFORM, to
  // support full page zoom, for example.
  pInfo->lFlags = HTMLPAINTER_TRANSPARENT;
  pInfo->lZOrder = HTMLPAINT_ZORDER_REPLACE_CONTENT;
  return S_OK;
}


HRESULT CanvasRenderingElementIE::HitTestPoint(
    POINT pt, BOOL *pbHit, LONG *plPartID) {
  return S_OK;
}


HRESULT CanvasRenderingElementIE::OnResize(SIZE pt) {
  return S_OK;
}

  
void CanvasRenderingElementIE::SetGearsCanvas(GearsCanvas *canvas) {
  assert(canvas_ == NULL);
  canvas_ = canvas;
}


void CanvasRenderingElementIE::OnGearsCanvasDestroyed() {
  assert(canvas_ != NULL);
  canvas_ = NULL;
}


IHTMLElement *CanvasRenderingElementIE::GetHtmlElement() {
  if (!canvas_) {
    return NULL;
  }
  if (html_element_) {
    return html_element_;
  }
  scoped_refptr<ModuleEnvironment> module_environment;
  canvas_->GetModuleEnvironment(&module_environment);
  CComPtr<IHTMLDocument2> document;
  if (FAILED(ActiveXUtils::GetHtmlDocument2(
          module_environment->iunknown_site_, &document))) {
    return NULL;
  }
  CComPtr<IHTMLElement> result;
  document->createElement(CComBSTR(L"gearsCanvasRenderer"), &result);
  CComQIPtr<IHTMLElement2> html_element2(result);
  if (!html_element2) {
    return NULL;
  }
  CComBSTR behavior_url(L"#" PRODUCT_SHORT_NAME L"#CanvasRendering");
  CComQIPtr<IElementBehaviorFactory> factory(this);
  if (!factory) {
    return NULL;
  }
  CComVariant factory_as_variant(static_cast<IUnknown *>(factory));
  LONG cookie = 0;
  if (FAILED(html_element2->addBehavior(
          behavior_url, &factory_as_variant, &cookie))) {
    return NULL;
  }
  html_element_.Attach(result.Detach());
  return html_element_;
}


void CanvasRenderingElementIE::InvalidateRenderingElement() {
  if (paint_site_) {
    paint_site_->InvalidateRect(NULL);
  }
}
