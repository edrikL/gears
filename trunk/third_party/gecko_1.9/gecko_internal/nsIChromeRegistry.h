/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM c:/firefox-3.0-source/mozilla/content/base/public/nsIChromeRegistry.idl
 */

#ifndef __gen_nsIChromeRegistry_h__
#define __gen_nsIChromeRegistry_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIURI; /* forward declaration */


/* starting interface:    nsIChromeRegistry */
#define NS_ICHROMEREGISTRY_IID_STR "68389281-f6d0-4533-841d-344a2018140c"

#define NS_ICHROMEREGISTRY_IID \
  {0x68389281, 0xf6d0, 0x4533, \
    { 0x84, 0x1d, 0x34, 0x4a, 0x20, 0x18, 0x14, 0x0c }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIChromeRegistry : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICHROMEREGISTRY_IID)

  enum { NONE = 0 };

  enum { PARTIAL = 1 };

  enum { FULL = 2 };

  /**
   * Resolve a chrome URL to an loadable URI using the information in the
   * registry. Does not modify aChromeURL.
   *
   * Chrome URLs are allowed to be specified in "shorthand", leaving the
   * "file" portion off. In that case, the URL is expanded to:
   *
   *   chrome://package/provider/package.ext
   *
   * where "ext" is:
   *
   *   "xul" for a "content" package,
   *   "css" for a "skin" package, and
   *   "dtd" for a "locale" package.
   *
   * @param aChromeURL the URL that is to be converted.
   */
  /* nsIURI convertChromeURL (in nsIURI aChromeURL); */
  NS_SCRIPTABLE NS_IMETHOD ConvertChromeURL(nsIURI *aChromeURL, nsIURI **_retval) = 0;

  /**
   * refresh the chrome list at runtime, looking for new packages/etc
   */
  /* void checkForNewChrome (); */
  NS_SCRIPTABLE NS_IMETHOD CheckForNewChrome(void) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIChromeRegistry, NS_ICHROMEREGISTRY_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSICHROMEREGISTRY \
  NS_SCRIPTABLE NS_IMETHOD ConvertChromeURL(nsIURI *aChromeURL, nsIURI **_retval); \
  NS_SCRIPTABLE NS_IMETHOD CheckForNewChrome(void); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSICHROMEREGISTRY(_to) \
  NS_SCRIPTABLE NS_IMETHOD ConvertChromeURL(nsIURI *aChromeURL, nsIURI **_retval) { return _to ConvertChromeURL(aChromeURL, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD CheckForNewChrome(void) { return _to CheckForNewChrome(); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSICHROMEREGISTRY(_to) \
  NS_SCRIPTABLE NS_IMETHOD ConvertChromeURL(nsIURI *aChromeURL, nsIURI **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->ConvertChromeURL(aChromeURL, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD CheckForNewChrome(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->CheckForNewChrome(); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsChromeRegistry : public nsIChromeRegistry
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHROMEREGISTRY

  nsChromeRegistry();

private:
  ~nsChromeRegistry();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsChromeRegistry, nsIChromeRegistry)

nsChromeRegistry::nsChromeRegistry()
{
  /* member initializers and constructor code */
}

nsChromeRegistry::~nsChromeRegistry()
{
  /* destructor code */
}

/* nsIURI convertChromeURL (in nsIURI aChromeURL); */
NS_IMETHODIMP nsChromeRegistry::ConvertChromeURL(nsIURI *aChromeURL, nsIURI **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void checkForNewChrome (); */
NS_IMETHODIMP nsChromeRegistry::CheckForNewChrome()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    nsIXULChromeRegistry */
#define NS_IXULCHROMEREGISTRY_IID_STR "2860e205-490e-4b06-90b6-87160d35a5a7"

#define NS_IXULCHROMEREGISTRY_IID \
  {0x2860e205, 0x490e, 0x4b06, \
    { 0x90, 0xb6, 0x87, 0x16, 0x0d, 0x35, 0xa5, 0xa7 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIXULChromeRegistry : public nsIChromeRegistry {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXULCHROMEREGISTRY_IID)

  /* void reloadChrome (); */
  NS_SCRIPTABLE NS_IMETHOD ReloadChrome(void) = 0;

  /* ACString getSelectedLocale (in ACString packageName); */
  NS_SCRIPTABLE NS_IMETHOD GetSelectedLocale(const nsACString & packageName, nsACString & _retval) = 0;

  /* void refreshSkins (); */
  NS_SCRIPTABLE NS_IMETHOD RefreshSkins(void) = 0;

  /**
   * Installable skin XBL is not always granted the same privileges as other
   * chrome. This asks the chrome registry whether scripts are allowed to be
   * run for a particular chrome URI. Do not pass non-chrome URIs to this
   * method.
   */
  /* boolean allowScriptsForPackage (in nsIURI url); */
  NS_SCRIPTABLE NS_IMETHOD AllowScriptsForPackage(nsIURI *url, PRBool *_retval) = 0;

  /**
   * Content should only be allowed to load chrome JS from certain packages.
   * This method reflects the contentaccessible flag on packages.
   * Do not pass non-chrome URIs to this method.
   */
  /* boolean allowContentToAccess (in nsIURI url); */
  NS_SCRIPTABLE NS_IMETHOD AllowContentToAccess(nsIURI *url, PRBool *_retval) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIXULChromeRegistry, NS_IXULCHROMEREGISTRY_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIXULCHROMEREGISTRY \
  NS_SCRIPTABLE NS_IMETHOD ReloadChrome(void); \
  NS_SCRIPTABLE NS_IMETHOD GetSelectedLocale(const nsACString & packageName, nsACString & _retval); \
  NS_SCRIPTABLE NS_IMETHOD RefreshSkins(void); \
  NS_SCRIPTABLE NS_IMETHOD AllowScriptsForPackage(nsIURI *url, PRBool *_retval); \
  NS_SCRIPTABLE NS_IMETHOD AllowContentToAccess(nsIURI *url, PRBool *_retval); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIXULCHROMEREGISTRY(_to) \
  NS_SCRIPTABLE NS_IMETHOD ReloadChrome(void) { return _to ReloadChrome(); } \
  NS_SCRIPTABLE NS_IMETHOD GetSelectedLocale(const nsACString & packageName, nsACString & _retval) { return _to GetSelectedLocale(packageName, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD RefreshSkins(void) { return _to RefreshSkins(); } \
  NS_SCRIPTABLE NS_IMETHOD AllowScriptsForPackage(nsIURI *url, PRBool *_retval) { return _to AllowScriptsForPackage(url, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD AllowContentToAccess(nsIURI *url, PRBool *_retval) { return _to AllowContentToAccess(url, _retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIXULCHROMEREGISTRY(_to) \
  NS_SCRIPTABLE NS_IMETHOD ReloadChrome(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->ReloadChrome(); } \
  NS_SCRIPTABLE NS_IMETHOD GetSelectedLocale(const nsACString & packageName, nsACString & _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetSelectedLocale(packageName, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD RefreshSkins(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->RefreshSkins(); } \
  NS_SCRIPTABLE NS_IMETHOD AllowScriptsForPackage(nsIURI *url, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AllowScriptsForPackage(url, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD AllowContentToAccess(nsIURI *url, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AllowContentToAccess(url, _retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsXULChromeRegistry : public nsIXULChromeRegistry
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXULCHROMEREGISTRY

  nsXULChromeRegistry();

private:
  ~nsXULChromeRegistry();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsXULChromeRegistry, nsIXULChromeRegistry)

nsXULChromeRegistry::nsXULChromeRegistry()
{
  /* member initializers and constructor code */
}

nsXULChromeRegistry::~nsXULChromeRegistry()
{
  /* destructor code */
}

/* void reloadChrome (); */
NS_IMETHODIMP nsXULChromeRegistry::ReloadChrome()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* ACString getSelectedLocale (in ACString packageName); */
NS_IMETHODIMP nsXULChromeRegistry::GetSelectedLocale(const nsACString & packageName, nsACString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void refreshSkins (); */
NS_IMETHODIMP nsXULChromeRegistry::RefreshSkins()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean allowScriptsForPackage (in nsIURI url); */
NS_IMETHODIMP nsXULChromeRegistry::AllowScriptsForPackage(nsIURI *url, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean allowContentToAccess (in nsIURI url); */
NS_IMETHODIMP nsXULChromeRegistry::AllowContentToAccess(nsIURI *url, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif

#define NS_CHROMEREGISTRY_CONTRACTID \
  "@mozilla.org/chrome/chrome-registry;1"
/**
 * Chrome registry will notify various caches that all chrome files need
 * flushing.
 */
#define NS_CHROME_FLUSH_TOPIC \
  "chrome-flush-caches"
/**
 * Chrome registry will notify various caches that skin files need flushing.
 * If "chrome-flush-caches" is notified, this topic will *not* be notified.
 */
#define NS_CHROME_FLUSH_SKINS_TOPIC \
  "chrome-flush-skin-caches"

#endif /* __gen_nsIChromeRegistry_h__ */
