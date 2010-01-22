/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM /builds/slave/mozilla-1.9.2-linux-xulrunner/build/content/base/public/nsIDOMFile.idl
 */

#ifndef __gen_nsIDOMFile_h__
#define __gen_nsIDOMFile_h__


#ifndef __gen_domstubs_h__
#include "domstubs.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIDOMFileError; /* forward declaration */


/* starting interface:    nsIDOMFile */
#define NS_IDOMFILE_IID_STR "0845e8ae-56bd-4f0e-962a-3b3e92638a0b"

#define NS_IDOMFILE_IID \
  {0x0845e8ae, 0x56bd, 0x4f0e, \
    { 0x96, 0x2a, 0x3b, 0x3e, 0x92, 0x63, 0x8a, 0x0b }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMFile : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMFILE_IID)

  /* readonly attribute DOMString fileName; */
  NS_SCRIPTABLE NS_IMETHOD GetFileName(nsAString & aFileName) = 0;

  /* readonly attribute unsigned long long fileSize; */
  NS_SCRIPTABLE NS_IMETHOD GetFileSize(PRUint64 *aFileSize) = 0;

  /* readonly attribute DOMString name; */
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName) = 0;

  /* readonly attribute unsigned long long size; */
  NS_SCRIPTABLE NS_IMETHOD GetSize(PRUint64 *aSize) = 0;

  /* readonly attribute DOMString type; */
  NS_SCRIPTABLE NS_IMETHOD GetType(nsAString & aType) = 0;

  /* DOMString getAsText (in DOMString encoding); */
  NS_SCRIPTABLE NS_IMETHOD GetAsText(const nsAString & encoding, nsAString & _retval NS_OUTPARAM) = 0;

  /* DOMString getAsDataURL (); */
  NS_SCRIPTABLE NS_IMETHOD GetAsDataURL(nsAString & _retval NS_OUTPARAM) = 0;

  /* DOMString getAsBinary (); */
  NS_SCRIPTABLE NS_IMETHOD GetAsBinary(nsAString & _retval NS_OUTPARAM) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMFile, NS_IDOMFILE_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMFILE \
  NS_SCRIPTABLE NS_IMETHOD GetFileName(nsAString & aFileName); \
  NS_SCRIPTABLE NS_IMETHOD GetFileSize(PRUint64 *aFileSize); \
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName); \
  NS_SCRIPTABLE NS_IMETHOD GetSize(PRUint64 *aSize); \
  NS_SCRIPTABLE NS_IMETHOD GetType(nsAString & aType); \
  NS_SCRIPTABLE NS_IMETHOD GetAsText(const nsAString & encoding, nsAString & _retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GetAsDataURL(nsAString & _retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GetAsBinary(nsAString & _retval NS_OUTPARAM); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMFILE(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetFileName(nsAString & aFileName) { return _to GetFileName(aFileName); } \
  NS_SCRIPTABLE NS_IMETHOD GetFileSize(PRUint64 *aFileSize) { return _to GetFileSize(aFileSize); } \
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName) { return _to GetName(aName); } \
  NS_SCRIPTABLE NS_IMETHOD GetSize(PRUint64 *aSize) { return _to GetSize(aSize); } \
  NS_SCRIPTABLE NS_IMETHOD GetType(nsAString & aType) { return _to GetType(aType); } \
  NS_SCRIPTABLE NS_IMETHOD GetAsText(const nsAString & encoding, nsAString & _retval NS_OUTPARAM) { return _to GetAsText(encoding, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetAsDataURL(nsAString & _retval NS_OUTPARAM) { return _to GetAsDataURL(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetAsBinary(nsAString & _retval NS_OUTPARAM) { return _to GetAsBinary(_retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMFILE(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetFileName(nsAString & aFileName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFileName(aFileName); } \
  NS_SCRIPTABLE NS_IMETHOD GetFileSize(PRUint64 *aFileSize) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFileSize(aFileSize); } \
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetName(aName); } \
  NS_SCRIPTABLE NS_IMETHOD GetSize(PRUint64 *aSize) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetSize(aSize); } \
  NS_SCRIPTABLE NS_IMETHOD GetType(nsAString & aType) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetType(aType); } \
  NS_SCRIPTABLE NS_IMETHOD GetAsText(const nsAString & encoding, nsAString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAsText(encoding, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetAsDataURL(nsAString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAsDataURL(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetAsBinary(nsAString & _retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAsBinary(_retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMFile : public nsIDOMFile
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFILE

  nsDOMFile();

private:
  ~nsDOMFile();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMFile, nsIDOMFile)

nsDOMFile::nsDOMFile()
{
  /* member initializers and constructor code */
}

nsDOMFile::~nsDOMFile()
{
  /* destructor code */
}

/* readonly attribute DOMString fileName; */
NS_IMETHODIMP nsDOMFile::GetFileName(nsAString & aFileName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute unsigned long long fileSize; */
NS_IMETHODIMP nsDOMFile::GetFileSize(PRUint64 *aFileSize)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute DOMString name; */
NS_IMETHODIMP nsDOMFile::GetName(nsAString & aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute unsigned long long size; */
NS_IMETHODIMP nsDOMFile::GetSize(PRUint64 *aSize)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute DOMString type; */
NS_IMETHODIMP nsDOMFile::GetType(nsAString & aType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString getAsText (in DOMString encoding); */
NS_IMETHODIMP nsDOMFile::GetAsText(const nsAString & encoding, nsAString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString getAsDataURL (); */
NS_IMETHODIMP nsDOMFile::GetAsDataURL(nsAString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString getAsBinary (); */
NS_IMETHODIMP nsDOMFile::GetAsBinary(nsAString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMFile_h__ */
