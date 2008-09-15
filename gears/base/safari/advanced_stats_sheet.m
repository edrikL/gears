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

#import "gears/base/safari/advanced_stats_sheet.h"

@interface AdvancedStatsSheetPane (PrivateMethods)
// Helper method to set the stats pref key
- (void)setStatsKeyInGoogleInstallerPrefs:(BOOL)enabled;
@end


@implementation AdvancedStatsSheetPane

- (NSString *)title {
  return [[NSBundle bundleForClass:[self class]] 
             localizedStringForKey:@"PaneTitle" value:nil table:nil];
}

- (void)willEnterPane:(InstallerSectionDirection)dir {
  if (dir == InstallerDirectionBackward) {
    // We should skip showing the sheet when they enter this pane 
    // going backwards.  When they re-enter this pane going forwards 
    // we'll redisplay the sheet 
    [self gotoPreviousPane];
    return;
  }

  [NSApp beginSheet:statsPanel_
     modalForWindow:[NSApp mainWindow]
      modalDelegate:self
     didEndSelector:NULL
        contextInfo:nil];
  [NSApp runModalForWindow:statsPanel_];

  [NSApp endSheet:statsPanel_];

  [statsPanel_ orderOut:self];

  [self gotoNextPane];
}

- (IBAction)enableStats:(id)sender {
  [NSApp stopModal]; 

  [self setStatsKeyInGoogleInstallerPrefs:YES];
}

- (IBAction)disableStats:(id)sender {
  [NSApp stopModal];
  
  [self setStatsKeyInGoogleInstallerPrefs:NO];
}

@end

@implementation AdvancedStatsSheetPane (PrivateMethods)

- (void)setStatsKeyInGoogleInstallerPrefs:(BOOL)enabled {
  NSString *flag_file = @"~/.__enable_gears_stats";
  
  // Remove any existing flag_file.
  [[NSFileManager defaultManager] removeFileAtPath:flag_file handler:nil];
  
  // We need to set this preference globally which requires root privileges,
  // since this plugin runs before the installer is authenticated, we can't
  // do this at this stage, also environmental variables set here aren't
  // readable when the postflight script is run so we touch a file which 
  // the postflight script can then detect.
  // The global preference value which Gears reads is then written by the 
  // Postflight script.
  if (enabled) {
    NSString *flag_file_path = [flag_file stringByExpandingTildeInPath];
    [@"YES" writeToFile:flag_file_path 
             atomically:NO
               encoding:NSUTF8StringEncoding
                  error:NULL];
  }
}

@end
