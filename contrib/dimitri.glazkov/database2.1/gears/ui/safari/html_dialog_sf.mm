// Copyright 2008, Google Inc.
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

#import <Cocoa/Cocoa.h>
#include <WebKit/WebKit.h>

#include "gears/base/common/string16.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/paths_sf_more.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/safari/cf_string_utils.h"
#include "gears/base/safari/scoped_cf.h"
#include "gears/ui/common/html_dialog.h"

// Class to display a Modal dialog in WebKit.
@interface HTMLDialogImp  : NSObject {
 @private
  NSString *dialog_url_;  // strong
  NSString *arguments_;  // strong
  int width_;
  int height_;
  bool dialog_dismissed_;
  NSString *result_string_;  // strong
  NSWindow *dialog_;
}

// Initialize an HTMLDialogImp instance.
- (id)initWithHtmlFile:(const std::string16 &)html_filename 
             arguments:(const std::string16 &)arguments
                width:(int)width
               height:(int)height;

// Show the modal dialog and block until said dialog is dismissed.
// returns: true on success.
- (bool)showModal: (std::string16 *)results;
@end

@implementation HTMLDialogImp

#pragma mark Public Instance methods
- (id)initWithHtmlFile:(const std::string16 &)html_filename 
             arguments:(const std::string16 &)arguments
                 width:(int)width
                height:(int)height {
  self = [super init];
  if (self) {
    // TODO(playmobil): Get path for localized files.
    std::string16 localized_html_file(STRING16(L"en-US/"));
    localized_html_file += html_filename;
    NSString *pluginPath = [GearsPathUtilities gearsResourcesDirectory];
    NSString *tmp = [NSString stringWithString16:localized_html_file.c_str()];
    pluginPath = [pluginPath stringByAppendingPathComponent:tmp];
    dialog_url_ = [NSString stringWithFormat:@"file:///%@", pluginPath]; 
    [dialog_url_ retain];
    arguments_ = [[NSString stringWithString16:arguments.c_str()] retain];
    width_ = width;
    height_ = height;
  }
  return self;
}

- (void)dealloc {
  [dialog_url_ release];
  [arguments_ release];
  [result_string_ release];
  [super dealloc];
}

- (bool)showModal:(std::string16 *)results {
  // Create the dialog's window.
  NSRect content_rect = NSMakeRect(0, 0, width_, height_);
  unsigned int dialog_style = NSTitledWindowMask|NSResizableWindowMask;
  dialog_ = [[NSWindow alloc]  initWithContentRect:content_rect 
                                         styleMask:dialog_style
                                           backing:NSBackingStoreBuffered
                                             defer:YES];

  if (!dialog_) {
    return false;
  }
  
  // Create a WebView and attach it.
  NSView *content_view = [dialog_ contentView];
  WebView *webview = [[WebView alloc] initWithFrame:[content_view frame]];
  
  // Set the user agent for the WebView that we're opening to the same one as
  // Safari.
  std::string16 ua_str16;
  BrowserUtils::GetUserAgentString(&ua_str16);
  NSString *user_agent = [NSString stringWithString16:ua_str16.c_str()];
  [webview setCustomUserAgent:user_agent];

  [content_view addSubview:webview];
  [webview release]; // addSubView retains webView.
  [webview setAutoresizingMask: NSViewHeightSizable | NSViewWidthSizable];
  
  // Make ourselves the WebView's delegate.
  [webview setFrameLoadDelegate: self];
  
  // Load in the dialog's contents.
  NSURL *url = [NSURL URLWithString:dialog_url_];
  NSURLRequest *url_request = [NSURLRequest requestWithURL:url]; 
  WebFrame *frame = [webview mainFrame];
  [frame loadRequest: url_request];
  
  // Turn off scrollbars.
  [[frame frameView] setAllowsScrolling:NO];

  // Display sheet.
  NSWindow *front_window = [NSApp keyWindow];
  [NSApp beginSheet:dialog_ 
     modalForWindow:front_window 
      modalDelegate:nil 
     didEndSelector:nil 
        contextInfo:nil];
  
  // Credit goes to David Sinclair of Dejal software for this method of running
  // a modal WebView.
  NSModalSession session = [NSApp beginModalSessionForWindow:front_window];
  while (!dialog_dismissed_ && 
         [NSApp runModalSession:session] == NSRunContinuesResponse) {
    [[NSRunLoop currentRunLoop] limitDateForMode:NSDefaultRunLoopMode];
  }
  [NSApp endModalSession:session];
  [NSApp endSheet:dialog_];
  [dialog_ close];
  dialog_ = nil;
  
  [result_string_ string16:results];
  return true;
}
@end

@implementation HTMLDialogImp(GearsWebViewDelegateMethods)
// Delegate method called just before any js is run, so this is the perfect
// time to inject vairables into the js environment.
- (void)webView:(WebView *)webView 
            windowScriptObjectAvailable:(WebScriptObject *)windowScriptObject {
      
  // Pass in dialog arguments in the gears_dialogArguments variable.
  [windowScriptObject setValue:arguments_
                        forKey:@"gears_dialogArguments"];
  
  // Register ourselves as the window.gears_dialog object.
  [windowScriptObject setValue:self forKey:@"gears_dialog"];
}

// Called by WebView to determine which of this object's selectors can be called
// from JS.
+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector {
  // Only allow calls to setResults:.
  if (selector == @selector(setResults:)) {
    return NO;
  }
  return YES;
}

+ (NSString *)webScriptNameForSelector:(SEL)sel {
  if (sel == @selector(setResults:)) {
    return @"setResults";
  } else {
    return nil;
  }
}

// JS callback for setting the result string.
- (void)setResults: (NSString *)dialog_results {
  result_string_ = [dialog_results copy];
  dialog_dismissed_ = true;
}
@end

// Bring plugin's container tab & window to front.
// This is a bit of a hack because we have no way to tell which window the
// Gears plugin is running in, so once we do this we know it's the frontmost
// window.  It's also a better experience for the user to have the dialog
// appear in relevant context.
static bool FocusPluginContainerWindow() {
  JsCallContext *call_context = BrowserUtils::GetCurrentJsCallContext();
  NPObject *window;
  if (NPN_GetValue(call_context->js_context(), NPNVWindowNPObject, &window) != 
      NPERR_NO_ERROR) {
    return false;
  }
  
  std::string script_utf8("window.focus()");
  NPString script = {script_utf8.data(), script_utf8.length()};
  ScopedNPVariant foo;
  if (!NPN_Evaluate(call_context->js_context(), window, &script, &foo)) {
    return false;
  }
    
  return true;
}

bool HtmlDialog::DoModalImpl(const char16 *html_filename, int width, int height,
                             const char16 *arguments_string) {
  if (!FocusPluginContainerWindow()) {
    return false;
  }
  
  scoped_objctype<HTMLDialogImp *> dialog([[HTMLDialogImp alloc]
                                              initWithHtmlFile:html_filename 
                                                     arguments:arguments_string
                                                         width:width
                                                        height:height]);
  
  if (!dialog.get()) {
    return false;
  }
  std::string16 results;
  if (![dialog.get() showModal:&results]) {
    return false;
  }
  
  SetResult(results.c_str());
  return true;
}
