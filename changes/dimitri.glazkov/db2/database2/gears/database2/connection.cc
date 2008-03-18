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

#include "gears/database2/connection.h"
#include "gears/database2/row_handler_interface.h"

bool Database2Connection::OpenAndVerifyVersion(std::string16 databaseVersion) {
  // read expected_version (user_version value)
  // read version from Permissions.db
  // if databaseVersion is not an empty value or null,
    // if databaseVersion matches version
      // return true
    // otherwise,
      // set error to INVALID_STATE_ERR exception value
  // return true
  return false;
}

bool Database2Connection::Execute(Database2RowHandlerInterface *row_handler) {
  if (bogus_version_) {
   // set error code to "version mismatch" (error code 2)
  }
  // prepare
  // step, for each row, call row_handler->HandleRow(..);
   // if error, set error code and message, return false;
  // return true upon success
  return false;
}

bool Database2Connection::Begin() {
  // execute BEGIN    
  // if error, set error code and message, return false
  // read actual_version, if doesn't match expected_version_, 
    // set bogus_version_ flag
  // return true upon success
  return false;
}

bool Database2Connection::Rollback() {
  // execute COMMIT
  // if error, set error code and message, return false
  // return true upon success
  return false;
}

bool Database2Connection::Commit() {
  // execute ROLLBACK
  // don't remember or handle errors
  return false;
}

sqlite3 *Database2Connection::GetHandle() {
  // opend database if not already open,
  // used by all operations to obtain the handle
  return NULL;
}
