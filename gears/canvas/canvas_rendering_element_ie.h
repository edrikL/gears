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

#ifndef GEARS_CANVAS_CANVAS_RENDERING_ELEMENT_IE_H__
#define GEARS_CANVAS_CANVAS_RENDERING_ELEMENT_IE_H__

#if !defined(OFFICIAL_BUILD)

#include "gears/base/common/common.h"

class GearsCanvas;
struct IHTMLElement;

class ATL_NO_VTABLE CanvasRenderingElementIE
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<CanvasRenderingElementIE>,
      public IElementBehavior,
      public IElementBehaviorFactory,
      public IHTMLPainter {
 public:
  DECLARE_NOT_AGGREGATABLE(CanvasRenderingElementIE)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  BEGIN_COM_MAP(CanvasRenderingElementIE)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IElementBehaviorFactory)
    COM_INTERFACE_ENTRY(IHTMLPainter)
  END_COM_MAP()

  // IElementBehavior methods
  STDMETHOD(Detach)(void);
  STDMETHOD(Init)(IElementBehaviorSite *pElementBehaviorSite);
  STDMETHOD(Notify)(LONG lEvent, VARIANT *pVar);

  // IElementBehaviorFactory methods
  STDMETHOD(FindBehavior)(
      BSTR name, BSTR url, IElementBehaviorSite *site, IElementBehavior **out);

  // IHTMLPainter methods
  STDMETHOD(Draw)(
      RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc,
      LPVOID pvDrawObject);
  STDMETHOD(GetPainterInfo)(HTML_PAINTER_INFO *pInfo);
  STDMETHOD(HitTestPoint)(POINT pt, BOOL *pbHit, LONG *plPartID);
  STDMETHOD(OnResize)(SIZE pt);

  CanvasRenderingElementIE();
  virtual ~CanvasRenderingElementIE();

  IHTMLElement *GetHtmlElement();
  void InvalidateRenderingElement();
  void OnGearsCanvasDestroyed();
  void SetGearsCanvas(GearsCanvas *canvas);

 private:
  GearsCanvas *canvas_;
  CComPtr<IHTMLElement> html_element_;
  CComQIPtr<IHTMLPaintSite> paint_site_;

  DISALLOW_EVIL_CONSTRUCTORS(CanvasRenderingElementIE);
};

#endif  // !defined(OFFICIAL_BUILD)
#endif  // GEARS_CANVAS_CANVAS_RENDERING_ELEMENT_IE_H__
