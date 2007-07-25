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

#import <WebKit/WebKit.h>

#import "gears/base/common/sqlite_wrapper.h"
#import "gears/base/common/stopwatch.h"
#import "gears/base/common/common_sf.h"
#import "gears/database/safari/result_set.h"

#ifdef DEBUG
extern Stopwatch g_database_stopwatch_;
#endif

@interface GearsResultSet(PrivateMethods)
- (NSNumber *)isValidRow;
- (void)next;
- (bool)nextImpl:(NSString **)error;
- (NSNumber *)fieldCount;
- (NSString *)fieldName:(NSNumber *)index;
- (id)field:(NSNumber *)index;
- (id)fieldByName:(NSString *)name;
- (void)close;
@end

@implementation GearsResultSet
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Public ||
//------------------------------------------------------------------------------
- (bool)setStatement:(sqlite3_stmt *)statement error:(NSString **)error {
  assert(statement);
  assert(error);
  statement_ = statement;
  // convention: call next() when the statement is set
  bool succeeded = [self nextImpl:error];
  if (!succeeded || sqlite3_column_count(statement_) == 0)  {
    // Either an error occurred or this was a command that does
    // not return a row, so we can just close automatically
    [self close];
  }
  
  return succeeded;
}

//------------------------------------------------------------------------------
- (void)close {
  int result = sqlite3_finalize(statement_);
  statement_ = nil;
  isValidRow_ = false;
  
  if (fieldNames_) {
    NSFreeMapTable(fieldNames_);
    fieldNames_ = nil;
  }
  
  if (result != SQLITE_OK)
    ThrowExceptionKeyAndReturn(@"errorInFinalizeFmt", result);
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || ScourComponent ||
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"isValidRow", @"isValidRow",
    @"next", @"next",
    @"fieldCount", @"fieldCount",
    @"fieldName", @"fieldName:",
    @"field", @"field:",
    @"fieldByName", @"fieldByName:",
    @"close", @"close",
    nil];
}

//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptKeys {
  // No public keys
  return nil;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject ||
//------------------------------------------------------------------------------
- (void)dealloc {
  [self close];
  [super dealloc];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Private ||
//------------------------------------------------------------------------------
- (NSNumber *)isValidRow {
  return [NSNumber numberWithBool:isValidRow_];
}

//------------------------------------------------------------------------------
- (void)next {
  if (!statement_)
    return;
  
  NSString *error = nil;
  
  if (![self nextImpl:&error]) 
    [WebScriptObject throwException:error];
}

//------------------------------------------------------------------------------
- (bool)nextImpl:(NSString **)error {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&g_database_stopwatch_);
#endif
  
  int sql_status = sqlite3_step(statement_);

  switch(sql_status) {
    case SQLITE_ROW:
      isValidRow_ = true;
      break;
      
    case SQLITE_BUSY:
      // If there was a timeout (SQLITE_BUSY) the SQL row cursor did not
      // advance, so we don't reset is_valid_row_. If it was valid prior to
      // this call, it's still valid now.
      break;
    default:
      isValidRow_ = false;
      break;
  }
  
  bool succeeded = (sql_status == SQLITE_ROW) ||
    (sql_status == SQLITE_DONE) ||
    (sql_status == SQLITE_OK);
  if (!succeeded) {
    NSString *msg = StringWithLocalizedKey(@"databaseOperationFailed");
    std::string16 msg16, out16;
    [msg string16:&msg16];
    BuildSqliteErrorString(msg16.c_str(), sql_status, 
                           sqlite3_db_handle(statement_), &out16);
    [WebScriptObject throwException:
      [NSString stringWithString16:out16.c_str()]];
  }
  
  return succeeded;
}

//------------------------------------------------------------------------------
- (NSNumber *)fieldCount {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&g_database_stopwatch_);
#endif

  int count = 0;
  
  if (statement_)
    count = sqlite3_column_count(statement_);

  return [NSNumber numberWithInt:count];
}

//------------------------------------------------------------------------------
- (NSString *)fieldName:(NSNumber *)index {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&g_database_stopwatch_);
#endif
  
  if (!statement_)
    return nil;
  
  int fieldIndex = [index intValue];
  int fieldCount = sqlite3_column_count(statement_);
  NSString *result = nil;
  
  if ((fieldIndex < fieldCount) && (fieldIndex >= 0))
    result = [NSString stringWithUTF8String:
      sqlite3_column_name(statement_, fieldIndex)];

  return result;
}

//------------------------------------------------------------------------------
- (id)field:(NSNumber *)index {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&g_database_stopwatch_);
#endif

  if (!statement_)
    return nil;
  
  int fieldIndex = [index intValue];
  int fieldCount = sqlite3_column_count(statement_);
  id result = nil;

  if ((fieldIndex < fieldCount) && (fieldIndex >= 0)) {
    int type = sqlite3_column_type(statement_, fieldIndex);
    
    switch (type) {
      case SQLITE_INTEGER:
        result = [NSNumber numberWithLongLong:
          sqlite3_column_int64(statement_, fieldIndex)];
        break;
        
      case SQLITE_FLOAT:
        result = [NSNumber numberWithDouble:
          sqlite3_column_double(statement_, fieldIndex)];
        break;
        
      case SQLITE_TEXT:
        result = [NSString stringWithUTF8String:(const char *)
          sqlite3_column_text(statement_, fieldIndex)];
        break;
        
      case SQLITE_NULL:
        result = [NSNull null];
        break;
        
      default:
        ThrowExceptionKeyAndReturnNil(@"datatypeNotSupportedFmt", type);
    }
  }
  
  return result;
}

//------------------------------------------------------------------------------
- (id)fieldByName:(NSString *)name {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&g_database_stopwatch_);
#endif

  if (!statement_)
    return nil;
  
  // Cache the names in a map table
  if (!fieldNames_) {
    int count = sqlite3_column_count(statement_);
    
    fieldNames_ = NSCreateMapTable(NSObjectMapKeyCallBacks, 
                                   NSObjectMapValueCallBacks, count);
    
    for (int i = 0; i < count; ++i) {
      NSString *name = [NSString stringWithUTF8String:
        sqlite3_column_name(statement_, i)];
      NSNumber *index = [NSNumber numberWithInt:i];
      
      NSMapInsertKnownAbsent(fieldNames_, name, index);
    }
  }
  
  NSNumber *index = (NSNumber *)NSMapGet(fieldNames_, name);
  
  if (!index)
    ThrowExceptionKeyAndReturnNil(@"invalidFieldNameFmt", name);
  
  return [self field:index];
}

@end
