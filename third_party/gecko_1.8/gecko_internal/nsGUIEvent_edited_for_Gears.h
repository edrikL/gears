// NOTE(nigeltao): This is an edited version of Gecko's nsGUIEvent.h, being
// reduced to only those declarations needed by Gears' drag-and-drop.
//
// This editing (which only consisted of deletions, not additions) was because
// trying to #include the full nsGUIEvent.h pulls in other #include files as
// dependencies (e.g. from XPCOM and nsString), which pull in further
// dependencies (e.g. hashtables, weak pointers), and so on until we'd more or
// less have to include large chunks of Gecko's header files inside
// third_party/gecko_1.?/gecko_internal.
//
// To avoid that (since all we really need is some #define's of integer
// constants, plus some struct definitions), we have edited nsGUIEvent.h.










/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 *   Makoto Kato  <m_kato@ga2.so-net.ne.jp>
 *   Dean Tessman <dean_tessman@hotmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsGUIEvent_h__
#define nsGUIEvent_h__

#include "nsPoint.h"
#include "nsEvent.h"


class nsHashKey;
class nsIWidget;


#define NS_EVENT                           1
#define NS_GUI_EVENT                       2
#define NS_SIZE_EVENT                      3
#define NS_SIZEMODE_EVENT                  4
#define NS_ZLEVEL_EVENT                    5
#define NS_PAINT_EVENT                     6
#define NS_SCROLLBAR_EVENT                 7
#define NS_INPUT_EVENT                     8
#define NS_KEY_EVENT                       9
#define NS_MOUSE_EVENT                    10
#define NS_MENU_EVENT                     11
#define NS_SCRIPT_ERROR_EVENT             12
#define NS_TEXT_EVENT                     13
#define NS_COMPOSITION_EVENT              14
#define NS_RECONVERSION_EVENT             15
#define NS_MOUSE_SCROLL_EVENT             16
#define NS_SCROLLPORT_EVENT               18
#define NS_MUTATION_EVENT                 19 // |nsMutationEvent| in content
#define NS_ACCESSIBLE_EVENT               20
#define NS_FORM_EVENT                     21
#define NS_FOCUS_EVENT                    22
#define NS_POPUP_EVENT                    23
#define NS_APPCOMMAND_EVENT               24
#define NS_POPUPBLOCKED_EVENT             25
#define NS_BEFORE_PAGE_UNLOAD_EVENT       26
#define NS_UI_EVENT                       27
#define NS_QUERYCARETRECT_EVENT           28
#define NS_PAGETRANSITION_EVENT               29
#ifdef MOZ_SVG
#define NS_SVG_EVENT                      30
#define NS_SVGZOOM_EVENT                  31
#endif // MOZ_SVG
#define NS_XUL_COMMAND_EVENT              32


#define NS_EVENT_FLAG_NONE                0x0000
#define NS_EVENT_FLAG_INIT                0x0001
#define NS_EVENT_FLAG_BUBBLE              0x0002
#define NS_EVENT_FLAG_CAPTURE             0x0004
#define NS_EVENT_FLAG_STOP_DISPATCH       0x0008
#define NS_EVENT_FLAG_NO_DEFAULT          0x0010
#define NS_EVENT_FLAG_CANT_CANCEL         0x0020
#define NS_EVENT_FLAG_CANT_BUBBLE         0x0040
#define NS_PRIV_EVENT_FLAG_SCRIPT         0x0080
#define NS_EVENT_FLAG_NO_CONTENT_DISPATCH 0x0100
#define NS_EVENT_FLAG_SYSTEM_EVENT        0x0200
#define NS_EVENT_FLAG_STOP_DISPATCH_IMMEDIATELY 0x0400 // @see nsIDOM3Event::stopImmediatePropagation()
#define NS_EVENT_FLAG_DISPATCHING         0x0800


#define NS_APP_EVENT_FLAG_NONE      0x00000000
#define NS_APP_EVENT_FLAG_HANDLED   0x00000001
#define NS_APP_EVENT_FLAG_TRUSTED   0x00000002


#define NS_MOUSE_MESSAGE_START          300
#define NS_MOUSE_MOVE                   (NS_MOUSE_MESSAGE_START)


class nsEvent
{
protected:
  nsEvent(PRBool isTrusted, PRUint32 msg, PRUint8 structType)
    : eventStructType(structType),
      message(msg),
      point(0, 0),
      refPoint(0, 0),
      time(0),
      flags(0),
      internalAppFlags(isTrusted ? NS_APP_EVENT_FLAG_TRUSTED :
                       NS_APP_EVENT_FLAG_NONE),
      userType(0)
  {
  }

public:
  nsEvent(PRBool isTrusted, PRUint32 msg)
    : eventStructType(NS_EVENT),
      message(msg),
      point(0, 0),
      refPoint(0, 0),
      time(0),
      flags(0),
      internalAppFlags(isTrusted ? NS_APP_EVENT_FLAG_TRUSTED :
                       NS_APP_EVENT_FLAG_NONE),
      userType(0)
  {
  }

  // See event struct types
  PRUint8     eventStructType;
  // See GUI MESSAGES,
  PRUint32    message;              
  // In widget relative coordinates, modified to be relative to
  // current view in layout.
  nsPoint     point;               
  // In widget relative coordinates, not modified by layout code.
  nsPoint     refPoint;               
  // Elapsed time, in milliseconds, from the time the system was
  // started to the time the message was created
  PRUint32    time;      
  // Flags to hold event flow stage and capture/bubble cancellation
  // status
  PRUint32    flags;
  // Flags for indicating more event state for Mozilla applications.
  PRUint32    internalAppFlags;
  // Additional type info for user defined events
  nsHashKey*  userType;
};


class nsGUIEvent : public nsEvent
{
protected:
  nsGUIEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w, PRUint8 structType)
    : nsEvent(isTrusted, msg, structType),
      widget(w), nativeMsg(nsnull)
  {
  }

public:
  nsGUIEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsEvent(isTrusted, msg, NS_GUI_EVENT),
      widget(w), nativeMsg(nsnull)
  {
  }

  /// Originator of the event
  nsIWidget*  widget;           
  /// Internal platform specific message.
  void*     nativeMsg;        
};


class nsInputEvent : public nsGUIEvent
{
protected:
  nsInputEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w,
               PRUint8 structType)
    : nsGUIEvent(isTrusted, msg, w, structType),
      isShift(PR_FALSE), isControl(PR_FALSE), isAlt(PR_FALSE), isMeta(PR_FALSE)
  {
  }

public:
  nsInputEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_INPUT_EVENT),
      isShift(PR_FALSE), isControl(PR_FALSE), isAlt(PR_FALSE), isMeta(PR_FALSE)
  {
  }

  /// PR_TRUE indicates the shift key is down
  PRBool          isShift;        
  /// PR_TRUE indicates the control key is down
  PRBool          isControl;      
  /// PR_TRUE indicates the alt key is down
  PRBool          isAlt;          
  /// PR_TRUE indicates the meta key is down (or, on Mac, the Command key)
  PRBool          isMeta;
};


class nsMouseEvent : public nsInputEvent
{
public:
  enum reasonType { eReal, eSynthesized };

  nsMouseEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w,
               reasonType aReason)
    : nsInputEvent(isTrusted, msg, w, NS_MOUSE_EVENT),
      clickCount(0), acceptActivation(PR_FALSE), reason(aReason)
  {
    if (msg == NS_MOUSE_MOVE) {
      flags |= NS_EVENT_FLAG_CANT_CANCEL;
    }
  }

  /// The number of mouse clicks
  PRUint32        clickCount;          
  /// Special return code for MOUSE_ACTIVATE to signal
  /// if the target accepts activation (1), or denies it (0)
  PRPackedBool    acceptActivation;           
  reasonType      reason : 8;
};


#endif // nsGUIEvent_h__
