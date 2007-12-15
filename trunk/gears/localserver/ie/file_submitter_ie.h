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

#ifndef GEARS_LOCALSERVER_IE_FILE_SUBMITTER_IE_H__
#define GEARS_LOCALSERVER_IE_FILE_SUBMITTER_IE_H__

#include "ie/genfiles/interfaces.h" // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/localserver/common/resource_store.h"

//------------------------------------------------------------------------------
// GearsFileSubmitter
//
// Facilitates the inclusion of captured local files in form submissions by
// associating the file to be submitted with a nested element within the <form>.
// Due to limitations in IE, usage of this API differs from its usage in FF.
// In IE, it cannot be used with <input type=file> elements; whereas in FF
// it can only be used with that type of element.
// TODO(michaeln): make this better
//
// IE DHTML Usage:
//
//  <form method="post" enctype="multipart/form-data" action="http://blah..."/>
//    <link id="gearsFileInput" name="formFieldName">
//    ... other form elements ...
//  </form>
//  <script>
//    var fileSubmitter = store.createFileSubmitter();
//    fileSubmitter.setFileInputElement(gearsFileInput, "keyOfFileToSubmit");
//  </script>
//
// @see SubmitFileBehavior
//------------------------------------------------------------------------------
class ATL_NO_VTABLE GearsFileSubmitter
    : public ModuleImplBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsFileSubmitter>,
      public IElementBehaviorFactory,
      public IDispatchImpl<GearsFileSubmitterInterface> {
 public:
  BEGIN_COM_MAP(GearsFileSubmitter)
    COM_INTERFACE_ENTRY(GearsFileSubmitterInterface)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IElementBehaviorFactory)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsFileSubmitter)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  // End boilerplate code. Begin interface.

  // need a default constructor to CreateInstance objects in IE
  GearsFileSubmitter() {}

  // GearsFileSubmitterInterface
  // This is the interface we expose to JavaScript.

  STDMETHOD(setFileInputElement)(IDispatch *file_input_element,
                                 const BSTR captured_url_key);

  // IElementBehaviorFactory
  // We implement this interface to assist with automatically attaching
  // the SubmitFileBehavior to elements when needed.

  STDMETHOD(FindBehavior)(BSTR name, BSTR url,
                          IElementBehaviorSite* behavior_site,
                          IElementBehavior** behavior_out);

 private:
  ResourceStore store_;

  friend class GearsResourceStore;

  DISALLOW_EVIL_CONSTRUCTORS(GearsFileSubmitter);
};

#endif  // GEARS_LOCALSERVER_IE_FILE_SUBMITTER_IE_H__
