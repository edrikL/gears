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

#include <assert.h>

#include "gears/localserver/ie/localserver_ie.h"

#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"
#include "gears/localserver/ie/managed_resource_store_ie.h"
#include "gears/localserver/ie/resource_store_ie.h"


//------------------------------------------------------------------------------
// canServeLocally
//------------------------------------------------------------------------------
STDMETHODIMP GearsLocalServer::canServeLocally(
    /* [in] */ const BSTR url,
    /* [retval][out] */ VARIANT_BOOL *can) {
  if (!url || !url[0]) {
    assert(url && url[0]);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  std::string16 full_url;
  if (!ResolveAndNormalize(EnvPageLocationUrl().c_str(), url, &full_url)) {
    RETURN_EXCEPTION(STRING16(L"Failed to resolve url."));
  }
  if (!EnvPageSecurityOrigin().IsSameOriginAsUrl(full_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Url is not from the same origin"));
  }

  *can = LocalServer::CanServeLocally(full_url.c_str())
            ? VARIANT_TRUE : VARIANT_FALSE;
  ATLTRACE(_T("LocalServer::CanServeLocally( %s ) %s\n"),
           url, *can ? L"TRUE" : L"FALSE");
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// createManagedStore
//------------------------------------------------------------------------------
STDMETHODIMP GearsLocalServer::createManagedStore(
    /* [in] */ const BSTR name,
    /* [optional][in] */ const VARIANT *required_cookie,
    /* [retval][out] */ GearsManagedResourceStoreInterface **store_out) {
  if (!store_out) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  CComBSTR required_cookie_bstr(L"");
  HRESULT hr = CheckLocalServerParameters(name, required_cookie,
                                          required_cookie_bstr);
  if (FAILED(hr)) {
    return hr;
  }

  // Check that this page uses a supported URL scheme.
  if (!HttpRequest::IsSchemeSupported(
                        EnvPageSecurityOrigin().scheme().c_str())) {
    RETURN_EXCEPTION(STRING16(L"URL scheme not supported."));
  }

  ATLTRACE(_T("LocalServer::createManagedStore( %s, %s )\n"),
           name, required_cookie_bstr.m_str);

  CComObject<GearsManagedResourceStore> *store;
  hr = CComObject<GearsManagedResourceStore>::CreateInstance(&store);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to CreateInstance."));
  }

  CComPtr< CComObject<GearsManagedResourceStore> > reference_adder(store);

  if (!store->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize base class."));
  }

  if (!store->store_.CreateOrOpen(EnvPageSecurityOrigin(),
                                  name, required_cookie_bstr.m_str)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize ManagedResourceStore."));
  }

  hr = store->QueryInterface(store_out);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to QueryInterface for"
                              L" GearsManagedResourceStoreInterface."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// openManagedStore
//------------------------------------------------------------------------------
STDMETHODIMP GearsLocalServer::openManagedStore(
    /* [in] */ const BSTR name,
    /* [optional][in] */ const VARIANT *required_cookie,
    /* [retval][out] */ GearsManagedResourceStoreInterface **store_out) {
  if (!store_out) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  CComBSTR required_cookie_bstr(L"");
  HRESULT hr = CheckLocalServerParameters(name, required_cookie,
                                          required_cookie_bstr);
  if (FAILED(hr)) {
    return hr;
  }

  ATLTRACE(_T("LocalServer::openManagedStore( %s, %s )\n"),
           name, required_cookie_bstr.m_str);

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (!ManagedResourceStore::ExistsInDB(EnvPageSecurityOrigin(),
                                        name,
                                        required_cookie_bstr.m_str,
                                        &existing_store_id)) {
    *store_out = NULL;
    RETURN_NORMAL();
  }

  CComObject<GearsManagedResourceStore> *store;
  hr = CComObject<GearsManagedResourceStore>::CreateInstance(&store);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to CreateInstance."));
  }

  CComPtr< CComObject<GearsManagedResourceStore> > reference_adder(store);

  if (!store->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize base class."));
  }

  if (!store->store_.Open(existing_store_id)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize ManagedResourceStore."));
  }

  hr = store->QueryInterface(store_out);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to QueryInterface for"
                              L" GearsManagedResourceStoreInterface."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// removeManagedStore
//------------------------------------------------------------------------------
STDMETHODIMP GearsLocalServer::removeManagedStore(
    /* [in] */ const BSTR name,
    /* [optional][in] */ const VARIANT *required_cookie) {
  CComBSTR required_cookie_bstr(L"");
  HRESULT hr = CheckLocalServerParameters(name, required_cookie,
                                          required_cookie_bstr);
  if (FAILED(hr)) {
    return hr;
  }

  ATLTRACE(_T("LocalServer::removeManagedStore( %s, %s )\n"),
           name, required_cookie_bstr.m_str);

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (!ManagedResourceStore::ExistsInDB(EnvPageSecurityOrigin(),
                                        name,
                                        required_cookie_bstr.m_str,
                                        &existing_store_id)) {
    RETURN_NORMAL();
  }

  ManagedResourceStore store;
  if (!store.Open(existing_store_id)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize ManagedResourceStore."));
  }

  if (!store.Remove()) {
    RETURN_EXCEPTION(STRING16(L"Failed to remove ManagedResourceStore."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// createStore
//------------------------------------------------------------------------------
STDMETHODIMP GearsLocalServer::createStore(
    /* [in] */ const BSTR name,
    /* [optional][in] */ const VARIANT *required_cookie,
    /* [retval][out] */ GearsResourceStoreInterface **store_out) {
  if (!store_out) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  CComBSTR required_cookie_bstr(L"");
  HRESULT hr = CheckLocalServerParameters(name, required_cookie,
                                          required_cookie_bstr);
  if (FAILED(hr)) {
    return hr;
  }

  // Check that this page uses a supported URL scheme.
  if (!HttpRequest::IsSchemeSupported(
                        EnvPageSecurityOrigin().scheme().c_str())) {
    RETURN_EXCEPTION(STRING16(L"URL scheme not supported."));
  }

  ATLTRACE(_T("LocalServer::createStore( %s, %s )\n"),
           name, required_cookie_bstr.m_str);

  CComObject<GearsResourceStore> *store;
  hr = CComObject<GearsResourceStore>::CreateInstance(&store);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to CreateInstance."));
  }

  CComPtr< CComObject<GearsResourceStore> > reference_adder(store);

  if (!store->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize base class."));
  }

  if (!store->store_.CreateOrOpen(EnvPageSecurityOrigin(),
                                  name, required_cookie_bstr.m_str)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize ResourceStore."));
  }


  hr = store->QueryInterface(store_out);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to QueryInterface for"
                              L" GearsResourceStoreInterface."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// openStore
//------------------------------------------------------------------------------
STDMETHODIMP GearsLocalServer::openStore(
    /* [in] */ const BSTR name,
    /* [optional][in] */ const VARIANT *required_cookie,
    /* [retval][out] */ GearsResourceStoreInterface **store_out) {
  if (!store_out) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  CComBSTR required_cookie_bstr(L"");
  HRESULT hr = CheckLocalServerParameters(name, required_cookie,
                                          required_cookie_bstr);
  if (FAILED(hr)) {
    return hr;
  }

  ATLTRACE(_T("LocalServer::openStore( %s, %s )\n"),
           name, required_cookie_bstr.m_str);

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (!ResourceStore::ExistsInDB(EnvPageSecurityOrigin(),
                                 name,
                                 required_cookie_bstr.m_str,
                                 &existing_store_id)) {
    *store_out = NULL;
    RETURN_NORMAL();
  }

  CComObject<GearsResourceStore> *store;
  hr = CComObject<GearsResourceStore>::CreateInstance(&store);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to CreateInstance."));
  }

  CComPtr< CComObject<GearsResourceStore> > reference_adder(store);

  if (!store->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize base class."));
  }

  if (!store->store_.Open(existing_store_id)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize ResourceStore."));
  }


  hr = store->QueryInterface(store_out);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to QueryInterface for"
                              L" GearsResourceStoreInterface."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// removeStore
//------------------------------------------------------------------------------
STDMETHODIMP GearsLocalServer::removeStore(
    /* [in] */ const BSTR name,
    /* [optional][in] */ const VARIANT *required_cookie) {
  CComBSTR required_cookie_bstr(L"");
  HRESULT hr = CheckLocalServerParameters(name, required_cookie,
                                          required_cookie_bstr);
  if (FAILED(hr)) {
    return hr;
  }

  ATLTRACE(_T("LocalServer::removeStore( %s, %s )\n"),
           name, required_cookie_bstr.m_str);

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (!ResourceStore::ExistsInDB(EnvPageSecurityOrigin(),
                                 name,
                                 required_cookie_bstr.m_str,
                                 &existing_store_id)) {
    RETURN_NORMAL();
  }

  ResourceStore store;
  if (!store.Open(existing_store_id)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize ResourceStore."));
  }

  if (!store.Remove()) {
    RETURN_EXCEPTION(STRING16(L"Failed to remove ResourceStore."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// CheckLocalServerParameters
//------------------------------------------------------------------------------
HRESULT GearsLocalServer::CheckLocalServerParameters(
                                            const BSTR name,
                                            const VARIANT *required_cookie,
                                            CComBSTR &required_cookie_bstr) {
  if (!name || !name[0]) {
    RETURN_EXCEPTION(STRING16(L"The name parameter is required."));
  }

  if (ActiveXUtils::OptionalVariantIsPresent(required_cookie)) {
    if (required_cookie->vt != VT_BSTR) {
      RETURN_EXCEPTION(STRING16(L"The required_cookie parameter must be a "
                                L"string."));
    }
    // TODO(michaeln): validate this parameter value, parse the name & value,
    // name must not be empty, value must not contain ';' unless its the
    // kNegatedRequiredCookieValue. Ditto for firefox.
    required_cookie_bstr = required_cookie->bstrVal;
  }

  if (!IsStringValidPathComponent(name)) {
    std::string16 error(STRING16(L"The name parameter contains invalid "
                                 L"characters:"));
    error += static_cast<const char16 *>(name);
    error += STRING16(L".");
    RETURN_EXCEPTION(error.c_str());
  }

  RETURN_NORMAL();
}

