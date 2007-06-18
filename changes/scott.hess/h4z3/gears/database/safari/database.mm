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

#import "gears/base/common/paths.h"
#import "gears/base/common/sqlite_wrapper.h"
#import "gears/base/common/timer.h"
#import "gears/base/safari/factory_utils.h"
#import "gears/base/common/paths_sf_more.h"
#import "gears/base/common/common_sf.h"
#import "gears/base/safari/string_utils.h"
#import "gears/base/safari/browser_utils.h"
#import "gears/database/common/database_utils.h"
#import "gears/database/safari/database.h"
#import "gears/database/safari/result_set.h"

#ifdef DEBUG
Timer g_database_timer_;
#endif

@class GearsResultSet;

@interface GearsDatabase(PrivateMethods)
- (int)bindArg:(id)arg index:(int)idx statement:(sqlite3_stmt *)statement;
- (BOOL)bindArgs:(id)args statement:(sqlite3_stmt *)statement;
- (GearsResultSet *)execute:(NSString *)expr array:(id)array;
- (void)close;
@end

@implementation GearsDatabase
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || SafariGearsBaseClass ||
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"open", @"open:",
    @"execute", @"execute:array:",
    @"close", @"close",
    nil];
}

//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptKeys {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"lastInsertRowId", @"lastInsertRowId_",
#ifdef DEBUG
    @"executeMsec", @"executeMsec_",
#endif
    nil];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject (NSKeyValueCoding) ||
//------------------------------------------------------------------------------
- (void)setValue:(id)value forKey:(NSString *)key {
  // None allowed
}

//------------------------------------------------------------------------------
- (id)valueForKey:(NSString *)key {
  if ([key isEqualToString:@"lastInsertRowId_"]) {
    if (db_)
      return [NSNumber numberWithLongLong:sqlite3_last_insert_rowid(db_)];
    
    ThrowExceptionKey(@"databaseHandleNULL");
  }

#ifdef DEBUG
  if ([key isEqualToString:@"executeMsec_"]) {
    return [NSNumber numberWithFloat:g_database_timer_.GetElapsed()];
  }
#endif
  
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
- (void)open:(NSString *)database_name {
  if (db_)
    ThrowExceptionKeyAndReturn(@"databaseAlreadyOpen");
  
  // Get the database_name arg (if caller passed it in).
  if (database_name && database_name != (NSString *)[WebUndefined undefined]) {
    if ([database_name isKindOfClass:[NSString class]]) {
      if ([database_name length]) {
        if (!IsStringValidPathComponent([database_name fileSystemRepresentation]))
          ThrowExceptionKeyAndReturn(@"invalidNameFmt", database_name);
      }
    } else {
      ThrowExceptionKeyAndReturn(@"invalidNameFmt", database_name);
    }
  } else {
    database_name = @""; // Convert undefined to empty string
  }

  // Convert the strings
  std::string16 database_name_str;

  if (![database_name string16:&database_name_str])
    ThrowExceptionKeyAndReturn(@"invalidParameter");
    
  // Open the database.
  if (!OpenSqliteDatabase(database_name_str.c_str(), 
                          base_->EnvPageSecurityOrigin(), &db_)) {
    ThrowExceptionKeyAndReturn(@"unableToOpenDB");
  }
  
  const int kSQLiteBusyTimeout = 5000;
  sqlite3_busy_timeout(db_, kSQLiteBusyTimeout);  
}

//------------------------------------------------------------------------------
- (int)bindArg:(id)arg index:(int)index statement:(sqlite3_stmt *)statement {
  int result = 0;
  
  // SQLite index is 1-based
  if (arg == [NSNull null])  {
    result = sqlite3_bind_null(statement, index + 1);
  } else {
    const char *argStr = NULL;
    if ([arg isKindOfClass:[NSString class]])
      argStr = [arg UTF8String];
    else if ([arg respondsToSelector:@selector(stringValue)])
      argStr = [[arg stringValue] UTF8String];
    else if (arg == [WebUndefined undefined]) {
      // TODO(miket): We'd like to come up with a more principled approach
      // to undefined and missing parameters. For now, this is consistent
      // with the Firefox implementation.
      argStr = "undefined";
    }
    
    if (argStr)
      result = sqlite3_bind_text(statement, index + 1, argStr, strlen(argStr), 
                                 SQLITE_TRANSIENT);
  }
  
  return result;
}

//------------------------------------------------------------------------------
- (BOOL)bindArgs:(id)args statement:(sqlite3_stmt *)statement {
  // Convert any arguments from WebScriptObject to NSArray
  NSArray *converted = [SafariGearsBaseClass convertWebScriptArray:args];

  // Check if there was an error in converting the arguments
  if (converted == (NSArray *)[NSNull null])
    ThrowExceptionKeyAndReturnNo(@"invalidParameter");
  
  int num_args = [converted count];
  int num_args_expected = sqlite3_bind_parameter_count(statement);
  
  // check that the correct number of SQL arguments were passed
  if (num_args_expected != num_args)
    ThrowExceptionKeyAndReturnNil(@"errorInBindingFmt",
                                  num_args_expected, num_args);
  
  // Bind each arg to its sql param
  for (int i = 0; i < num_args; ++i) {
    id arg = [converted objectAtIndex:i];
    
    if ([self bindArg:arg index:i statement:statement] != 0)
      ThrowExceptionKeyAndReturnNo(@"invalidParameter");
  }
  
  return YES;
}

//------------------------------------------------------------------------------
- (GearsResultSet *)execute:(NSString *)expr array:(id)array {
#ifdef DEBUG
  ScopedTimer scoped_timer(&g_database_timer_);
#endif // DEBUG
  
  if (!db_)
    ThrowExceptionKeyAndReturnNil(@"databaseNotOpen");

  // Build the query 
  const char *exprStr = [expr UTF8String];
  scoped_sqlite3_stmt_ptr stmt;
  int result = sqlite3_prepare_v2(db_, exprStr, strlen(exprStr), &stmt, NULL);
  
  if (result != SQLITE_OK || !stmt.get()) {
    NSString *msg = StringWithLocalizedKey(@"errorInPreparingQuery");
    std::string16 msg16, out16;
    [msg string16:&msg16];
    BuildSqliteErrorString(msg16.c_str(), result, db_, &out16);
    [WebScriptObject throwException:
      [NSString stringWithString16:out16.c_str()]];
    return nil;
  }

  if (![self bindArgs:array statement:stmt.get()])
    return nil;

  // Note the ResultSet takes ownership of the statement
  NSString *error = nil;
  GearsResultSet *resultSet = [[[GearsResultSet alloc] init] autorelease];
  
  if (![resultSet setStatement:stmt.release() error:&error]) {
    if (error)
      [WebScriptObject throwException:error];
    return nil;
  }
  
  return resultSet;
}

//------------------------------------------------------------------------------
- (void)close {
  sqlite3_close(db_);
  db_ = nil;
}

@end
