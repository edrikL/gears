// Copyright 2007, Google Inc.
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

#ifndef GEARS_DESKTOP_DESKTOP_IE_H__
#define GEARS_DESKTOP_DESKTOP_IE_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/desktop/desktop_utils.h"
#include "ie/genfiles/desktop_ie.h"  // from OUTDIR

class GearsDesktop
    : public ModuleImplBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsDesktop>,
      public IDispatchImpl<GearsDesktopInterface> {
 public:
  BEGIN_COM_MAP(GearsDesktop)
    COM_INTERFACE_ENTRY(GearsDesktopInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsDesktop)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  // End boilerplate code. Begin interface.

  // need a default constructor to instance objects from the Factory
  GearsDesktop() {}
  ~GearsDesktop() {}

  STDMETHOD(createShortcut)(BSTR name, BSTR description, BSTR url,
                            VARIANT icons);

#ifdef OFFICIAL_BUILD
  // Blob support is not ready for prime time yet
#else
#ifdef DEBUG
  STDMETHOD(newFileBlob)(const BSTR filename, IUnknown **retval);
#endif  // DEBUG
#endif  // OFFICIAL_BUILD

  static bool GetControlPanelIconLocation(const SecurityOrigin &origin,
                                          const std::string16 &app_name,
                                          std::string16 *icon_loc);

 private:
  bool SetShortcut(DesktopUtils::ShortcutInfo *shortcut, std::string16 *error);

  bool WriteControlPanelIcon(const DesktopUtils::ShortcutInfo &shortcut);
  bool FetchIcon(DesktopUtils::IconData *icon, int expected_size, 
                 std::string16 *error);
  bool ResolveUrl(std::string16 *url, std::string16 *error);

  DISALLOW_EVIL_CONSTRUCTORS(GearsDesktop);
};

#endif // GEARS_DESKTOP_DESKTOP_IE_H__