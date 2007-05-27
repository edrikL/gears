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

#ifndef BAR_COMMON_EVENTDISPATCHER_H_
#define BAR_COMMON_EVENTDISPATCHER_H_


// Class to help dispatching html and other script events to host methods
template <class Host>
class EventDispatcher: public IDispatch {
 public:
  // The event handler prototype.
  typedef HRESULT (__stdcall Host::*EventHandler)(IHTMLEventObj* event);

  EventDispatcher(Host* host, EventHandler handler):
    host_(host), handler_(handler) {
  }

  ~EventDispatcher() {
    host_ = NULL;
    handler_ = NULL;
  }

  // Helper method to attach to element events
  HRESULT Attach(IHTMLElement* element, const BSTR& event) {
    SP< IHTMLElement2 > element2 = element;
    if (element2 == NULL) return E_FAIL;
    VARIANT_BOOL result;
    HRESULT hr = S_OK;
    if (FAILED(hr = element2->attachEvent(event, this, &result))) return hr;
    if (!result) return E_FAIL; // Unable to attache the event handler.
    return S_OK;
  }

  // Helper method to detach from element events
  HRESULT Detach(IHTMLElement* element, const BSTR& event) {
    SP< IHTMLElement2 > element2 = element;
    if (element2 == NULL) return E_FAIL;
    HRESULT hr = S_OK;
    if (FAILED(hr = element2->detachEvent(event, this))) return hr;
    return S_OK;
  }

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDispatch)) {
      *ppv = static_cast<IDispatch*>(this);
      host_->AddRef();
      return S_OK;
    } else {
      *ppv = 0;
      return E_NOINTERFACE;
    }  
  }
  
  ULONG __stdcall AddRef() {
    return host_->AddRef();
  }
  
  ULONG __stdcall Release() {
    return host_->Release();
  }

  // IDispatch
  STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {
    *pctinfo = 0;
    return S_OK;
  }
  
  STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {
    *ppTInfo = 0;
    return E_NOTIMPL;
  }
  
  STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, 
                             LCID lcid, DISPID* rgDispId) {
    *rgDispId = NULL;
    return E_NOTIMPL;
  }
  
  STDMETHODIMP Invoke(/* [in] */ DISPID dispIdMember,
                      /* [in] */ REFIID riid,
                      /* [in] */ LCID lcid,
                      /* [in] */ WORD wFlags,
                      /* [out][in] */ DISPPARAMS* pDispParams,
                      /* [out] */ VARIANT* pVarResult,
                      /* [out] */ EXCEPINFO* pExcepInfo,
                      /* [out] */ UINT* puArgErr) {
    if (!host_ || !handler_) return E_FAIL;
    ASSERT(dispIdMember == 0);
    if (pDispParams->cArgs != 1) return E_INVALIDARG; // Expecting only one argument.

    SP< IHTMLEventObj > event(pDispParams->rgvarg[0].punkVal);
    if (event == NULL) return E_INVALIDARG;
    return (host_->*handler_)(event);
  }

 protected:
  Host* host_;
  EventHandler handler_;
};


#endif  // BAR_COMMON_EVENTDISPATCHER_H_
