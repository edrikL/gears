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
#include "gears/base/common/base_class.h"

#ifndef HIBYTE
#define HIBYTE(x) ((((uint32)(x)) & 0xff00) >> 8)
#endif

NPNetscapeFuncs NPNFuncs;

NPError OSCALL NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
  if (pFuncs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if (pFuncs->size < sizeof(NPPluginFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  pFuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
  pFuncs->newp          = NPP_New;
  pFuncs->destroy       = NPP_Destroy;
  pFuncs->setwindow     = NPP_SetWindow;
  pFuncs->newstream     = NPP_NewStream;
  pFuncs->destroystream = NPP_DestroyStream;
  pFuncs->asfile        = NPP_StreamAsFile;
  pFuncs->writeready    = NPP_WriteReady;
  pFuncs->write         = NPP_Write;
  pFuncs->print         = NPP_Print;
  pFuncs->event         = NPP_HandleEvent;
  pFuncs->urlnotify     = NPP_URLNotify;
  pFuncs->getvalue      = NPP_GetValue;
  pFuncs->setvalue      = NPP_SetValue;
  pFuncs->javaClass     = NULL;

  return NPERR_NO_ERROR;
}

NPError OSCALL NP_Initialize(NPNetscapeFuncs* pFuncs)
{
  if (pFuncs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if (HIBYTE(pFuncs->version) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  if (pFuncs->size < sizeof(NPNetscapeFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  NPNFuncs = *pFuncs;

  return NPERR_NO_ERROR;
}

NPError OSCALL NP_Shutdown()
{
  return NPERR_NO_ERROR;
}
