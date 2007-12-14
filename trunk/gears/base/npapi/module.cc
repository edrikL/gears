/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//////////////////////////////////////////////////////////////
//
// Main plugin entry point implementation
//
#include "gears/base/npapi/module.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/thread_locals.h"

#ifndef HIBYTE
#define HIBYTE(x) ((((uint32)(x)) & 0xff00) >> 8)
#endif

// Store the browser functions in thread local storage to avoid calling the
// functions on a different thread.
static NPNetscapeFuncs g_browser_funcs;
const std::string kNPNFuncsKey("base:NPNetscapeFuncs");

NPError OSCALL NP_GetEntryPoints(NPPluginFuncs* funcs)
{
  if (funcs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if (funcs->size < sizeof(NPPluginFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  funcs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
  funcs->newp          = NPP_New;
  funcs->destroy       = NPP_Destroy;
  funcs->setwindow     = NPP_SetWindow;
  funcs->newstream     = NPP_NewStream;
  funcs->destroystream = NPP_DestroyStream;
  funcs->asfile        = NPP_StreamAsFile;
  funcs->writeready    = NPP_WriteReady;
  funcs->write         = NPP_Write;
  funcs->print         = NPP_Print;
  funcs->event         = NPP_HandleEvent;
  funcs->urlnotify     = NPP_URLNotify;
  funcs->getvalue      = NPP_GetValue;
  funcs->setvalue      = NPP_SetValue;
  funcs->javaClass     = NULL;

  return NPERR_NO_ERROR;
}

NPError OSCALL NP_Initialize(NPNetscapeFuncs* funcs)
{
  if (funcs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if (HIBYTE(funcs->version) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  if (funcs->size < sizeof(NPNetscapeFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  MyDllMain(0, DLL_PROCESS_ATTACH, 0);

  g_browser_funcs = *funcs;
  ThreadLocals::SetValue(kNPNFuncsKey, &g_browser_funcs, NULL);

  return NPERR_NO_ERROR;
}

NPError OSCALL NP_Shutdown()
{
  MyDllMain(0, DLL_PROCESS_DETACH, 0);
  return NPERR_NO_ERROR;
}

BOOL MyDllMain(HANDLE instance, DWORD reason, LPVOID reserved) {
  switch (reason) {
    case DLL_THREAD_DETACH:
      ThreadLocals::HandleThreadDetached();
      break;
    case DLL_PROCESS_DETACH:
      ThreadLocals::HandleProcessDetached();
      break;
    case DLL_PROCESS_ATTACH:
      ThreadLocals::HandleProcessAttached();
      break;
  }

  return TRUE;
}
