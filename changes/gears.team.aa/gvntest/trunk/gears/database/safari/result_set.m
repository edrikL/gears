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

//=============================================================================
//  result_set.m
//  ScourSafari
//=============================================================================
#import <WebKit/WebKit.h>

#import "gears/third_party/sqlite_google/preprocessed/sqlite3.h"

#import "gears/database/safari/result_set.h"

@interface GOBSResultSet(PrivateMethods)
- (NSNumber *)isValidRow;
- (void)next;
- (NSNumber *)fieldCount;
- (NSString *)fieldName:(NSNumber *)idx;
- (id)field:(NSNumber *)idx;
- (id)fieldByName:(NSString *)name;
- (void)close;
@end

@implementation GOBSResultSet
//=============================================================================
#pragma mark -
#pragma mark || Public ||
//=============================================================================
- (id)initWithStatement:(sqlite3_stmt *)statement {
  if (self = [super init]) {
    statement_ = statement;
    isValidRow_ = (statement_) ? YES : NO;
    
    // Convention in the other Scour implementations (e.g., IE and FF)
    // is that the statement should be automatically stepped
    [self next];
    
    // If the statement didn't have a return value, cleanup automatically
    if (!sqlite3_column_count(statement_))
      [self close];
  }
  
  return self;
}

//=============================================================================
#pragma mark -
#pragma mark || WebScripting (informal protocol) ||
//=============================================================================
+ (NSString *)webScriptNameForSelector:(SEL)sel {
  if (sel == @selector(fieldName:))
    return @"fieldName";

  if (sel == @selector(field:))
    return @"field";

  if (sel == @selector(fieldByName:))
    return @"fieldByName";

  return @"";
}

//=============================================================================
+ (BOOL)isSelectorExcludedFromWebScript:(SEL)sel {
  if (sel == @selector(fieldName:))
    return NO;

  if (sel == @selector(isValidRow))
    return NO;
  
  if (sel == @selector(next))
    return NO;
  
  if (sel == @selector(fieldCount))
    return NO;
  
  if (sel == @selector(fieldName:))
    return NO;
  
  if (sel == @selector(field:))
    return NO;

  if (sel == @selector(fieldByName:))
    return NO;

  if (sel == @selector(close))
    return NO;

  return YES;
}

//=============================================================================
+ (BOOL)isKeyExcludedFromWebScript:(const char *)property {
  // No public keys
  return YES;
}

//=============================================================================
#pragma mark -
#pragma mark || NSObject ||
//=============================================================================
- (void)dealloc {
  [self close];
  [super dealloc];
}

//=============================================================================
- (NSString *)description {
  NSString *str = @"No statement";
  
  if (statement_)
    str = [NSString stringWithFormat:@"%d columns", 
      sqlite3_column_count(statement_)];
  
  return [NSString stringWithFormat:@"<%@: 0x%x>: %@",
    NSStringFromClass([self class]), self, str];
}

//=============================================================================
#pragma mark -
#pragma mark || Private ||
//=============================================================================
- (NSNumber *)isValidRow {
  return [NSNumber numberWithBool:isValidRow_];
}

//=============================================================================
- (void)next {
  if (!isValidRow_)
    return;
  
  int result = sqlite3_step(statement_);
  
  // These result values indicate that we're valid or in the middle of
  // a multi-row result
  if ((result == SQLITE_BUSY) || (result == SQLITE_ROW)) {
    isValidRow_ = YES;
  } else {
    // We've finished stepping through
    isValidRow_ = NO;

    if ((result != SQLITE_DONE) && (result != SQLITE_OK))
      [WebScriptObject throwException:@"SQLite step() failed"];
  }
}

//=============================================================================
- (NSNumber *)fieldCount {
  int count = 0;
  
  if (statement_)
    count = sqlite3_column_count(statement_);

  return [NSNumber numberWithInt:count];
}

//=============================================================================
- (NSString *)fieldName:(NSNumber *)idx {
  if (!statement_)
    return nil;
  
  int fieldIdx = [idx intValue];
  int fieldCount = sqlite3_column_count(statement_);
  NSString *result = nil;
  
  if ((fieldIdx < fieldCount) && (fieldIdx >= 0))
    result = [NSString stringWithUTF8String:
      sqlite3_column_name(statement_, fieldIdx)];

  return result;
}

//=============================================================================
- (id)field:(NSNumber *)idx {
  if (!statement_)
    return nil;
  
  int fieldIdx = [idx intValue];
  int fieldCount = sqlite3_column_count(statement_);
  id result = nil;

  if ((fieldIdx < fieldCount) && (fieldIdx >= 0)) {
    int type = sqlite3_column_type(statement_, fieldIdx);
    
    switch (type) {
      case SQLITE_INTEGER:
        result = [NSNumber numberWithLongLong:
          sqlite3_column_int64(statement_, fieldIdx)];
        break;
        
      case SQLITE_FLOAT:
        result = [NSNumber numberWithDouble:
          sqlite3_column_double(statement_, fieldIdx)];
        break;
        
      case SQLITE_TEXT:
        result = [NSString stringWithUTF8String:(const char *)
          sqlite3_column_text(statement_, fieldIdx)];
        break;
        
      case SQLITE_NULL:
        result = [NSNull null];
        break;
        
      case SQLITE_BLOB: // Return as null-terminated text
        result = [NSString stringWithUTF8String:(const char *)
          sqlite3_column_text(statement_, fieldIdx)];
        break;
        
      default:
        [WebScriptObject throwException:
          [NSString stringWithFormat:@"Datatype: %d not supported", type]];
    }
  }
  
  return result;
}

//=============================================================================
- (id)fieldByName:(NSString *)name {
  if (!statement_)
    return nil;
  
  // Cache the names in a map table
  if (! fieldNames_) {
    int count = sqlite3_column_count(statement_);
    
    fieldNames_ = NSCreateMapTable(NSObjectMapKeyCallBacks, 
                                   NSObjectMapValueCallBacks, count);
    
    for (int i = 0; i < count; ++i) {
      NSString *name = [NSString stringWithUTF8String:
        sqlite3_column_name(statement_, i)];
      NSNumber *idx = [NSNumber numberWithInt:i];
      
      NSMapInsertKnownAbsent(fieldNames_, name, idx);
    }
  }
  
  NSNumber *fieldIdx = NSMapGet(fieldNames_, name);
  
  return fieldIdx ? [self field:fieldIdx] : nil;
}

//=============================================================================
- (void)close {
  if (!statement_)
    return;

  int result = sqlite3_finalize(statement_);
  statement_ = nil;
  isValidRow_ = NO;
  
  if (fieldNames_)
    NSFreeMapTable(fieldNames_);
  
  if (result != SQLITE_OK)
    [WebScriptObject throwException:@"SQLite finalize() failed"];
}

@end
