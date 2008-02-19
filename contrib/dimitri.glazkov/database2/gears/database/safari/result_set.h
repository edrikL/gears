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

// Implements the GearsResultSet as defined by the Gears API.
// It represents the result of a GearsDatabase.execute() statement.

#import "gears/base/safari/base_class.h"

@interface GearsResultSet : SafariGearsBaseClass {
 @private
  struct sqlite3_stmt *statement_;  // The result from sqlite (STRONG)
  NSMapTable *fieldNames_;          // The column names (STRONG)
  bool isValidRow_;                 // If the current row is valid
}

//------------------------------------------------------------------------------
// Public
//------------------------------------------------------------------------------
// Will take ownership of |statement|
- (bool)setStatement:(sqlite3_stmt *)statement error:(NSString **)error;
- (void)close;

//------------------------------------------------------------------------------
// SafariGearsBaseClass
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings;
+ (NSDictionary *)webScriptKeys;

@end
