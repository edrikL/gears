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

#include "gears/base/common/module_wrapper.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"
#include "gears/localserver/managed_resource_store_module.h"
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

  *can = LocalServer::CanServeLocally(full_url.c_str(),
                                      EnvPageBrowsingContext())
            ? VARIANT_TRUE : VARIANT_FALSE;
  LOG16((L"LocalServer::CanServeLocally( %s ) %s\n",
         url, *can ? L"TRUE" : L"FALSE"));
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// createManagedStore
//------------------------------------------------------------------------------
STDMETHODIMP GearsLocalServer::createManagedStore(
    /* [in] */ const BSTR name,
    /* [optional][in] */ const VARIANT *required_cookie,
    /* [retval][out] */ IDispatch **store_out) {
  if (!store_out) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  CComBSTR required_cookie_bstr(L"");
  std::string16 error_message;
  if (!CheckLocalServerParameters(name, required_cookie, required_cookie_bstr, 
                                  &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  // Check that this page uses a supported URL scheme.
  if (!HttpRequest::IsSchemeSupported(
                        EnvPageSecurityOrigin().scheme().c_str())) {
    RETURN_EXCEPTION(STRING16(L"URL scheme not supported."));
  }

  LOG16((L"LocalServer::createManagedStore( %s, %s )\n",
         name, required_cookie_bstr.m_str));

  scoped_refptr<GearsManagedResourceStore> store;
  CreateModule<GearsManagedResourceStore>(GetJsRunner(), &store);

  if (!store->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize base class."));
  }

  if (!store->store_.CreateOrOpen(EnvPageSecurityOrigin(),
                                  name, required_cookie_bstr.m_str)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize ManagedResourceStore."));
  }

  *store_out = store->GetWrapperToken().pdispVal;
  (*store_out)->AddRef();

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// openManagedStore
//------------------------------------------------------------------------------
STDMETHODIMP GearsLocalServer::openManagedStore(
    /* [in] */ const BSTR name,
    /* [optional][in] */ const VARIANT *required_cookie,
    /* [retval][out] */ IDispatch **store_out) {
  if (!store_out) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  CComBSTR required_cookie_bstr(L"");
  std::string16 error_message;
  if (!CheckLocalServerParameters(name, required_cookie, required_cookie_bstr, 
                                  &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  LOG16((L"LocalServer::openManagedStore( %s, %s )\n",
         name, required_cookie_bstr.m_str));

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (!ManagedResourceStore::ExistsInDB(EnvPageSecurityOrigin(),
                                        name,
                                        required_cookie_bstr.m_str,
                                        &existing_store_id)) {
    *store_out = NULL;
    RETURN_NORMAL();
  }

  scoped_refptr<GearsManagedResourceStore> store;
  CreateModule<GearsManagedResourceStore>(GetJsRunner(), &store);

  if (!store->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize base class."));
  }

  if (!store->store_.Open(existing_store_id)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize ManagedResourceStore."));
  }

  *store_out = store->GetWrapperToken().pdispVal;
  (*store_out)->AddRef();

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// removeManagedStore
//------------------------------------------------------------------------------
STDMETHODIMP GearsLocalServer::removeManagedStore(
    /* [in] */ const BSTR name,
    /* [optional][in] */ const VARIANT *required_cookie) {
  CComBSTR required_cookie_bstr(L"");
  std::string16 error_message;
  if (!CheckLocalServerParameters(name, required_cookie, required_cookie_bstr, 
                                  &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  LOG16((L"LocalServer::removeManagedStore( %s, %s )\n",
         name, required_cookie_bstr.m_str));

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
  std::string16 error_message;
  if (!CheckLocalServerParameters(name, required_cookie, required_cookie_bstr, 
                                  &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  // Check that this page uses a supported URL scheme.
  if (!HttpRequest::IsSchemeSupported(
                        EnvPageSecurityOrigin().scheme().c_str())) {
    RETURN_EXCEPTION(STRING16(L"URL scheme not supported."));
  }

  LOG16((L"LocalServer::createStore( %s, %s )\n",
         name, required_cookie_bstr.m_str));

  CComObject<GearsResourceStore> *store;
  HRESULT hr = CComObject<GearsResourceStore>::CreateInstance(&store);
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
  std::string16 error_message;
  if (!CheckLocalServerParameters(name, required_cookie, required_cookie_bstr, 
                                  &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  LOG16((L"LocalServer::openStore( %s, %s )\n",
         name, required_cookie_bstr.m_str));

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (!ResourceStore::ExistsInDB(EnvPageSecurityOrigin(),
                                 name,
                                 required_cookie_bstr.m_str,
                                 &existing_store_id)) {
    *store_out = NULL;
    RETURN_NORMAL();
  }

  CComObject<GearsResourceStore> *store;
  HRESULT hr = CComObject<GearsResourceStore>::CreateInstance(&store);
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
  std::string16 error_message;
  if (!CheckLocalServerParameters(name, required_cookie, required_cookie_bstr, 
                                  &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  LOG16((L"LocalServer::removeStore( %s, %s )\n",
         name, required_cookie_bstr.m_str));

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
bool GearsLocalServer::CheckLocalServerParameters(
                                            const BSTR name,
                                            const VARIANT *required_cookie,
                                            CComBSTR &required_cookie_bstr,
                                            std::string16 *error_message) {
  if (!name || !name[0]) {
    *error_message = STRING16(L"The name parameter is required.");
    return false;
  }

  if (ActiveXUtils::OptionalVariantIsPresent(required_cookie)) {
    if (required_cookie->vt != VT_BSTR) {
      *error_message = STRING16(L"The required_cookie parameter must be a "
                                L"string.");
      return false;
    }
    // TODO(michaeln): validate this parameter value, parse the name & value,
    // name must not be empty, value must not contain ';' unless its the
    // kNegatedRequiredCookieValue. Ditto for firefox.
    required_cookie_bstr = required_cookie->bstrVal;
  }

  if (!IsUserInputValidAsPathComponent(std::string16(name), error_message)) {
    return false;
  }

  return true;
}

