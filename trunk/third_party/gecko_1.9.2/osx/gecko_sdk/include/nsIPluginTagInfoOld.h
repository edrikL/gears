/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM /builds/slave/mozilla-1.9.2-macosx-xulrunner/build/modules/plugin/base/public/nsIPluginTagInfoOld.idl
 */

#ifndef __gen_nsIPluginTagInfoOld_h__
#define __gen_nsIPluginTagInfoOld_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

#ifndef __gen_nspluginroot_h__
#include "nspluginroot.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
#include "nsplugindefs.h"
class nsIDOMElement; /* forward declaration */


/* starting interface:    nsIPluginTagInfoOld */
#define NS_IPLUGINTAGINFOOLD_IID_STR "5f1ec1d0-019b-11d2-815b-006008119d7a"

#define NS_IPLUGINTAGINFOOLD_IID \
  {0x5f1ec1d0, 0x019b, 0x11d2, \
    { 0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a }}

/**
 * Plugin Tag Info Interface
 * This interface provides information about the HTML tag on the page.
 * Some day this might get superseded by a DOM API.
 */
class NS_NO_VTABLE nsIPluginTagInfoOld : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPLUGINTAGINFOOLD_IID)

  /**
   * QueryInterface on nsIPluginInstancePeer to get this.
   *
   * (Corresponds to NPP_New's argc, argn, and argv arguments.)
   * Get a ptr to the paired list of attribute names and values,
   * returns the length of the array.
   *
   * Each name or value is a null-terminated string.
   */
  /* void getAttributes (in PRUint16Ref aCount, in constCharStarConstStar aNames, in constCharStarConstStar aValues); */
  NS_IMETHOD GetAttributes(PRUint16 & aCount, const char* const* & aNames, const char* const* & aValues) = 0;

  /**
     * Gets the value for the named attribute.
     *
   * @param aName   - the name of the attribute to find
   * @param aResult - the resulting attribute
     * @result - NS_OK if this operation was successful, NS_ERROR_FAILURE if
     * this operation failed. result is set to NULL if the attribute is not found
     * else to the found value.
     */
  /* void getAttribute (in string aName, out constCharPtr aResult); */
  NS_IMETHOD GetAttribute(const char *aName, const char * *aResult NS_OUTPARAM) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIPluginTagInfoOld, NS_IPLUGINTAGINFOOLD_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIPLUGINTAGINFOOLD \
  NS_IMETHOD GetAttributes(PRUint16 & aCount, const char* const* & aNames, const char* const* & aValues); \
  NS_IMETHOD GetAttribute(const char *aName, const char * *aResult NS_OUTPARAM); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIPLUGINTAGINFOOLD(_to) \
  NS_IMETHOD GetAttributes(PRUint16 & aCount, const char* const* & aNames, const char* const* & aValues) { return _to GetAttributes(aCount, aNames, aValues); } \
  NS_IMETHOD GetAttribute(const char *aName, const char * *aResult NS_OUTPARAM) { return _to GetAttribute(aName, aResult); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIPLUGINTAGINFOOLD(_to) \
  NS_IMETHOD GetAttributes(PRUint16 & aCount, const char* const* & aNames, const char* const* & aValues) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAttributes(aCount, aNames, aValues); } \
  NS_IMETHOD GetAttribute(const char *aName, const char * *aResult NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAttribute(aName, aResult); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsPluginTagInfoOld : public nsIPluginTagInfoOld
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINTAGINFOOLD

  nsPluginTagInfoOld();

private:
  ~nsPluginTagInfoOld();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsPluginTagInfoOld, nsIPluginTagInfoOld)

nsPluginTagInfoOld::nsPluginTagInfoOld()
{
  /* member initializers and constructor code */
}

nsPluginTagInfoOld::~nsPluginTagInfoOld()
{
  /* destructor code */
}

/* void getAttributes (in PRUint16Ref aCount, in constCharStarConstStar aNames, in constCharStarConstStar aValues); */
NS_IMETHODIMP nsPluginTagInfoOld::GetAttributes(PRUint16 & aCount, const char* const* & aNames, const char* const* & aValues)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getAttribute (in string aName, out constCharPtr aResult); */
NS_IMETHODIMP nsPluginTagInfoOld::GetAttribute(const char *aName, const char * *aResult NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIPluginTagInfoOld_h__ */
