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

#include "gears/base/safari/string_utils.h"
#include "gears/factory/safari/factory_utils.h"
#include "common/genfiles/product_name_constants.h"  // OUTDIR

//------------------------------------------------------------------------------
GearsFactory::GearsFactory()
    : is_permission_granted_(false),
      is_permission_value_from_user_(false) {
}

//------------------------------------------------------------------------------
void ShowPermissionsPrompt(const SecurityOrigin &origin,
                           bool *allow_origin,
                           bool *remember_choice) {
  // Initialize to default values
  *allow_origin = false;
  *remember_choice = false;

  // Show something more user-friendly for kUnknownDomain.
  // TODO(aa): This is needed by settings dialog too. Factor this out into a
  // common utility.
  std::string16 display_origin(origin.url());
  if (origin.host() == kUnknownDomain) {
    ReplaceAll(display_origin,
               std::string16(kUnknownDomain),
               std::string16(STRING16(L"<no domain>")));
  }
  
  // Setup the contents of the dialog:  Allow, Deny buttons and a "remember"
  // checkbox.  The |display_origin| will be included in the message as well
  // as the product name.
  NSString *productStr = 
    [NSString stringWithUTF8String:PRODUCT_FRIENDLY_NAME_ASCII];
  NSString *originStr = [NSString stringWithString16:display_origin.c_str()];
  NSString *msg = 
    StringWithLocalizedKey(@"capabilitiesPromptFmt", originStr, productStr);
  NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
    productStr, kCFUserNotificationAlertHeaderKey,
    msg, kCFUserNotificationAlertMessageKey,
    StringWithLocalizedKey(@"Allow"), kCFUserNotificationDefaultButtonTitleKey,
    StringWithLocalizedKey(@"Deny"), kCFUserNotificationAlternateButtonTitleKey,
    StringWithLocalizedKey(@"rememberDecision"),
      kCFUserNotificationCheckBoxTitlesKey,
    nil];
  CFTimeInterval timeout = 30;  // Timeout after 30 seconds
  SInt32 error;  
  CFUserNotificationRef note =
    CFUserNotificationCreate(kCFAllocatorDefault, timeout, 
        kCFUserNotificationCautionAlertLevel, &error,
        (CFDictionaryRef)dict);
  // If there was an error creating the dialog, return
  if (!note)
    return;
  
  // Display the dialog and wait for a response or the timeout
  CFOptionFlags flags;
  if (CFUserNotificationReceiveResponse(note, timeout, &flags) == 0) {
    // The index of the button pressed is in the low 2 bits
    if ((flags & 0x03) == kCFUserNotificationDefaultResponse)
      *allow_origin = true;
    
    if (flags & CFUserNotificationCheckBoxChecked(0))
      *remember_choice = true;
  } else 
    CFUserNotificationCancel(note); // Required to dismiss
  
  CFRelease(note);
}
