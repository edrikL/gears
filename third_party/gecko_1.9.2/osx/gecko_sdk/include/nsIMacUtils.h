/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM /builds/slave/mozilla-1.9.2-macosx-xulrunner/build/xpcom/base/nsIMacUtils.idl
 */

#ifndef __gen_nsIMacUtils_h__
#define __gen_nsIMacUtils_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIMacUtils */
#define NS_IMACUTILS_IID_STR "f6fc107c-5cba-4c5c-a35e-b69d580d1db6"

#define NS_IMACUTILS_IID \
  {0xf6fc107c, 0x5cba, 0x4c5c, \
    { 0xa3, 0x5e, 0xb6, 0x9d, 0x58, 0x0d, 0x1d, 0xb6 }}

/**
 * nsIMacUtils: Generic globally-available Mac-specific utilities.
 */
class NS_NO_VTABLE NS_SCRIPTABLE nsIMacUtils : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMACUTILS_IID)

  /**
   * True when the main executable is a fat file supporting at least
   * ppc and x86 (universal binary).
   */
  /* readonly attribute boolean isUniversalBinary; */
  NS_SCRIPTABLE NS_IMETHOD GetIsUniversalBinary(PRBool *aIsUniversalBinary) = 0;

  /**
   * True when running under binary translation (Rosetta).
   */
  /* readonly attribute boolean isTranslated; */
  NS_SCRIPTABLE NS_IMETHOD GetIsTranslated(PRBool *aIsTranslated) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIMacUtils, NS_IMACUTILS_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIMACUTILS \
  NS_SCRIPTABLE NS_IMETHOD GetIsUniversalBinary(PRBool *aIsUniversalBinary); \
  NS_SCRIPTABLE NS_IMETHOD GetIsTranslated(PRBool *aIsTranslated); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIMACUTILS(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetIsUniversalBinary(PRBool *aIsUniversalBinary) { return _to GetIsUniversalBinary(aIsUniversalBinary); } \
  NS_SCRIPTABLE NS_IMETHOD GetIsTranslated(PRBool *aIsTranslated) { return _to GetIsTranslated(aIsTranslated); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIMACUTILS(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetIsUniversalBinary(PRBool *aIsUniversalBinary) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIsUniversalBinary(aIsUniversalBinary); } \
  NS_SCRIPTABLE NS_IMETHOD GetIsTranslated(PRBool *aIsTranslated) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIsTranslated(aIsTranslated); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsMacUtils : public nsIMacUtils
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMACUTILS

  nsMacUtils();

private:
  ~nsMacUtils();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsMacUtils, nsIMacUtils)

nsMacUtils::nsMacUtils()
{
  /* member initializers and constructor code */
}

nsMacUtils::~nsMacUtils()
{
  /* destructor code */
}

/* readonly attribute boolean isUniversalBinary; */
NS_IMETHODIMP nsMacUtils::GetIsUniversalBinary(PRBool *aIsUniversalBinary)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean isTranslated; */
NS_IMETHODIMP nsMacUtils::GetIsTranslated(PRBool *aIsTranslated)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIMacUtils_h__ */
