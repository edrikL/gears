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

struct JSContext; // must declare this before including nsIJSContextStack.h
#include <gecko_sdk/include/nsIURI.h>
#include <gecko_sdk/include/nsIIOService.h>
#include <gecko_sdk/include/nsIDOMWindow.h>
#include <gecko_sdk/include/nsIDOMHTMLInputElement.h>
#include <gecko_sdk/include/nsIDOMEventTarget.h>
#include <gecko_sdk/include/nsIDOMDocument.h>
#include <gecko_sdk/include/nsIInterfaceRequestor.h>
#include <gecko_internal/jsapi.h>
#include <gecko_internal/nsIBaseWindow.h>
#include <gecko_internal/nsIDocShell.h>
#include <gecko_internal/nsIDocShellTreeItem.h>
#include <gecko_internal/nsIDocShellTreeOwner.h>
#if defined(GECKO_19)
#include <gecko_internal/nsDOMJSUtils.h>
#endif
#include <gecko_internal/nsIDOM3Node.h>
#include <gecko_internal/nsIDOMWindowInternal.h>
#include <gecko_internal/nsIInterfaceInfoManager.h>
#include <gecko_internal/nsIJSContextStack.h>
#include <gecko_internal/nsIScriptContext.h>
#include <gecko_internal/nsIScriptGlobalObject.h>
#include <gecko_internal/nsIScriptSecurityManager.h>
#include <gecko_internal/nsIWebNavigation.h>
#include <gecko_internal/nsIXPConnect.h>
#include <gecko_internal/nsIXULWindow.h>
#include "gears/base/common/common.h"
#include "gears/base/common/security_model.h"
#include "gears/base/firefox/dom_utils.h"

#if defined(LINUX) && !defined(OS_MACOSX)
#include <gtk/gtk.h>
#endif

// The IID for nsIContent in different versions of Firefox/Gecko
// TODO(michaeln): Add to this list as new versions show up

#if defined(GECKO_19)
// Firefox 3.0.x
#define NS_ICONTENT_IID_GECKO190 \
{ 0xd3434698, 0x3a16, 0x4dbe, \
  { 0x9d, 0xed, 0xbe, 0x64, 0x16, 0x1a, 0xa3, 0x52 } }
#else
// Firefox 1.5.0.x
#define NS_ICONTENT_IID_GECKO180 \
{ 0x3fecc374, 0x2839, 0x4db3, \
  { 0x8d, 0xe8, 0x6b, 0x76, 0xd1, 0xd8, 0xe6, 0xf6 } }

// Firefox 2.0.0.x
#define NS_ICONTENT_IID_GECKO181 \
{ 0x9d059608, 0xddb0, 0x4e6a, \
  { 0x99, 0x69, 0xd2, 0xf3, 0x63, 0xa1, 0xb5, 0x57 } }

#endif

static const nsIID kPossibleNsContentIIDs[] = {
#if defined(GECKO_19)
      NS_ICONTENT_IID_GECKO190,
#else
      NS_ICONTENT_IID_GECKO180,
      NS_ICONTENT_IID_GECKO181,
#endif
    };


bool DOMUtils::GetJsContext(JSContext **context) {
  // Get JSContext from stack.
  nsCOMPtr<nsIJSContextStack> stack =
      do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  if (!stack) { return false; }

  JSContext *cx;
  if (NS_FAILED(stack->Peek(&cx)) || !cx) {
    return false;
  }

  *context = cx;  // only modify output param on success
  return true;
}


nsresult DOMUtils::GetScriptContext(nsIScriptContext **context) {
  JSContext *cx;
  if (!GetJsContext(&cx)) {
    return NS_ERROR_FAILURE;
  }

  *context = GetScriptContextFromJSContext(cx);
  if (*context) {
    // TODO(miket): We found a case where GetScriptContextFromJSContext() fails
    // if Toolbar is installed with safebrowsing. Safebrowsing wants to grab
    // https://www.google.com/safebrowsing/getkey? on startup, and for some
    // reason there's no JSContext at that point. We (I and Darin) decided that
    // ignoring that problem with this test is probably the equivalent of
    // whatever the real right thing is to do.
    NS_ADDREF(*context);
  }
  return NS_OK;
}


nsresult DOMUtils::GetDocument(nsIDOMDocument **document) {
  nsCOMPtr<nsIDOMWindowInternal> window;
  GetWindow(getter_AddRefs(window));

  return window->GetDocument(document);
}


nsresult DOMUtils::GetWindow(nsIDOMWindowInternal **result) {
  nsCOMPtr<nsIScriptContext> sc;
  GetScriptContext(getter_AddRefs(sc));
  NS_ENSURE_STATE(sc);

  return CallQueryInterface(sc->GetGlobalObject(), result);
}

nsresult DOMUtils::GetNativeWindow(NativeWindowPtr* window) {
  nsIDOMWindowInternal* internal_window = NULL;
  nsresult nr = GetWindow(&internal_window);
  if (NS_FAILED(nr))
    return nr;

  nsCOMPtr<nsIInterfaceRequestor> requestor(do_QueryInterface(internal_window,
                                                              &nr));
  if (NS_FAILED(nr))
    return nr;

  nsCOMPtr<nsIWebNavigation> nav;
  nr = requestor->GetInterface(NS_GET_IID(nsIWebNavigation),
                               getter_AddRefs(nav));
  if (NS_FAILED(nr))
    return nr;

  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(nav, &nr));
  if (NS_FAILED(nr))
    return nr;

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  nr = treeItem->GetTreeOwner(getter_AddRefs(treeOwner));
  if (NS_FAILED(nr))
    return nr;

  requestor = do_QueryInterface(treeOwner, &nr);
  if (NS_FAILED(nr))
    return nr;

  nsCOMPtr<nsIXULWindow> xulWindow;
  nr = requestor->GetInterface(NS_GET_IID(nsIXULWindow),
                               getter_AddRefs(xulWindow));
  if (NS_FAILED(nr))
    return nr;

  nsCOMPtr<nsIDocShell> docShell;
  nr = xulWindow->GetDocShell(getter_AddRefs(docShell));
  if (NS_FAILED(nr))
    return nr;

  nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(docShell, &nr));
  if (NS_FAILED(nr))
    return nr;

  void* parentWindow = NULL;
  nr = baseWindow->GetParentNativeWindow(&parentWindow);
  if (!parentWindow || NS_FAILED(nr))
    return NS_ERROR_FAILURE;

#if defined(LINUX) && !defined(OS_MACOSX)
  GdkWindow* parentGdkWindow = reinterpret_cast<GdkWindow*>(parentWindow);
  gpointer user_data = NULL;
  gdk_window_get_user_data(gdk_window_get_toplevel(parentGdkWindow),
                           &user_data);
  if (!user_data)
    return NS_ERROR_FAILURE;

  *window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(user_data)));
#elif defined(WIN32)
  *window = reinterpret_cast<HWND>(parentWindow);
#else
  *window = parentWindow;
#endif

  return NS_OK;
}

nsresult DOMUtils::GetWindowEventTarget(nsIDOMEventTarget **target) {
  // WARNING: nsIDOMWindow2::GetWindowRoot() would return an nsIDOMEventTarget
  // that fires for _all_ tabs in the window!

  nsCOMPtr<nsIDOMWindowInternal> window_internal;
  nsresult nr = DOMUtils::GetWindow(getter_AddRefs(window_internal));
  if (NS_FAILED(nr) || !window_internal) { return NS_ERROR_FAILURE; }

  return CallQueryInterface(window_internal, target);
}


//
// This is a security measure to prevent script from spoofing DOM elements.
//
static bool VerifyNsContent(nsISupports *unknown) {
  if (!unknown) return false;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIInterfaceInfoManager> iface_info_manager;
  iface_info_manager = do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID,
                                     &rv);
  if (NS_FAILED(rv) || !iface_info_manager) { return false; }

  // The nsIContentIID is version dependent, we test for all we know about
  for (size_t i = 0; i < ARRAYSIZE(kPossibleNsContentIIDs); ++i) {
    const nsIID *ns_content_iid = &kPossibleNsContentIIDs[i];

    // Paranoia, ensure that the IID we query for is either unknown to the
    // interface manager or not scriptable. The XPConnect JSWrapper
    // QueryInterface implementation will not forward to script for such
    // interfaces. In Firefox 2.0 and 1.5, nsIContent is not known by
    // the interface manager.
    nsCOMPtr<nsIInterfaceInfo> iface_info;
    rv = iface_info_manager->GetInfoForIID(ns_content_iid,
                                           getter_AddRefs(iface_info));
    if (NS_SUCCEEDED(rv) && iface_info) {
      PRBool is_scriptable = PR_TRUE;
      rv = iface_info->IsScriptable(&is_scriptable);
      if (NS_FAILED(rv) || is_scriptable) {
        continue;  // Don't test for this interface id
      }
    }

    // Test if our 'unknown' argument implements nsIContent,
    // a positive test indicates 'unknown' is not script based.
    nsCOMPtr<nsISupports> nscontent;
    rv = unknown->QueryInterface(*ns_content_iid,
                                 reinterpret_cast<void**>(&nscontent));
    if (NS_SUCCEEDED(rv) && nscontent) {
      return true;
    }
  }

  return false;
}


nsresult DOMUtils::VerifyAndGetFileInputElement(nsISupports *unknown,
                                          nsIDOMHTMLInputElement **file_input) {
  // Verify 'unknown' is actually a content node from Gecko's DOM as opposed
  // to a JavaScript object pretending to be one.
  if (!VerifyNsContent(unknown))  return NS_ERROR_FAILURE;

  // Verify unknown is an <input> element
  nsCOMPtr<nsIDOMHTMLInputElement> input = do_QueryInterface(unknown);
  NS_ENSURE_ARG(input);

  // Verify it's type attibute is "file", <input type=file>
  nsString type;
  nsresult rv = input->GetType(type);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG(type.Equals(NS_LITERAL_STRING("file")));

  // Return a reference to the interface pointer
  *file_input = input;
  (*file_input)->AddRef();

  return NS_OK;
}


bool DOMUtils::NewResolvedURI(const char16 *base_url,
                              const char16 *url,
                              nsIURI **url_obj) {
  assert(base_url);
  assert(url);
  assert(url_obj);

  nsCOMPtr<nsIIOService> ios =
      do_GetService("@mozilla.org/network/io-service;1");
  if (!ios) { return false; }

  nsCOMPtr<nsIURI> base_url_obj;
  nsresult nr = ios->NewURI(NS_ConvertUTF16toUTF8(base_url),
                            nsnull, nsnull, getter_AddRefs(base_url_obj));
  if (NS_FAILED(nr)) { return false; }

  nr = ios->NewURI(NS_ConvertUTF16toUTF8(url),
                   nsnull, base_url_obj, url_obj);
  if (NS_FAILED(nr)) { return false; }
  return true;
}


bool DOMUtils::NewAbsoluteURI(const char16 *url, nsIURI **url_obj) {
  assert(url);
  assert(url_obj);

  nsCOMPtr<nsIIOService> ios =
      do_GetService("@mozilla.org/network/io-service;1");
  if (!ios) { return false; }

  nsresult nr = ios->NewURI(NS_ConvertUTF16toUTF8(url),
                            nsnull, nsnull, url_obj);
  if (NS_FAILED(nr)) { return false; }
  return true;
}


bool DOMUtils::GetPageLocation(std::string16 *location_url) {
  assert(location_url);
  nsresult nr;

  // get a nsIURI for the current page
  nsCOMPtr<nsIScriptSecurityManager> sec_man =
      do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &nr);
  if (NS_FAILED(nr) || !sec_man) { return false; }

  nsCOMPtr<nsIPrincipal> ppl;
  nr = sec_man->GetSubjectPrincipal(getter_AddRefs(ppl));
  if (NS_FAILED(nr) || !ppl) { return false; }

  nsCOMPtr<nsIURI> url;
  nr = ppl->GetURI(getter_AddRefs(url));
  if (NS_FAILED(nr) || !url) { return false; }

  // get the page location url
  nsCString out_utf8;
  nr = url->GetSpec(out_utf8);
  if (NS_FAILED(nr)) { return false; }

  location_url->assign(NS_ConvertUTF8toUTF16(out_utf8).get());

  return true;  // succeeded
}


bool DOMUtils::GetPageOrigin(SecurityOrigin *security_origin) {
  std::string16 location;
  if (!GetPageLocation(&location))
    return false;
  return security_origin->InitFromUrl(location.c_str());
}


bool DOMUtils::IsOnline() {
  nsCOMPtr<nsIIOService> ios = do_GetService(
                                    "@mozilla.org/network/io-service;1");
  NS_ENSURE_TRUE(ios, false);
  PRBool offline = PR_FALSE;
  ios->GetOffline(&offline);
  return offline ? false : true;
}
