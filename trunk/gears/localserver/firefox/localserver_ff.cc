// Copyright 2005, Google Inc.
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

#include "gecko_internal/nsIDOMClassInfo.h"
#include "gecko_internal/nsIVariant.h"

#include "gears/localserver/firefox/localserver_ff.h"

#include "gears/base/common/paths.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/localserver/firefox/managed_resource_store_ff.h"
#include "gears/localserver/firefox/resource_store_ff.h"


// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_THREADSAFE_ADDREF(GearsLocalServer)
NS_IMPL_THREADSAFE_RELEASE(GearsLocalServer)
NS_INTERFACE_MAP_BEGIN(GearsLocalServer)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsLocalServerInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsLocalServerInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsLocalServer)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsLocalServerClassName = "GearsLocalServer";
const nsCID kGearsLocalServerClassId = {0x3d139191, 0xbfa4, 0x4bf7, {0xaf, 0x8c,
                                        0xce, 0x61, 0x3, 0x7a, 0x6, 0x58}};
                                       // {3D139191-BFA4-4bf7-AF8C-CE61037A0658}

// TODO(michaeln): support for wide string logging would be nice and then make
// the logging in this class match that of the corresponding class in IE

//-----------------------------------------------------------------------------
// CanServeLocally
//-----------------------------------------------------------------------------
NS_IMETHODIMP GearsLocalServer::CanServeLocally(//const nsAString &url,
                                                PRBool *retval) {
  JsParamFetcher js_params(this);

  // Get required parameters
  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  std::string16 url;
  if (!js_params.GetAsString(0, &url)) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  // Validate parameters
  if (url.empty()) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  std::string16 full_url;
  if (!ResolveAndNormalize(EnvPageLocationUrl().c_str(), url.c_str(),
                           &full_url)) {
    RETURN_EXCEPTION(STRING16(L"Failed to resolve url."));
  }
  if (!EnvPageSecurityOrigin().IsSameOriginAsUrl(full_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Url is not from the same origin"));
  }

  *retval = LocalServer::CanServeLocally(full_url.c_str())
                ? PR_TRUE : PR_FALSE;
  RETURN_NORMAL();
}

//-----------------------------------------------------------------------------
// CreateManagedStore
//-----------------------------------------------------------------------------
NS_IMETHODIMP GearsLocalServer::CreateManagedStore(
                  //const nsAString &name,
                  //OPTIONAL in AString required_cookie
                  GearsManagedResourceStoreInterface **retval) {
  std::string16 name;
  std::string16 required_cookie;
  std::string16 error_message;
  if (!GetAndCheckParameters(false, &name, &required_cookie, &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  // Check that this page uses a supported URL scheme.
  if (!HttpRequest::IsSchemeSupported(
                        EnvPageSecurityOrigin().scheme().c_str())) {
    RETURN_EXCEPTION(STRING16(L"URL scheme not supported."));
  }

  //LOG(_T("LocalServer::CreateManagedStore( %s, %s )\n"),
  //       name, required_cookie_bstr.m_str);

  nsCOMPtr<GearsManagedResourceStore> store(new GearsManagedResourceStore());
  if (!store->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Error initializing ManagedResourceStore."));
  }
  if (!store->store_.CreateOrOpen(EnvPageSecurityOrigin(),
                                  name.c_str(), required_cookie.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Error initializing ManagedResourceStore."));
  }

  NS_ADDREF(*retval = store);
  RETURN_NORMAL();
}

//-----------------------------------------------------------------------------
// OpenManagedStore
//-----------------------------------------------------------------------------
NS_IMETHODIMP GearsLocalServer::OpenManagedStore(
                  //const nsAString &name,
                  //OPTIONAL in AString required_cookie
                  GearsManagedResourceStoreInterface **retval) {
  std::string16 name;
  std::string16 required_cookie;
  std::string16 error_message;
  if (!GetAndCheckParameters(false, &name, &required_cookie, &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  //LOG(_T("LocalServer::OpenManagedStore( %s, %s )\n"),
  //       name, required_cookie_bstr.m_str);

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (!ManagedResourceStore::ExistsInDB(EnvPageSecurityOrigin(),
                                        name.c_str(),
                                        required_cookie.c_str(),
                                        &existing_store_id)) {
    *retval = NULL;
    RETURN_NORMAL();
  }

  nsCOMPtr<GearsManagedResourceStore> store(new GearsManagedResourceStore());
  if (!store->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Error initializing ManagedResourceStore."));
  }
  if (!store->store_.Open(existing_store_id)) {
    RETURN_EXCEPTION(STRING16(L"Error initializing ManagedResourceStore."));
  }

  NS_ADDREF(*retval = store);
  RETURN_NORMAL();
}

//-----------------------------------------------------------------------------
// RemoveManagedStore
//-----------------------------------------------------------------------------
NS_IMETHODIMP GearsLocalServer::RemoveManagedStore(
                  //const nsAString &name
                  //OPTIONAL in AString required_cookie
                  ) {
  std::string16 name;
  std::string16 required_cookie;
  std::string16 error_message;
  if (!GetAndCheckParameters(false, &name, &required_cookie, &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  //LOG(_T("LocalServer::RemoveManagedStore( %s, %s )\n"),
  //       name, required_cookie_bstr.m_str);

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (!ManagedResourceStore::ExistsInDB(EnvPageSecurityOrigin(),
                                        name.c_str(),
                                        required_cookie.c_str(),
                                        &existing_store_id)) {
    RETURN_NORMAL();
  }

  ManagedResourceStore store;
  if (!store.Open(existing_store_id)) {
    RETURN_EXCEPTION(STRING16(L"Error initializing ManagedResourceStore."));
  }

  if (!store.Remove()) {
    RETURN_EXCEPTION(STRING16(L"Error removing ManagedResourceStore."));
  }

  RETURN_NORMAL();
}

//-----------------------------------------------------------------------------
// CreateStore
//-----------------------------------------------------------------------------
NS_IMETHODIMP GearsLocalServer::CreateStore(
                  //const nsAString &name
                  //OPTIONAL in AString required_cookie
                  GearsResourceStoreInterface **retval) {
  std::string16 name;
  std::string16 required_cookie;
  std::string16 error_message;
  if (!GetAndCheckParameters(false, &name, &required_cookie, &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  // Check that this page uses a supported URL scheme.
  if (!HttpRequest::IsSchemeSupported(
                        EnvPageSecurityOrigin().scheme().c_str())) {
    RETURN_EXCEPTION(STRING16(L"URL scheme not supported."));
  }

  //LOG(_T("LocalServer::CreateStore( %s, %s )\n"),
  //       name, required_cookie_bstr.m_str);

  nsCOMPtr<GearsResourceStore> store(new GearsResourceStore());
  if (!store->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Error initializing ResourceStore."));
  }
  if (!store->store_.CreateOrOpen(EnvPageSecurityOrigin(),
                                  name.c_str(), required_cookie.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Error initializing ResourceStore."));
  }

  NS_ADDREF(*retval = store);
  RETURN_NORMAL();
}

//-----------------------------------------------------------------------------
// OpenStore
//-----------------------------------------------------------------------------
NS_IMETHODIMP GearsLocalServer::OpenStore(
                  //const nsAString &name
                  //OPTIONAL in AString required_cookie
                  GearsResourceStoreInterface **retval) {
  std::string16 name;
  std::string16 required_cookie;
  std::string16 error_message;
  if (!GetAndCheckParameters(false, &name, &required_cookie, &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  //LOG(_T("LocalServer::OpenStore( %s, %s )\n"),
  //       name, required_cookie_bstr.m_str);

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (!ResourceStore::ExistsInDB(EnvPageSecurityOrigin(),
                                 name.c_str(),
                                 required_cookie.c_str(),
                                 &existing_store_id)) {
    *retval = NULL;
    RETURN_NORMAL();
  }

  nsCOMPtr<GearsResourceStore> store(new GearsResourceStore());
  if (!store->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Error initializing ResourceStore."));
  }
  if (!store->store_.Open(existing_store_id)) {
    RETURN_EXCEPTION(STRING16(L"Error initializing ResourceStore."));
  }

  NS_ADDREF(*retval = store);
  RETURN_NORMAL();
}

//-----------------------------------------------------------------------------
// RemoveStore
//-----------------------------------------------------------------------------
NS_IMETHODIMP GearsLocalServer::RemoveStore(
                  //const nsAString &name
                  //OPTIONAL in AString required_cookie
                  ) {
  std::string16 name;
  std::string16 required_cookie;
  std::string16 error_message;
  if (!GetAndCheckParameters(false, &name, &required_cookie, &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  //LOG(_T("LocalServer::RemoveStore( %s, %s )\n"),
  //       name, required_cookie_bstr.m_str);

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (!ResourceStore::ExistsInDB(EnvPageSecurityOrigin(),
                                 name.c_str(),
                                 required_cookie.c_str(),
                                 &existing_store_id)) {
    RETURN_NORMAL();
  }

  ResourceStore store;
  if (!store.Open(existing_store_id)) {
    RETURN_EXCEPTION(STRING16(L"Error initializing ResourceStore."));
  }

  if (!store.Remove()) {
    RETURN_EXCEPTION(STRING16(L"Error removing ResourceStore."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// GetAndCheckParameters
//------------------------------------------------------------------------------
bool GearsLocalServer::GetAndCheckParameters(
                                         bool has_string_retval,
                                         std::string16 *name,
                                         std::string16 *required_cookie,
                                         std::string16 *error_message) {
  JsParamFetcher js_params(this);
  const int kNameParamIndex = 0;
  const int kRequiredCookieParamIndex = 1;

  if (js_params.GetCount(has_string_retval) < 1) {
    *error_message = STRING16(L"The name parameter is required.");
    return false;
  }

  // Get required parameters
  if (!js_params.GetAsString(kNameParamIndex, name)) {
    *error_message = STRING16(L"The name parameter must be a string.");
    return false;
  }

  // Get optional parameters
  if (js_params.IsOptionalParamPresent(kRequiredCookieParamIndex,
                                       has_string_retval)) {
    if (!js_params.GetAsString(kRequiredCookieParamIndex, required_cookie)) {
      *error_message = STRING16(L"The required_cookie parameter must be a "
                                L"string.");
      return false;
    }
  }

  // Validate parameters
  if (name->empty()) {
    *error_message = STRING16(L"The name parameter is required.");
    return false;
  }

  if (!IsUserInputValidAsPathComponent(*name, error_message)) {
    return false;
  }

  return true;
}
