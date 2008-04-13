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

var db_manager = google.gears.factory.create('beta.databasemanager');

function testDatabaseManagerCreation() {
  assertNotNull(db_manager, 'Database manager should be a value'); 
}

function testDatabaseManagerApiSig() {
  var method = 'DatabaseManager.openDatabase()';
  assert(db_manager.openDatabase, method + ' should be present');
  assertError(function() {
    db_manager.openDatabase();
  }, null, method + ' has required parameters');
  assertError(function() {
    db_manager.openDatabase('unit_test_db');
  }, null, method + ' requires a version parameter');
}

function testDatabaseOpening() {
  var db = db_manager.openDatabase('unit_test_db', '', 'ignored_description', 
    'ignored_estimated_size');
  assert(db, 'Database should be a value');
}

function testDatabaseApiSig() {
  withDb(function(db) {
    var method = 'Database2.transaction()';
    assert(db.transaction, method + ' should be present');
    assertError(function() {
      db.transaction();
    }, null, method + ' requires a callback');
    
    var method = 'Database2.synchronousTransaction()';
    assert(db.synchronousTransaction, method + ' should be present');
    
    var method = 'Database2.changeVersion()';
    assert(db.changeVersion, method + ' should be present');
  });
}

function testDatabaseTransaction() {
  withDb(function(db) {
    var method = 'Database2.transaction()';
    var inFlight;
    var outOfOrder = 0;
    // this tests the fact that the transaction callback is called 
    // asynchronously and whether the callback is invoked in a proper sequence
    // if startAsync times out, the callback is not invoked at all
    db.transaction(function(tx) {
      inFlight = true;
      outOfOrder--;
      if (outOfOrder) {
        assert(false, method + 'invokes callback out of sequence');
      }
      completeAsync();
    });
    outOfOrder++;
    if (inFlight) {
      assert(false, method + 'is not called asynchronously');
    }
    startAsync();
  });
}

function testSQLTransactionApiSig() {
  withDb(function(db) {
    var method = 'SQLTransaction.executeSql';
    db.transaction(function(tx) {
      assert(tx.executeSQL, method + ' should be present');
    });
  });
}

function withDb(fn, version) {
  db_manager && fn && fn.call(
    this, db_manager.openDatabase('unit_test_db', version || ''));
}
