// Copyright 2006, Google Inc.
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
//
// A collection of static utility methods.

#ifndef GEARS_BASE_IE_ACTIVEX_UTILS_H__
#define GEARS_BASE_IE_ACTIVEX_UTILS_H__

#include "gears/base/common/string16.h"
#include "gears/base/ie/atl_headers.h"

struct IHTMLElement;
class SecurityOrigin;

#ifdef WINCE
struct IWebBrowser2;
struct IPIEHTMLDocument;
struct IPIEHTMLDocument2;
struct IPIEHTMLWindow2;
typedef IPIEHTMLDocument IHTMLDocument;
typedef IPIEHTMLDocument2 IHTMLDocument2;
#endif

class ActiveXUtils {
 public:
  // Returns the location url of the containing webBrowser as determined by
  // IWebBrowser2.LocationURL.
  // TODO(cprince): across the codebase, change PageOrigin to PageSecurityOrigin
  // and PageLocation to PageLocationUrl, for consistency.
  static bool GetPageLocation(IUnknown *site, std::string16 *page_location_url);
  static bool GetPageOrigin(IUnknown *site, SecurityOrigin *security_origin);

  // Returns the IWebBrowser2 interface corresponding to the given site.
  static HRESULT GetWebBrowser2(IUnknown *site,
                                IWebBrowser2 **browser2);

  // Returns the IHTMLDocument2 interface corresponding to the given site.
  static HRESULT GetHtmlDocument2(IUnknown *site,
                                  IHTMLDocument2 **document2);

// Returns the IHTMLWindow2 interface corresponding to the given site.  
#ifdef WINCE
  static HRESULT GetHtmlWindow2(IUnknown *site,
                                IPIEHTMLWindow2 **window2);
#else
  // Returns the IHTMLWindow2 interface corresponding to the given site.
  static HRESULT GetHtmlWindow2(IUnknown *site,
                                IHTMLWindow2 **window2);
#endif

#ifdef WINCE
  // TODO(andreip): no IHTMLWindow3 in Windows Mobile.
#else
  // Returns the IHTMLWindow3 interface corresponding to the given site.
  // Can be used with our HtmlEventMonitor.
  static HRESULT GetHtmlWindow3(IUnknown *site,
                                IHTMLWindow3 **window3);
#endif

  // Returns the dispatch id of the named member.
  static HRESULT GetDispatchMemberId(IDispatch *dispatch, const WCHAR *name,
                                     DISPID *dispid) {
    *dispid = DISPID_UNKNOWN;
    return dispatch->GetIDsOfNames(IID_NULL, const_cast<WCHAR**>(&name), 1, 0,
                                   dispid);
  }

  // Determine whether a variant is, ahem, null or undefined.
  static bool VariantIsNullOrUndefined(const VARIANT *var) {
    return var->vt == VT_NULL || var->vt == VT_EMPTY;
  }

  // Determine whether an optional variant parameter was specified. Handles
  // unpassed params, as well as <undefined> and <null> valued params.
  // TODO(cprince): When JsParamFetcher is implemented for IE, move this
  // function there.
  static bool OptionalVariantIsPresent(const VARIANT *arg) {
    // If an optional variant was not passed, we get this
    // See: http://msdn2.microsoft.com/en-us/library/ms931135.aspx
    if (arg->vt == VT_ERROR && arg->scode == DISP_E_PARAMNOTFOUND) {
      return false;
    }

    if (VariantIsNullOrUndefined(arg)) {
      return false;
    }

    return true;
  }

  // Returns the property value by name.
  // The caller is responsible for freeing the returned VARIANT.
  static HRESULT GetDispatchProperty(IDispatch *dispatch, const WCHAR *name,
                                     VARIANT *value) {
    DISPID dispid;
    HRESULT hr = GetDispatchMemberId(dispatch, name, &dispid);
    if (FAILED(hr)) return hr;
    return GetDispatchProperty(dispatch, dispid, value);
  }

  // Returns the property value by dispid.
  // The caller is responsible for freeing the returned VARIANT.
  static HRESULT GetDispatchProperty(IDispatch *dispatch, DISPID dispid,
                                     VARIANT *value);

  // Sets the property value by name.
  static HRESULT SetDispatchProperty(IDispatch *dispatch, const WCHAR *name,
                                     const VARIANT *value) {
    DISPID dispid;
    HRESULT hr = GetDispatchMemberId(dispatch, name, &dispid);
    if (FAILED(hr)) return hr;
    return SetDispatchProperty(dispatch, dispid, value);
  }

  // Sets the property value by dispid.
  static HRESULT SetDispatchProperty(IDispatch *dispatch, DISPID dispid,
                                     const VARIANT *value);

#ifdef WINCE
  // TODO(andreip): implement on Windows Mobile.
#else
  // Returns the html attribute value of the given HTMLElement.
  // The caller is responsible for freeing the returned VARIANT.
  static HRESULT GetHTMLElementAttributeValue(IHTMLElement *element,
                                              const WCHAR *name,
                                              VARIANT *value);
#endif

  // Convert NULL BSTR to the empty string.
  static const CComBSTR kEmptyBSTR;
  static const BSTR SafeBSTR(const BSTR value) {
    return value ? value : kEmptyBSTR.m_str;
  }
  // Returns true if there the browser is in 'online' mode and the local
  // system is connected to a network.
  static bool IsOnline();

#ifdef WINCE
  static void SetWebBrowser2FromSite(IUnknown* site);
 private:
  static CComPtr<IWebBrowser2> web_browser2_;
#endif
};


#endif // GEARS_BASE_IE_ACTIVEX_UTILS_H__
