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

// INSTRUCTIONS FOR WRITING TESTS:
// - Each test case should be a separate function.
// - To propagate errors from your test case:
//     (a) if the routines being tested return error codes, look at the values
//         and return a cumulative 'success' bool from your test case.
//     (b) if the routines being tested only throw exceptions on failure,
//         simply execute the code and return g.SUCCEEDED. Any exception will
//         be caught by the test wrapper.

// TODO: we should be using single quotes instead of double quotes for strings,
// as required by the Google JavaScript guidelines.

var gIsDebugBuild;
var gIsChildWorker;

var gExpectedErrors = [];
var gReceivedErrors = [];

window.onerror = function(errorMessage) {
  for (var i = 0; i < gExpectedErrors.length; i++) {
    if (errorMessage.indexOf(gExpectedErrors[i]) > -1) {
      // This is an error we were expecting. Note down that we got it and
      // prevent the browser UI.
      gReceivedErrors.push(errorMessage);

      // Remove the error from the expected list so that we minimize the chance
      // we will remove other errors accidentally.
      gExpectedErrors.splice(i, 1);
      return true;
    }
  }

  // This is some other error. Fall through to the regular browser UI.
  return false;
};

function receivedError(errorMessage) {
  for (var i = 0; i < gReceivedErrors.length; i++) {
    if (gReceivedErrors[i].indexOf(errorMessage) > -1) {
      return true;
    }
  }
  return false;
}

function doit(document) {
  document.getElementById("browserInfo").innerHTML =
      "<b>Browser Version:</b> " + navigator.userAgent;

  var info = document.getElementById("gearsInfo");
  if (!google.gears.factory) {
    info.innerHTML = "Google Gears does not appear to be installed.";
    return;
  }

  var buildInfo = google.gears.factory.getBuildInfo();
  info.innerHTML = "<b>Google Gears Build:</b> " + buildInfo;

  if (buildInfo.indexOf("dbg") >= 0) {
    gIsDebugBuild = true;
  } else {
    gIsDebugBuild = false;
  }
  gIsChildWorker = 0;

  var queryString = window.location.search;
  var doAll = !queryString ||
              queryString.length == 0 ||
              queryString.indexOf('all') != -1
  
  // Note: Keep runWebCaptureTests() first in this list. Otherwise, when
  // debugging the C++ in the internal tests, you get stopped at every single
  // exception the other tests cause.
  if (doAll || queryString.indexOf('localserver') != -1) {
    runWebCaptureTests();
    runManifestTests();
  }
  if (doAll || queryString.indexOf('factory') != -1) {
    runFactoryTests();
  }
  if (doAll || queryString.indexOf('database') != -1) {
    runDatabaseTests();
  }
  if (doAll || queryString.indexOf('workerpool') != -1) {
    runWorkerPoolTests();
  }
  if (doAll || queryString.indexOf('timer') != -1) {
    runTimerTests();
  }
  if (doAll || queryString.indexOf('misc') != -1) {
    runMiscTests();
  }
  if (doAll || queryString.indexOf('httprequest') != -1) {
    runHttpRequestTests();
  }
  
  var c = document.getElementById("complete");
  c.innerHTML =
      "All unit tests <span class='success'>initiated without error</span>.";
}

function insertRow(name, result, reason, runTime) {
  var row = document.getElementById('myTable').insertRow(-1);
  row.insertCell(0).innerHTML = name;
  if (!reason) {
    reason = "";
  } else {
    reason = " (" + reason + ")";
  }
  if (runTime) {
    row.insertCell(1).innerHTML = runTime.toString();
  } else {
    row.insertCell(1).innerHTML = "---";
  }
  if (result) {
    row.insertCell(2).innerHTML = "<span class='success'>success" + reason +
                                  "</span>";
  } else {
    row.insertCell(2).innerHTML = "<span class='failure'>failure" + reason +
                                  "</span>";
  }
}

function insertExpected(name, expected, actual) {
  if (expected == actual) {
    insertRow(name, true);
  } else {
    insertRow(name, false, "Expected " + expected + " but got " + actual);
  }
}

function insertNotExpected(name, expected, actual) {
  if (expected == actual) {
    insertRow(name, false, "Expected anything but " + expected +
              " but got it");
  } else {
    insertRow(name, true);
  }
}

// Keep all globals in an object, and store in string format, so we can
// easily setup same globals in child worker.
var testGlobals =
  '{ db: null,' +

  '  TESTDB1: "GEARSUNITTEST112233",' +
  '  TESTINT: 99,' +
  '  TESTINT_UNDEFINED: 100,' +
  '  TESTINT_NULLINROW: 101,' +
  '  TESTFLOAT: 88.88,' +
  '  TESTSTR: "potato",' +
  '  TESTSTR_UNDEFINED: "undefined",' +

  '  SUCCEEDED: true,' +
  '  FAILED: false,' +

  '  CAPTURED_APP_NAME: "gearsUnitTestApp",' +
  '  UPDATE_STATUS_OK: 0,' +
  '  UPDATE_STATUS_FAILED: 3' +
  '}';

var g = eval('(' + testGlobals + ')');


//------------------------------------------------------------------------------
// Factory tests
//------------------------------------------------------------------------------

function factory_CheckVersionProperty() {
  var versionComponents = google.gears.factory.version.split('.');
  var major = versionComponents[0];
  var minor = versionComponents[1];
  // Update this test when the major/minor version changes (not very often).
  if (major != 0) {
    return g.FAILED;
  }
  if (minor != 2) {
    return g.FAILED;
  }
  return g.SUCCEEDED;
}


//
// Array of all tests in this category
//

var factoryTests = [
  factory_CheckVersionProperty
];

function runFactoryTests() {
  runArrayOfTests(factoryTests);
}

//------------------------------------------------------------------------------
// Database tests
//------------------------------------------------------------------------------

function initDB1() {
  g.db = google.gears.factory.create('beta.database', '1.0');
  g.db.open();

  var rs = g.db.execute("drop table if exists " + g.TESTDB1);
  rs.close();

  return g.SUCCEEDED;
}

function testDB1_CreateTable() {
  var rs = g.db.execute("create table " + g.TESTDB1 +
                        "(id int auto_increment primary key, " +
                        "myvarchar varchar(255), " +
                        "myint int, " +
                        "myfloat float, " +
                        "mydouble double " +
                        ")" );
  rs.close();
  return g.SUCCEEDED;
}

function testDB1_Insert() {
  g.db.execute("insert into " + g.TESTDB1 + " values (1, 'a', 2, 3, 4)");
  return g.SUCCEEDED;
}

function testDB1_DeleteAll() {
  g.db.execute("delete from " + g.TESTDB1);
  return g.SUCCEEDED;
}



function testDB1_RowCountIsZero() {
  var rs = g.db.execute("select count(id) from " + g.TESTDB1);
  var success = (0 == rs.field(0));
  rs.close();
  return success;
}

function testDB1_FieldCountIsFive() {
  var rs = g.db.execute("select * from " + g.TESTDB1);
  var success = (5 == rs.fieldCount());
  rs.close();
  return success;
}

function testDB1_IsValidRowWhenNoRowsReturned() {
  var rs = g.db.execute("select * from " + g.TESTDB1);
  var success = !rs.isValidRow();
  rs.close();
  return success;
}

function testDB1_CertainResultSetMethodsShouldAlwaysBeSafe() {
  function invokeResultSetMethods(rs) {
    // these methods should not throw for _any_ ResultSet
    var numFields = rs.fieldCount();
    var isValid = rs.isValidRow();
    rs.close();
  }

  // Even if statement doesn't return results, should return a valid ResultSet.
  var TEMP_TABLE = 'Temporary1';
  var statements = [
    'drop table if exists ' + TEMP_TABLE,
    'create table ' + TEMP_TABLE + ' (foo int)',
    'select * from ' + TEMP_TABLE,
    'insert into ' + TEMP_TABLE + ' values (1)',
    'delete from ' + TEMP_TABLE
  ];

  for (var i = 0; i < statements.length; ++i) {
    var rs = g.db.execute(statements[i]);
    invokeResultSetMethods(rs);
  }
  return g.SUCCEEDED;
}

function testDB1_InsertIntRow() {
  var q = "insert into " + g.TESTDB1 + " values (" +
          g.TESTINT + ", '" + g.TESTSTR + "', " +
          g.TESTINT + ", " + g.TESTFLOAT + ", " + g.TESTFLOAT + ")";
  var rs = g.db.execute(q);
  rs.close();
  return g.SUCCEEDED;
}

function testDB1_InsertUndefRow() {
  var q = "insert into " + g.TESTDB1 + " values (" +
          g.TESTINT_UNDEFINED + ", '" + g.TESTSTR_UNDEFINED + "', " +
          g.TESTINT + ", " + g.TESTFLOAT + ", " + g.TESTFLOAT + ")";
  var rs = g.db.execute(q);
  rs.close();
  return g.SUCCEEDED;
}

function testDB1_InsertNullRow() {
  var q = "insert into " + g.TESTDB1 + " values (" +
          g.TESTINT_NULLINROW + ", NULL, " +
          g.TESTINT + ", " + g.TESTFLOAT + ", " + g.TESTFLOAT + ")";
  var rs = g.db.execute(q);
  rs.close();
  return g.SUCCEEDED;
}


// TODO: create a better name for this function; what is the intent?
function testDB1_RowCountThree() {
  var rs = g.db.execute("select count(id) from " + g.TESTDB1);
  var success = (rs.field(0) == 3); // row count three
  rs.close();
  return success;
}

function testDB1_FieldTypesNamesAndPositions() {

  function fieldIsCorrect(rs, index, colName, value, type) {
    // check value by index, value by name, and type
    var success = true;
    success &= (rs.field(index) == value);
    success &= (rs.field(index) == rs.fieldByName(colName));
    success &= (rs.fieldName(index) == colName);
    success &= (typeof rs.field(index) == type);
    return success;
  }

  var rs = g.db.execute("select * from " + g.TESTDB1);
  var success = true;
  success &= rs.isValidRow(); // get at least one row back
  success &= fieldIsCorrect(rs, 0, "id", g.TESTINT, "number");
  success &= fieldIsCorrect(rs, 1, "myvarchar", g.TESTSTR, "string");
  success &= fieldIsCorrect(rs, 2, "myint", g.TESTINT, "number");
  success &= fieldIsCorrect(rs, 3, "myfloat", g.TESTFLOAT, "number");
  success &= fieldIsCorrect(rs, 4, "mydouble", g.TESTFLOAT, "number");
  rs.close();
  return success;
}

function testDB1_UndefinedQueryParamThatFails() {
  var rs = g.db.execute("select * from " + g.TESTDB1 + " where id=?",
                        [undefined]);
  var success = !rs.isValidRow();
  rs.close();
  return success;
}

function testDB1_UndefinedQueryParamThatWorks() {
  var rs = g.db.execute("select * from " + g.TESTDB1 + " where myvarchar=?",
                        [undefined]);
  var success = (rs.field(0) == g.TESTINT_UNDEFINED);
  rs.close();
  return success;
}

function testDB1_NullTextInQuery() {
  var rs = g.db.execute("select myvarchar from " + g.TESTDB1 + " where id=" +
                        g.TESTINT_NULLINROW);
  var success = (rs.field(0) == null);
  rs.close();
  return success;
}

function testDB1_ArgsRequired() {
  try {
    var rs = g.db.execute("select myvarchar from " + g.TESTDB1 + " where id=?");
    rs.close();
  } catch (e) {
    return g.SUCCEEDED;
  }
  return g.FAILED;
}

function testDB1_ArgsForbidden() {
  try {
    var rs = g.db.execute("select * from " + g.TESTDB1, [42]);
    rs.close();
  } catch (e) {
    return g.SUCCEEDED;
  }
  return g.FAILED;
}

function testDB1_ArgsMustBeArray() {
  try {
    // note missing array brackets []
    var rs = g.db.execute("select * from " + g.TESTDB1 + " where id = ?", 42);
    rs.close();
  } catch (e) {
    return g.SUCCEEDED;
  }
  return g.FAILED;
}

function testDB1_EmptyArgsOK() {
  // this is OK - we're not actually passing any args
  var rs = g.db.execute("select * from " + g.TESTDB1, []);
  rs.close();
  return g.SUCCEEDED;
}

function testDB1_UndefinedArgumentsArray() {
  // If we pass <undefined> for the arguments array, that should be the same
  // as not passing it at all.
  var rs = g.db.execute("select * from " + g.TESTDB1, undefined);
  rs.close();
  return g.SUCCEEDED;
}

function testDB1_NullArgumentsArray() {
  // If we pass <null> for the arguments array, that should be the same
  // as not passing it at all.
  var rs = g.db.execute("select * from " + g.TESTDB1, null);
  rs.close();
  return g.SUCCEEDED;
}

function testDB1_WrongNumberOfArgs() {
  try {
    var rs = g.db.execute(
        "select * from " + g.TESTDB1 + " where id=? and myvarchar=?", [42]);
    rs.close();
  } catch (e) {
    return g.SUCCEEDED;
  }
  return g.FAILED;
}

function testDB1_MultipleQueryParams() {

  function fieldIsCorrect(rs, index, colName, value, type) {
    // check value by index, value by name, and type
    var success = true;
    success &= (rs.field(index) == value);
    success &= (rs.field(index) == rs.fieldByName(colName));
    success &= (typeof rs.field(index) == type);
    return success;
  }

  var rs = g.db.execute("select * from " + g.TESTDB1 +
                        " where id=? and myvarchar=?",
                        [g.TESTINT_UNDEFINED, g.TESTSTR_UNDEFINED]);

  var success = true;
  success &= rs.isValidRow();
  success &= fieldIsCorrect(rs, 0, "id", g.TESTINT_UNDEFINED, "number");
  success &= fieldIsCorrect(rs, 1, "myvarchar", g.TESTSTR_UNDEFINED, "string");
  rs.close();

  return success;
}

// test fix for bug #180262
function testDB1_IntegerRanges() {

  function testIntegerValue(value) {
    g.db.execute("delete from RangeTestTable");
    g.db.execute("insert into RangeTestTable values (" + value + ")");
    var rs = g.db.execute("select * from RangeTestTable");
    var out = rs.field(0);
    rs.close();
    // alert(value + " " + typeof value + "\n" +
    //       out + " " + typeof out  + "\n" +
    //       (value == out));
    return (value == out) && (typeof value == typeof out);
  }

  g.db.execute("drop table if exists RangeTestTable");
  g.db.execute("create table RangeTestTable (i int)");

  var success = true;
  success &= testIntegerValue(0);
  success &= testIntegerValue(-2147483648);  // int32.min
  success &= testIntegerValue(2147483647);   // int32.max
  success &= testIntegerValue(0xffffffff);   // uint32.max;
  success &= testIntegerValue(9007199254740989);   // large positive int64
  success &= testIntegerValue(-9007199254740989);  // large negative int64
  // We dont explicitly test min/max possible int64 values because javascript
  // seems to represent those values as floats rather than ints and enough
  // precision is lost to mess up our test.

  return success;
}

function testDB1_NullArguments() {
  function checkValue(pre, post) {
    if ((pre != post) || (typeof pre != typeof post)) {
      success = false;
      //alert(pre + " (" + typeof pre + ") != " +
      //      post + " (" + typeof post + ")");
    }
  }

  var v1inserted = 0;
  var v2inserted = "null";
  var v3inserted = null;
  g.db.execute("drop table if exists NullArgsTable");
  g.db.execute("create table NullArgsTable (v1 int, v2 text, v3)");
  g.db.execute("insert into NullArgsTable values (?, ?, ?)",
               [v1inserted, v2inserted, v3inserted]);
  var rs = g.db.execute("select * from NullArgsTable");
  var v1 = rs.field(0);
  var v2 = rs.field(1);
  var v3 = rs.field(2);
  rs.close();
  var success = true;
  checkValue(v1inserted, v1);
  checkValue(v2inserted, v2);
  checkValue(v3inserted, v3);
  return success;
}

// This caught a crashing bug where JsContext wasn't set in a worker
// thread, when object1 called object2 which called JsSetException.
// In this case, object1 is a Database and object2 is a ResultSet, and
// the exception is thrown due to an error in the first call to
// sqlite3_step(), as opposed to sqlite3_prepare_v2().
function testDB1_ValidStatementError() {
  try {
    g.db.execute('rollback').close();
  } catch (e) {
    // good, we expected an error
    return g.SUCCEEDED;
  }
  return g.FAILED;
}

function testDB1_PragmaSetDisabled() {
  try {
    g.db.execute('PRAGMA cache_size = 1999').close();
  } catch (e) {
    // good, we expected an error
    return g.SUCCEEDED;
  }
  return g.FAILED;
}

function testDB1_PragmaGetDisabled() {
  try {
    g.db.execute('PRAGMA cache_size').close();
  } catch (e) {
    // good, we expected an error
    return g.SUCCEEDED;
  }
  return g.FAILED;
}

// On IE, the specialEmpty value below will be a NULL BSTR.  This is
// supposed to be treated the same as the empty string, but some code
// dereferenced it.
function testDB1_SpecialEmptyString() {
  if (typeof document != 'undefined') {
    var specialEmpty = document.getElementById("empty").value;

    try {
      // NULL-dereference crash.
      g.db.execute(specialEmpty).close();
      return g.FAILED;
    } catch(e) {
      // good, we expected an error
    }

    // Sanity check for same bug in bind parameters (there wasn't
    // one).
    g.db.execute('select ?', [specialEmpty]).close();

    var rs = g.db.execute('select ? as field', ['value']);
    try {
      // NULL-dereference crash.
      rs.fieldByName(specialEmpty);
      return g.FAILED;
    } catch(e) {
      // good, we expected an error
    } finally {
      rs.close();
    }
  }
  return g.SUCCEEDED;
}

// Causes a segmentation fault.
function testDB1_StackOverflow() {
  var q = "select * from (select 1";
  for (var i = 0; i < 50000; ++i) {
    q += " union select 1";
  }
  q += ")";
  try {
    g.db.execute(q).close();
  } catch(e) {
    // good, we expected an error
    return g.SUCCEEDED;
  }
  return g.FAILED;
}

// Using VACUUM would cause corruption of fts2 tables.  We have
// auto_vacuum turned on, so VACUUM isn't really necessary.
function testDB1_VacuumDisabled() {
  try {
    g.db.execute('VACUUM').close();
  } catch(e) {
    // good, we expected an error
    return g.SUCCEEDED;
  }
  return g.FAILED;
}

function testDB1_CloseDatabase() {
  g.db.close();
  return g.SUCCEEDED;
}

function testDB1_CloseDatabaseTwice() {
  g.db.close();
  g.db.close();
  return g.SUCCEEDED;
}

function testDB1_ExecuteMsecOnlyInDebug() {
  // property should exist iff this is a debug build
  if (g.db.executeMsec) {
    return gIsDebugBuild ? g.SUCCEEDED : g.FAILED;
  } // else
  return gIsDebugBuild ? g.FAILED : g.SUCCEEDED;
}


// Test that the optional database name works correctly for all variations.
// <undefined>, <null>, and not passing any arguments should all mean the
// same thing, on all APIs throughout Google Gears.
function test_OptionalDatabaseName() {
  g.db.close();

  // pass '' for the database name
  g.db.open('');
  // insert a value
  g.db.execute("drop table if exists TempTable");
  g.db.execute("create table TempTable (int i, t text)");
  g.db.execute("insert into TempTable" +
               " values (1, 'OptionalDatabaseName')");
  g.db.close();
  
  var testDb = function(testName) {
    // expect to find value inserted above
    var rs = g.db.execute("select * from TempTable");
    var success = rs.isValidRow();

    rs.close();
    g.db.close();
    
    if (!success) {
      throw new Error("Not valid row: " + testName);
    }    
  }

  // pass null for the database name
  g.db.open(null);
  testDb("null");
  
  // pass undefined for the database name
  g.db.open(undefined);
  testDb("undefined");
  
  // don't pass anything for the database name
  g.db.open();
  testDb("argument not passed");

  return true;
}

function test_InvalidDatabaseNameType() {
  g.db.close();

  var tryOpen = function(notAString) {
    try {
      g.db.open(notAString);
    } catch (e) {
      // good, we expected an error
      return true;
    }
    
    g.db.close();
    throw new Error("Incorrectly opened database: " + notAString);
  }  
  
  tryOpen(42);
  tryOpen(true);
  tryOpen(new Date());
  tryOpen([]);
  
  return true;
}

function test_SemicolonSeparatedStatementsShouldNotWork() {
  // this test simulates one large class of SQL injection attacks
  g.db.open();
  g.db.execute('drop table if exists TempTable');
  g.db.execute('create table TempTable' +
               ' (Username text, Email text, Password text)');
  g.db.execute('insert into TempTable values ("joe", "joe@a.com", "abcdef")');

  // simulate multi-part query attack
  var goodStmt = 'select Email from TempTable where Username="joe"';
  var evilStmt = 'update TempTable set Password="" where Username="joe"';
  var rs = g.db.execute(goodStmt + ' ; ' + evilStmt);
  rs.close();

  // see if the attack succeeded
  rs = g.db.execute('select * from TempTable where Username="joe"');
  var password = rs.fieldByName('Password');
  rs.close();
  g.db.close();
  
  if (password != "abcdef") {
    return g.FAILED;
  }

  return g.SUCCEEDED;
}

function test_AttachShouldNotWork() {
  var rs;

  try {
    // TODO(shess) Find portable solution which won't put
    // gears_test.db in your root if this succeeds.
    rs = g.db.execute('ATTACH DATABASE ? AS attached_db',
                      ["/gears_test.db"]);
  } catch(e) {
    // TODO(shess) Can we make any assertions about _how_ it failed?
    return g.SUCCEEDED;
  }

  rs.close();

  rs = g.db.execute('DETACH DATABASE attached_db');
  rs.close();

  return g.FAILED;
}

function test_AttachFnShouldNotWork() {
  var rs;

  try {
    // TODO(shess) Find portable solution which won't put
    // gears_test.db in your root if this succeeds.
    rs = g.db.execute('SELECT sqlite_attach(?, ?, NULL)',
                      ["/gears_test.db", "attached_db"]);
  } catch(e) {
    // TODO(shess) Can we make any assertions about _how_ it failed?
    return g.SUCCEEDED;
  }

  rs.close();

  rs = g.db.execute('SELECT sqlite_detach(?)',
                    ["attached_db"]);
  rs.close();

  return g.FAILED;
}

function test_OpenDatabaseWithIds() {

  function setupDB(db) {
    var rs = db.execute('drop table if exists xxx');
    rs = db.execute('create table xxx (id int auto_increment primary key, ' +
                    'int mydata)');
    rs.close();
  }

  var rs;

  var db1 = google.gears.factory.create('beta.database', '1.0');
  db1.open('1');
  setupDB(db1);
  rs = db1.execute('insert into xxx values (1, 1)');
  rs.close();
  db1.close();

  var db2 = google.gears.factory.create('beta.database', '1.0');
  db2.open('2');
  setupDB(db2);
  rs = db2.execute('insert into xxx values (2, 2)');
  rs.close();
  db2.close();

  var db3 = google.gears.factory.create('beta.database', '1.0');
  db3.open('3');
  setupDB(db3);
  rs = db3.execute('insert into xxx values (3, 3)');
  rs.close();
  db3.close();

  db1.open('1');
  rs = db1.execute('select * from xxx where id=1');
  if (rs.isValidRow()) {
    if (rs.field(1) != 1) {
      throw new Error('DB with no identifier has row with bad ID (not 1)');
    }
  } else {
    throw new Error('DB with no identifier is missing row.');
  }
  rs.close();
  db1.close();

  db2.open('2');
  rs = db2.execute('select * from xxx where id=2');
  if (rs.isValidRow()) {
    if (rs.field(1) != 2) {
      throw new Error('DB with second identifier has row with bad ID (not 2)');
    }
  } else {
    throw new Error('DB with second identifier is missing row.');
  }
  rs.close();
  db2.close();

  db3.open('3');
  rs = db3.execute('select * from xxx where id=3');
  if (rs.isValidRow()) {
    if (rs.field(1) != 3) {
      throw new Error('DB with third identifier has row with bad ID (not 3)');
    }
  } else {
    throw new Error('DB with third identifier is missing row.');
  }
  rs.close();
  db3.close();

  return g.SUCCEEDED;
}

function test_OpenDatabaseWithIllegalIds() {

  function doIllegalIdTest(id) {
    var temp_db =
        google.gears.factory.create('beta.database', '1.0');
    try {
      temp_db.open(id);
    } catch (e) {
      return; // good, we want an exception
              // Note: no retval from helper function.
    }
    temp_db.close();
    throw new Error('Should not have been able to open DB with ID ' + id);
  }

  doIllegalIdTest('/');
  doIllegalIdTest('\\');
  doIllegalIdTest(':');
  doIllegalIdTest('*');
  doIllegalIdTest('?');
  doIllegalIdTest('"');
  doIllegalIdTest('<');
  doIllegalIdTest('>');
  doIllegalIdTest('|');
  doIllegalIdTest(';');
  doIllegalIdTest(',');
  doIllegalIdTest('.a_perfectly_valid_id_except_for_leading_dot');
  doIllegalIdTest('a_perfectly_valid_id_except_for_trailing_dot.');
  doIllegalIdTest('an id with spaces');
  doIllegalIdTest('an_id_with_an_eight_bit_char�cter');
  return g.SUCCEEDED;
}

function test_SimultaneousDatabaseOpens() {
  function useDB(db) {
    var rs = db.execute('drop table if exists TestTable');
    rs = db.execute('create table TestTable (i int)');
    rs.close();
  }

  var db1 = google.gears.factory.create('beta.database', '1.0');
  db1.open('foo');
  useDB(db1);

  var db2 = google.gears.factory.create('beta.database', '1.0');
  db2.open('bar');
  useDB(db2);

  db2.close();
  db1.close();

  return g.SUCCEEDED;
}

function test_FulltextIndexing_FTS1() {
  var rs;
  g.db = google.gears.factory.create('beta.database', '1.0');
  g.db.open();

  rs = g.db.execute('drop table if exists foo');
  rs.close();
  rs = g.db.execute("create virtual table foo using fts1(content)");
  rs.close();
  rs = g.db.execute("insert into foo (content) values " +
                    "('to sleep perchance to dream')");
  rs.close();
  rs = g.db.execute("insert into foo (content) values ('to be or not to be')");
  rs.close();
  rs = g.db.execute("select * from foo where content match 'dream'");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'to sleep perchance to dream') {
      throw new Error('Fulltext match statement failed');
    }
  } else {
    throw new Error('Fulltext match statement returned no results');
  }
  rs.close();

  g.db.close();

  return g.SUCCEEDED;
}

function test_FulltextIndexingByRowId_FTS1() {
  var rs;
  g.db = google.gears.factory.create('beta.database', '1.0');
  g.db.open();

  rs = g.db.execute("insert into foo(rowid, content) values(12345, 'hello')");
  rs.close();
  rs = g.db.execute("select content from foo where rowid=12345");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'hello') {
      throw new Error('select by rowid failed');
    }
  } else {
    throw new Error('select by rowid returned no results');
  }
  rs.close();

  rs = g.db.execute("select content from foo where content match 'hello'");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'hello') {
      throw new Error('match rowid-inserted value failed');
    }
  } else {
    throw new Error('match rowid-inserted value returned no results');
  }
  rs.close();

  rs = g.db.execute("select content from foo where rowid=12345");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'hello') {
      throw new Error('select by rowid failed');
    }
  } else {
    throw new Error('select by rowid returned no results');
  }
  rs.close();

  g.db.close();

  return g.SUCCEEDED;
}

function test_FulltextIndexingRowIdChanges_FTS1() {
  // This test is known not to work in the current version of Cinnamon.
  // Simply put, updates don't work.
  //
  // TODO(miket): remove following line when Cinnamon can handle updates.
  return g.SUCCEEDED;

  var rs;
  g.db = google.gears.factory.create('beta.database', '1.0');
  g.db.open();

  rs = g.db.execute("update foo set rowid=23456 where rowid=12345");
  rs.close();

  rs = g.db.execute("select content from foo where rowid=12345");
  if (rs.isValidRow()) {
    throw "rowid update didn't take";
  }
  rs.close();

  rs = g.db.execute("select content from foo where rowid=23456");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'hello') {
      throw new Error('select by modified rowid failed');
    }
  } else {
    throw new Error('select by modified rowid returned no results');
  }
  rs.close();

  rs = g.db.execute("select rowid from foo where content match 'hello'");
  if (rs.isValidRow()) {
    if (rs.field(0) != 23456) {
      throw new Error('select modified rowid by match failed');
    }
  } else {
    throw new Error('select modified rowid by match returned no results');
  }
  rs.close();

  g.db.close();

  return g.SUCCEEDED;
}

function test_FulltextIndexing_FTS2() {
  var rs;
  g.db = google.gears.factory.create('beta.database', '1.0');
  g.db.open();

  rs = g.db.execute('drop table if exists foo');
  rs.close();
  rs = g.db.execute("create virtual table foo using fts2(content)");
  rs.close();
  rs = g.db.execute("insert into foo (content) values " +
                    "('to sleep perchance to dream')");
  rs.close();
  rs = g.db.execute("insert into foo (content) values ('to be or not to be')");
  rs.close();
  rs = g.db.execute("select * from foo where content match 'dream'");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'to sleep perchance to dream') {
      throw new Error('Fulltext match statement failed');
    }
  } else {
    throw new Error('Fulltext match statement returned no results');
  }
  rs.close();

  g.db.close();

  return g.SUCCEEDED;
}

function test_FulltextIndexingByRowId_FTS2() {
  var rs;
  g.db = google.gears.factory.create('beta.database', '1.0');
  g.db.open();

  rs = g.db.execute("insert into foo(rowid, content) values(12345, 'hello')");
  rs.close();
  rs = g.db.execute("select content from foo where rowid=12345");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'hello') {
      throw new Error('select by rowid failed');
    }
  } else {
    throw new Error('select by rowid returned no results');
  }
  rs.close();

  rs = g.db.execute("select content from foo where content match 'hello'");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'hello') {
      throw new Error('match rowid-inserted value failed');
    }
  } else {
    throw new Error('match rowid-inserted value returned no results');
  }
  rs.close();

  rs = g.db.execute("select content from foo where rowid=12345");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'hello') {
      throw new Error('select by rowid failed');
    }
  } else {
    throw new Error('select by rowid returned no results');
  }
  rs.close();

  g.db.close();

  return g.SUCCEEDED;
}

// fts2 was throwing assertions when inserting with rowid<=0.
function test_FulltextIndexingRowidZero_FTS2() {
  var rs;
  g.db = google.gears.factory.create('beta.database', '1.0');
  g.db.open();

  rs = g.db.execute("insert into foo(rowid, content) values(0, 'bother')");
  rs.close();
  rs = g.db.execute("select content from foo where rowid=0");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'bother') {
      throw new Error('select by rowid failed');
    }
  } else {
    throw new Error('select by rowid returned no results');
  }
  rs.close();

  rs = g.db.execute("insert into foo(rowid, content) values(-1, 'bother')");
  rs.close();
  rs = g.db.execute("select content from foo where rowid=-1");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'bother') {
      throw new Error('select by rowid failed');
    }
  } else {
    throw new Error('select by rowid returned no results');
  }
  rs.close();

  rs = g.db.execute("select rowid from foo where content match 'bother' "+
                    "order by rowid");
  if (rs.isValidRow()) {
    if (rs.field(0) != -1) {
      throw new Error('match rowid-inserted value failed');
    }
    rs.next();
    if (rs.isValidRow()) {
      if (rs.field(0) != 0) {
        throw new Error('match rowid-inserted value failed');
      }
      rs.next();
    } else {
      throw new Error('match rowid-inserted value returned no results');
    }
  } else {
    throw new Error('match rowid-inserted value returned no results');
  }
  rs.close();

  g.db.close();

  return g.SUCCEEDED;
}

function test_FulltextIndexingRowIdChanges_FTS2() {
  // This test is known not to work in the current version of Cinnamon.
  // Simply put, updates don't work.
  //
  // TODO(miket): remove following line when Cinnamon can handle updates.
  return g.SUCCEEDED;

  var rs;
  g.db = google.gears.factory.create('beta.database', '1.0');
  g.db.open();

  rs = g.db.execute("update foo set rowid=23456 where rowid=12345");
  rs.close();

  rs = g.db.execute("select content from foo where rowid=12345");
  if (rs.isValidRow()) {
    throw "rowid update didn't take";
  }
  rs.close();

  rs = g.db.execute("select content from foo where rowid=23456");
  if (rs.isValidRow()) {
    if (rs.field(0) != 'hello') {
      throw new Error('select by modified rowid failed');
    }
  } else {
    throw new Error('select by modified rowid returned no results');
  }
  rs.close();

  rs = g.db.execute("select rowid from foo where content match 'hello'");
  if (rs.isValidRow()) {
    if (rs.field(0) != 23456) {
      throw new Error('select modified rowid by match failed');
    }
  } else {
    throw new Error('select modified rowid by match returned no results');
  }
  rs.close();

  g.db.close();

  return g.SUCCEEDED;
}

function test_LastInsertRowId() {
  var rs;
  g.db = google.gears.factory.create('beta.database', '1.0');
  g.db.open();

  rs = g.db.execute('drop table if exists lastInsertRowId');
  rs.close();
  rs = g.db.execute("create table lastInsertRowId (" +
                    "id int auto_increment primary key, " +
                    "myint int)");
  rs.close();
  for (var i = 1; i < 10; ++i) {
    rs = g.db.execute("insert into lastInsertRowId (id, myint) values " +
                      "(" + i + ", " + i * 100 + ")");
    rs.close();
    var lii = g.db.lastInsertRowId;
    if (lii != i) {
      throw "Didn't find expected last insert rowid";
    }
  }
  g.db.close();

  return g.SUCCEEDED;
}

//
// Array of all tests in this category
//

var databaseTests = [
  initDB1,
  testDB1_CreateTable,
  testDB1_Insert,
  testDB1_DeleteAll,
  testDB1_RowCountIsZero,
  testDB1_FieldCountIsFive,
  testDB1_IsValidRowWhenNoRowsReturned,
  testDB1_CertainResultSetMethodsShouldAlwaysBeSafe,
  testDB1_InsertIntRow,
  testDB1_InsertUndefRow,
  testDB1_InsertNullRow,
  testDB1_RowCountThree,
  testDB1_FieldTypesNamesAndPositions,
  testDB1_UndefinedQueryParamThatFails,
  testDB1_UndefinedQueryParamThatWorks,
  testDB1_NullTextInQuery,
  testDB1_ArgsRequired,
  testDB1_ArgsForbidden,
  testDB1_EmptyArgsOK,
  testDB1_UndefinedArgumentsArray,
  testDB1_NullArgumentsArray,
  testDB1_ArgsMustBeArray,
  testDB1_WrongNumberOfArgs,
  testDB1_MultipleQueryParams,
  testDB1_IntegerRanges,
  testDB1_NullArguments,
  testDB1_ValidStatementError,
  testDB1_PragmaSetDisabled,
  testDB1_PragmaGetDisabled,
  testDB1_SpecialEmptyString,
  // TODO(shess) Fix things so that we can run this, again.  See bug
  // 199.
  //testDB1_StackOverflow,
  testDB1_VacuumDisabled,
  testDB1_CloseDatabase,
  testDB1_CloseDatabaseTwice,
  testDB1_ExecuteMsecOnlyInDebug,
  test_OptionalDatabaseName,
  test_InvalidDatabaseNameType,
  
  test_SemicolonSeparatedStatementsShouldNotWork,

  test_AttachShouldNotWork,
  test_AttachFnShouldNotWork,

  test_OpenDatabaseWithIds,
  test_OpenDatabaseWithIllegalIds,

  test_SimultaneousDatabaseOpens,

  test_FulltextIndexing_FTS1,
  test_FulltextIndexingByRowId_FTS1,
  test_FulltextIndexingRowIdChanges_FTS1,
  test_FulltextIndexing_FTS2,
  test_FulltextIndexingByRowId_FTS2,
  test_FulltextIndexingRowIdChanges_FTS2,
  test_FulltextIndexingRowidZero_FTS2,

  test_LastInsertRowId
];

function runDatabaseTests() {
  runArrayOfTests(databaseTests);
}

//------------------------------------------------------------------------------
// Misc tests
//------------------------------------------------------------------------------

function test_DisallowDirectObjectCreation() {
  // JS code should only be able to instantiate the GearsFactory object;
  // everything else should go through GearsFactory.
  var objects = [
    'GearsDatabase',
    'GearsResultSet',
    'GearsWorkerPool',
    'GearsLocalServer',
    'GearsResourceStore' // testing one indirect localserver interface is enough
  ];

  for (var i = 0; i < objects.length; ++i) {

    // test 'new FOO()'
    //  and 'new ActiveXObject("Gears.FOO")'
    var prefix_suffix = [
      ['new ',                       '();'],
      ['new ActiveXObject("Gears.',  '");']
    ];

    for (var j = 0; j < prefix_suffix.length; ++j) {

      var succeeded = false;
      try {
        var toEval = prefix_suffix[j][0] + objects[i] + prefix_suffix[j][1];
        var instance = eval(toEval);
      } catch (e) {
        succeeded = true;  // good, we want an exception
      }

      if (!succeeded) {
        return g.FAILED;
      }
    }
  }

  return g.SUCCEEDED;
}

function test_FactoryCreate() {
  // Make sure older versions of objects can be created.
  // TODO(ace): Think about how to keep this test from becoming stale when
  // the Gears object version numbers are incremented.
  google.gears.factory.create('beta.workerpool', '1.0');

  // Test the create() factory method with various strange inputs.
  try {
    google.gears.factory.create('strange', '1.0');
    return g.FAILED;
  } catch (e) {
    // Good, we want an exception.
  }

  try {
    google.gears.factory.create('beta.strange', '1.0');
    return g.FAILED;
  } catch (e) {
    // Good, we want an exception.
  }

  try {
    google.gears.factory.create('beta.workerpool', '9.9');
    return g.FAILED;
  } catch (e) {
    // Good, we want an exception.
  }

  if (typeof document != 'undefined') {
    // On IE, the specialEmpty value below will be a NULL BSTR.  This
    // is supposed to be treated the same as the empty string, but
    // some code dereferenced it.
    var specialEmpty = document.getElementById("empty").value;

    try {
      // Crash parsing NULL version number.
      google.gears.factory.create('beta.workerpool', specialEmpty);
      return g.FAILED;
    } catch(e) {
      // Good, we want an exception.
    }

    try {
      // Crash creating a string from contents of NULL.
      google.gears.factory.create(specialEmpty, '1.0');
      return g.FAILED;
    } catch(e) {
      // Good, we want an exception.
    }
  }

  return g.SUCCEEDED;
}


function test_WorkerSecurityContext() {
  if (!gIsChildWorker) {
    return g.SUCCEEDED;
  }

  var not_allowed;

  if (typeof ActiveXObject != 'undefined') {
    // We disable *all* activexobjects in workers. This eliminates many messy
    // questions that would come up around prompting for permission from a
    // background thread.
    not_allowed = [
      // not safe for scripting
      'new ActiveXObject("Scripting.FileSystemObject")',
      // safe for scripting, but also not allowed on workers
      'new ActiveXObject("Scripting.Dictionary")',
      // commonly used on main thread, not allowed on workers
      'new ActiveXObject("Msxml2.XMLHTTP")'
      ];
  } else {
    not_allowed = [
    // "Components" can lead to all kinds of nasty things in mozilla. Since we
    // are hosting spidermonkey directly, our js does not have access to it.
    'Components',
    // Similarly, other objects which are available to web page js should not
    // be available.
    'new XMLHttpRequest()'
    ];
  }

  for (var i = 0; i < not_allowed.length; i++) {
    try {
      eval(not_allowed[i]);
    } catch (e) {
      continue; // good, we want an exception
    }

    throw new Error('Worker incorrectly allowed: ' + not_allowed[i]);
  }
  
  return g.SUCCEEDED;
}

//
// Array of all tests in this category
//

var miscTests = [
  test_DisallowDirectObjectCreation,
  test_FactoryCreate,
  test_WorkerSecurityContext
];

function runMiscTests() {
  runArrayOfTests(miscTests);
}

//------------------------------------------------------------------------------
// Test runner: executes each test in the parent worker and a child worker.
//------------------------------------------------------------------------------

function runArrayOfTests(tests) {

  function runOneTest(testBody) {
    var stringToEval = '{ var nextTest = ' + testBody + '; nextTest(); }';
    var success = g.FAILED;
    var reason = '';
    var startTime = new Date().getTime();
    try {
      success = eval(stringToEval);
    } catch (e) {
      reason += e.message;
    }
    var endTime = new Date().getTime();
    var execTime = endTime - startTime;

    return [testBody, // pass as function to avoid string-quoting problems
            success,
            '\'' + reason.replace(/'/g, '\\\'') + '\'',
            execTime];
  }


  //
  // Run all tests on the parent worker.
  //

  for (var testIndex = 0; testIndex < tests.length; ++testIndex) {
    var testBody = String(tests[testIndex]);
    var retvals = runOneTest(testBody);
    // do String() and eval() to match what happens in child worker
    var msg = '[' + String(retvals) + ']';
    retvals = eval('(' + msg + ')');
    insertRow(retvals[0], retvals[1], retvals[2], retvals[3]);
  }


  //
  // Then run all tests on a child worker.
  //

  function child_init() {
    google.gears.workerPool.onmessage = onmessageHandler;
  }

  function child_onmessage(text, sender, m) {
    // runs any code received (should always be a single test function)
    var retvals = runOneTest(m.text);
    google.gears.workerPool.sendMessage('[' + String(retvals) + ']', sender);
  }

  function parent_onmessage(text, sender, m) {
    // calls insertRow() with the values returned by the child worker
    var retvals = eval('(' + m.text + ')');
    insertRow('<b>[WORKER THREAD]</b> ' + retvals[0], // testBody
              retvals[1],  // success
              retvals[2],  // reason
              retvals[3]); // execTime
  }

  var childCode = 'var gIsDebugBuild = ' + gIsDebugBuild + ';' +
                  'var gIsChildWorker = 1; ' +
                  'var testGlobals = \'' + testGlobals + '\';' +
                  'var g = eval("(" + testGlobals + ")");' +
                  runOneTest +
                  'var onmessageHandler = ' + child_onmessage + ';' +
                  'var runInit = ' + child_init + ';' +
                  'runInit();';

  var workerPool =
      google.gears.factory.create('beta.workerpool', '1.1');
  workerPool.onmessage = parent_onmessage;
  try {
    var childId = workerPool.createWorker(childCode);
    for (var testIndex = 0; testIndex < tests.length; ++testIndex) {
      var text = String(tests[testIndex]);
      workerPool.sendMessage(text, childId);
    }
  } catch (e) {
    insertRow('While creating worker in runArrayOfTests...', // testBody
              false,  // success
              e.message,  // reason
              0); // execTime
  }

}


//------------------------------------------------------------------------------
// WorkerPool tests
//------------------------------------------------------------------------------

var NUM_WORKERS = 10;  // set >= 10 to break Firefox (2007/02/22)
var workerIndexToWorkerId = [];
var nextValueToRecvFromWorkerId = []; // [worker] -> value
var NUM_REQUEST_LOOPS = 5;            // set workers * requests * responses
var NUM_RESPONSES_PER_REQUEST = 100;  // >= 10000 to break IE (2007/02/22)
var EXPECTED_RESPONSES_PER_WORKER = NUM_REQUEST_LOOPS * NUM_RESPONSES_PER_REQUEST;

var workersCreateBeginTime;
var workersCreateEndTime;
var workersSendRecvBeginTime;
var workersSendRecvEndTime;

// TODO(cprince): use the following to test performance of large messages
//var LONG_STRING_PADDING = 'a';
//for (var i = 0; i < 10; ++i) {  // generates a 2^N character string
//  LONG_STRING_PADDING = LONG_STRING_PADDING + LONG_STRING_PADDING;
//}


function workerpool_SynchronizationStressTest() {

  // Parent/child worker code.  Each child is programmed to respond with the
  // the number of messages requested by the parent.  The parent is programmed
  // to tally each response, and to look for dropped messages.

  function parent_onmessage(text, sender, m) {
    // Track cumulative per-worker message count.
    // Detect out-of-order responses.
    var retval = parseInt(m.text);
    var expected = nextValueToRecvFromWorkerId[sender];
    if (retval != expected) {
      insertRow('SynchronizationStressTest()',
                false,  // success
                'Out-of-order value ' + retval,  // reason
                0); // execTime
    }
    nextValueToRecvFromWorkerId[sender] = retval + 1;

    if (0 == (nextValueToRecvFromWorkerId[sender] % 
              EXPECTED_RESPONSES_PER_WORKER)) {
      // will update this once per worker, which is fine
      workersSendRecvEndTime = new Date().getTime();
    }
  }

  function child_init() {
    g.messageTotal = 0;
    google.gears.workerPool.onmessage = onmessageHandler;
  }

  function child_onmessage(text, sender, m) {
    var numResponses = parseInt(m.text);
    for (var i = 0; i < numResponses; ++i) {
      google.gears.workerPool.sendMessage(String(gFirstResponse + g.messageTotal),
                                          sender);
      g.messageTotal += 1;
    }
  }

  var childCode = 'var gIsDebugBuild = ' + gIsDebugBuild + ';' +
                  'var gIsChildWorker = 1;' +
                  'var testGlobals = \'' + testGlobals + '\';' +
                  'var g = eval("(" + testGlobals + ")");' +
                  'var onmessageHandler = ' + child_onmessage + ';' +
                  'var runInit = ' + child_init + ';' +
                  'runInit();';

  var workerPool =
      google.gears.factory.create('beta.workerpool', '1.1');
  workerPool.onmessage = parent_onmessage;


  // Create all workers.

  workersCreateBeginTime = new Date().getTime();
  for (var t = 0; t < NUM_WORKERS; ++t) {
    try {
      var firstResponse = t * EXPECTED_RESPONSES_PER_WORKER;
      // test with different code in every worker
      var thisChildCode = 'var gFirstResponse = ' + firstResponse + ';' +
                          childCode;
      var childId = workerPool.createWorker(thisChildCode);
      workerIndexToWorkerId[t] = childId;
      nextValueToRecvFromWorkerId[childId] = firstResponse;
    } catch (e) {
      insertRow('While creating worker in SynchronizationStressTest()...',
                false,  // success
                e.message,  // reason
                0); // execTime
    }
  }
  workersCreateEndTime = new Date().getTime();


  // Loop over the workers several times, asking them to return some number
  // of messages each time. This floods the message path into the parent worker.

  workersSendRecvBeginTime = new Date().getTime();
  for (var i = 0; i < NUM_REQUEST_LOOPS; ++i) {
    for (var t = 0; t < NUM_WORKERS; ++t) {
      var workerId = workerIndexToWorkerId[t];
      workerPool.sendMessage(String(NUM_RESPONSES_PER_REQUEST) + ',' +
                             String(t * EXPECTED_RESPONSES_PER_WORKER),
                             workerId);
    }
  }
}


var pingReceived_GCWithFunctionClosures = 0;

function workerpool_GCWithFunctionClosures() {

  function childThread() {

    google.gears.workerPool.onmessage = function(text, sender, m) {
      if (m.text == 'ping') {
        google.gears.workerPool.sendMessage('pong', m.sender);
      }
    }

    if (gIsDebugBuild) { google.gears.workerPool.forceGC(); }

  }

  var wp = google.gears.factory.create('beta.workerpool', '1.1');
  wp.onmessage = function(text, sender, m) {
    if (m.text == 'pong') {
      pingReceived_GCWithFunctionClosures = 1;
    }
  }

  var childCode = 'var gIsDebugBuild = ' + gIsDebugBuild + ';' +
                  'var gIsChildWorker = 1;' +
                  'var testGlobals = \'' + testGlobals + '\';' +
                  'var g = eval("(" + testGlobals + ")");' +
                  '[' + childThread + '][0]()';
  var childId = wp.createWorker(childCode);

  if (gIsDebugBuild) { wp.forceGC(); } // only exists in internal builds

  wp.sendMessage('ping', childId);

  if (gIsDebugBuild) { wp.forceGC(); } // only exists in internal builds
}


function workerpool_OnMessageTests() {
  // Test that onmessage Object parameter is correct.
  workerpool_OnMessageTests.message_object_ok = false;
  var wp1 = google.gears.factory.create('beta.workerpool', '1.0');
  wp1.onmessage = function(text, sender, m) {
    workerpool_OnMessageTests.message_object_ok = (text == 'PING1' &&
                                                   m.text == text &&
                                                   m.sender == sender &&
                                                   m.origin != '');
  };
  var childId = wp1.createWorker('var wp = google.gears.workerPool;' +
                                 'wp.onmessage = function(a, b, m) {' +
                                 '  wp.sendMessage(m.text, m.sender);' +
                                 '};');
  wp1.sendMessage('PING1', childId);
  // TODO(ace): Add tests that m.origin is _correct_ for same-origin,
  // cross-origin, and different URL schemes.
}


// Class used to test that the onerror handler bubbles correctly.
function WorkerPoolBubbleTest(name, onerrorContent, shouldBubble) {
  this.fullName = 'WorkerPoolBubbleTest: ' + name;
  this.onerrorContent = onerrorContent;
  this.shouldBubble = shouldBubble;
}

WorkerPoolBubbleTest.prototype.start = function() {
  this.wp = google.gears.factory.create('beta.workerpool', '1.1');
  var str = [];

  if (this.shouldBubble) {
    gExpectedErrors.push(this.fullName);
  }

  if (this.onerrorContent) {
    str.push('google.gears.workerPool.onerror = function() {');
    str.push(this.onerrorContent);
    str.push('}');
  }

  str.push('throw new Error("' + this.fullName + '")');
  this.wp.createWorker(str.join('\n'));
};

WorkerPoolBubbleTest.prototype.checkResult = function() {
  if (this.shouldBubble) {
    if (receivedError(this.fullName)) {
      insertRow(this.fullName, true, '');
    } else {
      insertRow(this.fullName, false, 'Expected global error was not thrown');
    }
  } // else, the error was unexpected and was allowed to bubble up to the
    // browser error UI.
};


workerpool_bubbleTests = [
  new WorkerPoolBubbleTest('no bubble', 'return true;', false),
  new WorkerPoolBubbleTest('bubble 1', 'return false;', true),
  // TODO(aa): When coercion is implemented these next two should *not* bubble
  // because they would get coerced to <true>.
  new WorkerPoolBubbleTest('bubble 2', 'return 42;', true),
  new WorkerPoolBubbleTest('bubble 3', 'return {};', true),
  new WorkerPoolBubbleTest('bubble 4', 'return;', true),
  new WorkerPoolBubbleTest('bubble 5', null, true),
  new WorkerPoolBubbleTest('nested error outer',
                           'throw new Error("nested error inner")', true)
];

function workerpool_OnErrorTests() {
  // Test that onerror gets called.
  workerpool_OnErrorTests.handler_called = false;
  var wp1 = google.gears.factory.create('beta.workerpool', '1.1');
  wp1.onmessage = function() {
    workerpool_OnErrorTests.handler_called = true;
  };
  var childId = wp1.createWorker(
      ['var parentId;',
       'google.gears.workerPool.onmessage = function(m, senderId) {',
       'parentId = senderId;',
       'throw new Error("hello");',
       '}',
       'google.gears.workerPool.onerror = function() {',
       'google.gears.workerPool.sendMessage("", parentId);',
       'return true;',
       '}'].join('\n'));
  wp1.sendMessage('hello', childId);

  // Test errors are bubbled up correctly for different onerror configurations
  for (var i = 0, test; test = workerpool_bubbleTests[i]; i++) {
    test.start();
  }

  // The last test generates one extra error
  gExpectedErrors.push('nested error inner');
}


function workerpool_CreateWorkerFromUrl() {
  var sameOriginWorkerFile = 'unit_tests_worker_same_origin.js';
  var crossOriginWorkerFile = 'unit_tests_worker_cross_origin.js';

  var currentUrl = location.href;
  var sameOriginPath = currentUrl.substring(0, 1 + currentUrl.lastIndexOf('/'));
  var crossOriginPath =
      'http://google-gears.googlecode.com/svn/trunk/gears/test/';


  // TEST 1
  workerpool_CreateWorkerFromUrl.result1 = '';
  workerpool_CreateWorkerFromUrl.description1 =
      'createWorkerFromUrl() Test 1: ' +
      'Same-origin, relative URLs should work.';

  var wp1 = google.gears.factory.create('beta.workerpool', '1.1');
  wp1.onmessage = function(text, sender, m) {
    workerpool_CreateWorkerFromUrl.result1 = m.text;
  }
  var childId = wp1.createWorkerFromUrl(sameOriginWorkerFile);
  wp1.sendMessage('PING1', childId);


  // TEST 2
  workerpool_CreateWorkerFromUrl.result2A = '';
  workerpool_CreateWorkerFromUrl.description2A =
      'createWorkerFromUrl() Test 2A: ' +
      'Same-origin, absolute URLs should work. ';
  workerpool_CreateWorkerFromUrl.result2B = '';
  workerpool_CreateWorkerFromUrl.description2B =
      'createWorkerFromUrl() Test 2B: ' +
      'And worker database SHOULD exist in parent origin. ';

  // Cleanup any local DB before starting test.  
  var db2 = google.gears.factory.create('beta.database', '1.0');
  db2.open('worker_js');
  db2.execute('drop table if exists PING2').close();
  db2.close();

  var wp2 = google.gears.factory.create('beta.workerpool', '1.1');
  wp2.onmessage = function(text, sender, m) {
    // If we reach this line, test 2A succeeded.
    workerpool_CreateWorkerFromUrl.result2A = m.text;

    // Worker database SHOULD exist in parent origin.
    var db2 = google.gears.factory.create('beta.database', '1.0');
    db2.open('worker_js');
    db2.execute('drop table PING2').close();  // to throw if table doesn't exist
    db2.close();
    // If we reach this line, test 2B succeeded.
    workerpool_CreateWorkerFromUrl.result2B = 'DB ok';  // did not throw
  }
  var childId = wp2.createWorkerFromUrl(sameOriginPath + sameOriginWorkerFile);
  wp2.sendMessage('PING2', childId);


  // TEST 3
  workerpool_CreateWorkerFromUrl.result3A = '';
  workerpool_CreateWorkerFromUrl.description3A =
      'createWorkerFromUrl() Test 3A: ' +
      'Cross-origin, absolute URLs should work. ';
  workerpool_CreateWorkerFromUrl.result3B = '';
  workerpool_CreateWorkerFromUrl.description3B =
      'createWorkerFromUrl() Test 3B: ' +
      'And worker database should NOT exist in parent origin. ';

  // Cleanup any local DB before starting test.  
  var db3 = google.gears.factory.create('beta.database', '1.0');
  db3.open('worker_js');
  db3.execute('drop table if exists PING3').close();
  db3.close();

  var wp3 = google.gears.factory.create('beta.workerpool', '1.1');
  wp3.onmessage = function(text, sender, m) {
    // If we reach this line, test 3A succeeded.
    workerpool_CreateWorkerFromUrl.result3A = m.text + ' Origin: ' + m.origin;

    // Worker database should NOT exist in parent origin.
    var db3 = google.gears.factory.create('beta.database', '1.0');
    db3.open('worker_js');
    db3.execute('create table PING3 (DUMMY)');  // to throw if table exists
    db3.execute('drop table PING3').close();
    db3.close();
    // If we reach this line, test 3B succeeded.
    workerpool_CreateWorkerFromUrl.result3B = 'DB ok';  // did not throw
  }

  // TODO(cprince): In dbg builds, add a 2nd param to createWorkerFromUrl()
  // so callers can simulate a different origin without being online.
  //if (!gIsDebugBuild) {
  var childId = wp3.createWorkerFromUrl(crossOriginPath +
                                            crossOriginWorkerFile);
  //} else {
  //  var childId = wp3.createWorkerFromUrl(sameOriginPath +
  //                                            crossOriginWorkerFile,
  //                                        crossOriginPath);
  //}
  wp3.sendMessage('PING3', childId);


  // TEST 4
  workerpool_CreateWorkerFromUrl.expectedError4 =
      'http://example.com/non-existent-file.js';
  workerpool_CreateWorkerFromUrl.description4 =
      'createWorkerFromUrl() Test 4: ' +
      'File Not Found should fire error.';

  var wp4 = google.gears.factory.create('beta.workerpool', '1.1');
  gExpectedErrors.push(workerpool_CreateWorkerFromUrl.expectedError4);
  wp4.createWorkerFromUrl('http://example.com/non-existent-file.js');


  // TEST 5
  workerpool_CreateWorkerFromUrl.expectedError5 =
      'Page does not have permission to use Google Gears';
  workerpool_CreateWorkerFromUrl.description5 =
      'createWorkerFromUrl() Test 5: ' +
      'Cross-origin worker missing allowCrossOrigin() should fire error.';

  var wp5 = google.gears.factory.create('beta.workerpool', '1.1');
  gExpectedErrors.push(workerpool_CreateWorkerFromUrl.expectedError5);
  wp5.createWorkerFromUrl(crossOriginPath + sameOriginWorkerFile);
  // TODO(cprince): Could add debug-only origin override here too.
  wp5.sendMessage('PING5', childId);

}


function runWorkerPoolTests() {
  // Start the tests
  workerpool_SynchronizationStressTest();
  workerpool_GCWithFunctionClosures();
  workerpool_OnMessageTests();
  workerpool_OnErrorTests();
  workerpool_CreateWorkerFromUrl();

  // Check the results after a delay
  setTimeout('checkWorkerPoolTests()',
             MSEC_DELAY_BEFORE_CHECKING_WORKER_RESULTS);
}  


// checkWorkerPoolTests() should be called by setTimeout,
// after giving workers time to finish.

var MSEC_DELAY_BEFORE_CHECKING_WORKER_RESULTS = 8000;

function checkWorkerPoolTests() {

  // Checks for workerpool_SynchronizationStressTest()
  //
  // We validate the final number of messages received from each worker.

  insertRow('Workers: creation time (' +
              NUM_WORKERS + ' workers)', // testBody
            true,  // success
            '',  // reason
            workersCreateEndTime - workersCreateBeginTime); // execTime

  var succeeded = true;
  var resultString = '';
  for (var t = 0; t < NUM_WORKERS; ++t) {

    var workerId = workerIndexToWorkerId[t];
    var firstValueToRecv = t * EXPECTED_RESPONSES_PER_WORKER;
    var numReceived = nextValueToRecvFromWorkerId[workerId] - firstValueToRecv;
    if (numReceived != EXPECTED_RESPONSES_PER_WORKER) {
      succeeded = false;
    }
    resultString += 'Worker ' + workerId + ' = ' + numReceived + '. ';
  }

  insertRow('Workers: correctness, and send/recv time (' +
            EXPECTED_RESPONSES_PER_WORKER * NUM_WORKERS + ' msgs)', // testBody
            succeeded,  // success
            resultString,  // reason
            workersSendRecvEndTime - workersSendRecvBeginTime); // execTime

  // Checks for workerpool_GCWithFunctionClosures()
  //
  // We make sure the test finished without crashing.

  insertRow('workerpool_GCWithFunctionClosures()', // testBody
            pingReceived_GCWithFunctionClosures,  // success
            '',  // reason
            0); // execTime
            
  // Check the OnMessage tests.

  insertRow('workerpool_OnMessageTests.message_object_ok',
            workerpool_OnMessageTests.message_object_ok,
            '',
            0);

  // Check the OnError tests.

  insertRow('workerpool_OnErrorTests.handler_called',
            workerpool_OnErrorTests.handler_called,
            '',
            0);

  for (var i = 0, test; test = workerpool_bubbleTests[i]; i++) {
    test.checkResult();
  }

  // Check the CreateWorkerFromUrl tests.

  insertRow(workerpool_CreateWorkerFromUrl.description1,
            '' != workerpool_CreateWorkerFromUrl.result1,
            workerpool_CreateWorkerFromUrl.result1, // explanation
            0);

  insertRow(workerpool_CreateWorkerFromUrl.description2A,
            '' != workerpool_CreateWorkerFromUrl.result2A,
            workerpool_CreateWorkerFromUrl.result2A, // explanation
            0);

  insertRow(workerpool_CreateWorkerFromUrl.description2B,
            '' != workerpool_CreateWorkerFromUrl.result2B,
            workerpool_CreateWorkerFromUrl.result2B, // explanation
            0);

  insertRow(workerpool_CreateWorkerFromUrl.description3A,
            '' != workerpool_CreateWorkerFromUrl.result3A,
            workerpool_CreateWorkerFromUrl.result3A, // explanation
            0);

  insertRow(workerpool_CreateWorkerFromUrl.description3B,
            '' != workerpool_CreateWorkerFromUrl.result3B,
            workerpool_CreateWorkerFromUrl.result3B, // explanation
            0);

  insertRow(workerpool_CreateWorkerFromUrl.description4,
            receivedError(workerpool_CreateWorkerFromUrl.expectedError4),
            'Expected global error was not received', // explanation
            0);

  insertRow(workerpool_CreateWorkerFromUrl.description5,
            receivedError(workerpool_CreateWorkerFromUrl.expectedError5),
            'Expected global error was not received', // explanation
            0);
}


//------------------------------------------------------------------------------
// Timer tests
//------------------------------------------------------------------------------

var TIMER_TARGET_INTERVALS = 3;

var parentTimeoutTestSucceeded = false;
var parentIntervalTestIntervals = 0;
var workerTimeoutTestSucceeded = false;
var workerIntervalTestIntervals = 0;
var parent1000msTimeoutTime = 0;
var worker1000msTimeoutTime = 0;
var ParentTimeoutScriptTestSucceeded = false;
var ParentIntervalScriptTestIntervals = 0;
var WorkerTimeoutScriptTestSucceeded = false;
var WorkerIntervalScriptTestIntervals = 0;

function timer_ParentTimeoutTest() {
  var timerFired = false;
  var timer = google.gears.factory.create('beta.timer', '1.0');

  timer.setTimeout(timeoutHandler, 300);

  function timeoutHandler() {
    // Set success to true on the first firing, false if it fires again.
    parentTimeoutTestSucceeded = !timerFired;
    timerFired = true;
  }
}

function timer_ParentIntervalTest() {
  var timer = google.gears.factory.create('beta.timer', '1.0');
  var timerId = timer.setInterval(intervalHandler, 300);

  function intervalHandler() {
    ++parentIntervalTestIntervals;
    if (parentIntervalTestIntervals == TIMER_TARGET_INTERVALS) {
      timer.clearInterval(timerId);
    }
  }
}

function timer_WorkerTimeoutTest() {
  var timerFired = false;
  var workerPool = google.gears.factory.create('beta.workerpool', '1.1');
  workerPool.onmessage = function(text, sender, m) {
    if (text == 'timeout') {
      // Set success to true on the first firing, false if it fires again.
      workerTimeoutTestSucceeded = !timerFired;
      timerFired = true;
    }
  }

  var childCode = String(workerInit) +
                  String(workerOnmessage) +
                  'var timer;' +
                  'var parentId;' +
                  'workerInit();';

  var workerId = workerPool.createWorker(childCode);
  workerPool.sendMessage('setTimeout', workerId);

  function workerInit() {
    timer = google.gears.factory.create('beta.timer', '1.0');
    google.gears.workerPool.onmessage = workerOnmessage;
  }

  function workerOnmessage(text, sender, m) {
    if (m.text == 'setTimeout') {
      parentId = m.sender;
      timer.setTimeout(timeoutHandler, 300);
    }

    function timeoutHandler() {
      google.gears.workerPool.sendMessage('timeout', parentId);
    }
  }
}

function timer_WorkerIntervalTest() {
  var timerFired = false;
  var workerPool = google.gears.factory.create('beta.workerpool', '1.1');
  workerPool.onmessage = function(text, sender, m) {
    if (m.text == 'interval') {
      ++workerIntervalTestIntervals;
    }
  }

  var childCode = String(workerInit) +
                  String(workerOnmessage) +
                  'var timer;' +
                  'var parentId;' +
                  'var timerId;' +
                  'var intervals;' +
                  'var TIMER_TARGET_INTERVALS = ' +
                     String(TIMER_TARGET_INTERVALS) + ';' +
                  'workerInit();';

  var workerId = workerPool.createWorker(childCode);
  workerPool.sendMessage('setInterval', workerId);

  function workerInit() {
    timer = google.gears.factory.create('beta.timer', '1.0');
    google.gears.workerPool.onmessage = workerOnmessage;
  }

  function workerOnmessage(text, sender, m) {
    if (m.text == 'setInterval') {
      parentId = m.sender;
      intervals = 0;
      timerId = timer.setInterval(intervalHandler, 300);
    }

    function intervalHandler() {
      google.gears.workerPool.sendMessage('interval', parentId);
      ++intervals;
      if (intervals == TIMER_TARGET_INTERVALS) {
        timer.clearInterval(timerId);
      }
    }
  }
}

function timer_Parent1000msTimeoutTest() {
  var timerBegin = new Date().getTime();
  var timer = google.gears.factory.create('beta.timer', '1.0');

  timer.setTimeout(timeoutHandler, 1000);

  function timeoutHandler() {
    parent1000msTimeoutTime = new Date().getTime() - timerBegin;
  }
}

function timer_Worker1000msTimeoutTest() {
  var workerPool = google.gears.factory.create('beta.workerpool', '1.1');
  workerPool.onmessage = function(text, sender, m) {
    // The only message we get is the time delta
    worker1000msTimeoutTime = m.text;
  }

  var childCode = String(workerInit) +
                  String(workerOnmessage) +
                  'var timer;' +
                  'var parentId;' +
                  'var timerBegin;' +
                  'workerInit();';

  var workerId = workerPool.createWorker(childCode);
  workerPool.sendMessage('setTimeout', workerId);

  function workerInit() {
    timer = google.gears.factory.create('beta.timer', '1.0');
    google.gears.workerPool.onmessage = workerOnmessage;
  }

  function workerOnmessage(text, sender, m) {
    if (m.text == 'setTimeout') {
      parentId = m.sender;
      timerBegin = new Date().getTime();
      timer.setTimeout(timeoutHandler, 1000);
    }

    function timeoutHandler() {
      var delta = new Date().getTime() - timerBegin;
      google.gears.workerPool.sendMessage(String(delta), parentId);
    }
  }
}

var ParentTimeoutScriptTimerFired = false;
function timer_ParentTimeoutScriptTest() {
  var timer = google.gears.factory.create('beta.timer', '1.0');

  timer.setTimeout(
      'ParentTimeoutScriptTestSucceeded = !ParentTimeoutScriptTimerFired;' +
      'ParentTimeoutScriptTimerFired = true;',
      300);
}

var ParentIntervalScriptTimerId;
var ParentIntervalScriptTimer;
function timer_ParentIntervalScriptTest() {
  ParentIntervalScriptTimer = google.gears.factory.create('beta.timer', '1.0');
  ParentIntervalScriptTimerId =
      ParentIntervalScriptTimer .setInterval(
        'ParentIntervalScriptTestIntervals++;' +
        'if (ParentIntervalScriptTestIntervals == TIMER_TARGET_INTERVALS) {' +
        '  ParentIntervalScriptTimer.clearInterval(' +
        '      ParentIntervalScriptTimerId);' +
        '}',
        300);
}

function timer_WorkerTimeoutScriptTest() {
  var timerFired = false;
  var workerPool = google.gears.factory.create('beta.workerpool', '1.1');
  workerPool.onmessage = function(text, sender, m) {
    if (m.text == 'timeout') {
      // Set success to true on the first firing, false if it fires again.
      WorkerTimeoutScriptTestSucceeded = !timerFired;
      timerFired = true;
    }
  }

  var childCode = String(workerInit) +
                  String(workerOnmessage) +
                  'var timer;' +
                  'var parentId;' +
                  'workerInit();';

  var workerId = workerPool.createWorker(childCode);
  workerPool.sendMessage('setTimeout', workerId);

  function workerInit() {
    timer = google.gears.factory.create('beta.timer', '1.0');
    google.gears.workerPool.onmessage = workerOnmessage;
  }

  function workerOnmessage(text, sender, m) {
    if (m.text == 'setTimeout') {
      parentId = m.sender;
      timer.setTimeout(
        'google.gears.workerPool.sendMessage(\'timeout\', parentId);',
        300);
    }
  }
}

function timer_WorkerIntervalScriptTest() {
  // Checks that intervals created via the full script setInterval() in workers
  // fire until clearInterval() is called, and no longer.
  var workerPool = google.gears.factory.create('beta.workerpool', '1.1');
  workerPool.onmessage = function(text, sender, m) {
    if (m.text == 'interval') {
      WorkerIntervalScriptTestIntervals++;
    }
  }

  var childCode = String(workerInit) +
                  String(workerOnmessage) +
                  'var timer;' +
                  'var parentId;' +
                  'var timerId;' +
                  'var intervals;' +
                  'var TIMER_TARGET_INTERVALS = ' +
                     String(TIMER_TARGET_INTERVALS) + ';' +
                  'workerInit();';

  var workerId = workerPool.createWorker(childCode);
  workerPool.sendMessage('setInterval', workerId);

  function workerInit() {
    timer = google.gears.factory.create('beta.timer', '1.0');
    google.gears.workerPool.onmessage = workerOnmessage;
  }

  function workerOnmessage(message, sender) {
    if (message == 'setInterval') {
      parentId = sender;
      intervals = 0;
      timerId = timer.setInterval(
        'google.gears.workerPool.sendMessage(\'interval\', parentId);' +
        '++intervals;' +
        'if (intervals == TIMER_TARGET_INTERVALS) {' +
        '  timer.clearInterval(timerId);' +
        '}',
        300);
    }
  }
}

function runTimerTests() {
  // Start the tests
  timer_ParentTimeoutTest();
  timer_ParentIntervalTest();
  timer_WorkerTimeoutTest();
  timer_WorkerIntervalTest();
  timer_Parent1000msTimeoutTest();
  timer_Worker1000msTimeoutTest();
  timer_ParentTimeoutScriptTest();
  timer_ParentIntervalScriptTest();
  timer_WorkerTimeoutScriptTest();
  timer_WorkerIntervalScriptTest();

  // Check the results after a delay
  setTimeout('checkTimerTests()',
             MSEC_DELAY_BEFORE_CHECKING_TIMER_RESULTS);
}


// checkTimerTests() should be called by setTimeout,
// after giving workers time to finish.
var MSEC_DELAY_BEFORE_CHECKING_TIMER_RESULTS = 4000;

// Parent timer test gets larger tolerance because it is in the same thread
// as the rest of the unit tests.  Worker is in its own thread, so the
// maximum delta should be smaller.
var MSEC_MAX_TIMER_EPSILON_IN_PARENT = 200;  // for 1000 ms interval
var MSEC_MAX_TIMER_EPSILON_IN_WORKER = 50;   // for 1000 ms interval

function checkTimerTests() {

  // We make sure the test finished without crashing.
  insertRow(String(timer_ParentTimeoutTest), // testBody
            parentTimeoutTestSucceeded,  // success
            '',  // reason
            0); // execTime
  insertRow(String(timer_ParentIntervalTest), // testBody
            parentIntervalTestIntervals == TIMER_TARGET_INTERVALS,
            String(parentIntervalTestIntervals) + ' intervals observed, ' +
                'expected ' + TIMER_TARGET_INTERVALS + ' intervals.',  // reason
            0); // execTime
  insertRow(String(timer_WorkerTimeoutTest), // testBody
            workerTimeoutTestSucceeded,  // success
            '',  // reason
            0); // execTime
  insertRow(String(timer_WorkerIntervalTest), // testBody
            workerIntervalTestIntervals == TIMER_TARGET_INTERVALS,
            String(workerIntervalTestIntervals) + ' intervals observed, ' +
                'expected ' + TIMER_TARGET_INTERVALS + ' intervals.',  // reason
            0); // execTime
  insertRow(String(timer_Parent1000msTimeoutTest),
            Math.abs(1000 - parent1000msTimeoutTime)
                < MSEC_MAX_TIMER_EPSILON_IN_PARENT,
            'Took ' + parent1000msTimeoutTime + ' ms, expected 1000 +/- ' +
                MSEC_MAX_TIMER_EPSILON_IN_PARENT + ' ms.',  // reason
            parent1000msTimeoutTime); // execTime
  insertRow(String(timer_Worker1000msTimeoutTest),
            Math.abs(1000 - worker1000msTimeoutTime)
              < MSEC_MAX_TIMER_EPSILON_IN_WORKER,
            'Took ' + worker1000msTimeoutTime + ' ms, expected 1000 +/- ' +
                MSEC_MAX_TIMER_EPSILON_IN_WORKER + ' ms.',  // reason
            worker1000msTimeoutTime); // execTime

  insertRow(String(timer_ParentTimeoutScriptTest), // testBody
            ParentTimeoutScriptTestSucceeded,  // success
            '',  // reason
            0); // execTime
  insertRow(String(timer_ParentIntervalScriptTest), // testBody
            ParentIntervalScriptTestIntervals == TIMER_TARGET_INTERVALS,
            String(ParentIntervalScriptTestIntervals) + ' intervals observed,' +
                ' expected ' + TIMER_TARGET_INTERVALS + ' intervals.', // reason
            0); // execTime
  insertRow(String(timer_WorkerTimeoutScriptTest), // testBody
            WorkerTimeoutScriptTestSucceeded,  // success
            '',  // reason
            0); // execTime
  insertRow(String(timer_WorkerIntervalScriptTest), // testBody
            WorkerIntervalScriptTestIntervals == TIMER_TARGET_INTERVALS,
            String(WorkerIntervalScriptTestIntervals) + ' intervals observed,' +
                ' expected ' + TIMER_TARGET_INTERVALS + ' intervals.', // reason
            0); // execTime
}


//------------------------------------------------------------------------------
// LocalServer tests
//------------------------------------------------------------------------------

var STORE_NAME = "unittest-store";
var CAPTURE_URI = "unit_tests.js";
var EXPECTED_CAPTURE_URI_CONTENT = "// Copyright 2007, Google Inc.";
var CAPTURE_TEST_FILE_FRAGMENT = "test_file_fragment";
var CAPTURE_TEST_FILES = {
  "test_file_0.txt": 0,
  "nonexistent_file": -1,  // should fail
  "test_file_1.txt": 1,
  "server_redirect.php?location=nonexistent_file": -1, // should fail
  "test_file_1024.txt": 1024
};
var localServer;
var webCaptureStore = null;
var captureComplete = false;
var captureCompleteCount = 0;
var captureExpectedCount = 0;

function testLocalServerEmptyParams() {
  var shouldFail = [
    "localServer.createStore()",
    "localServer.createStore('')",
    "localServer.createStore(null)",
    "localServer.createStore(undefined)",
    "localServer.createManagedStore()",
    "localServer.createManagedStore('')",
    "localServer.createManagedStore(null)",
    "localServer.createManagedStore(undefined)"
  ];
  
  for (var i = 0, str; str = shouldFail[i]; i++) {
    try {
      eval(str);
    } catch (e) {
      // good, we expected an exception
      continue;
    }
    insertRow("testLocalServerEmptyParams", false,
              "Did not get expected error for: " + str);
    return;
  }
  insertRow("testLocalServerEmptyParams", true, "");
}

function runWebCaptureTests() {
  if (location.protocol.indexOf("http") != 0) {
    insertRow("webcapture tests", false,
              "Webcapture tests must be run from an http URL.");
    return;
  }

  // Verify we can create a local server object
  localServer =
      google.gears.factory.create("beta.localserver", "1.0");
  insertNotExpected("Create LocalServer object", null, localServer);

  testLocalServerEmptyParams();

  // Start from a clean slate
  localServer.removeStore(STORE_NAME);

  // If we have a debug build of Google Gears, run it's internal tests
  if (gIsDebugBuild) {
    insertExpected("webcache internal tests", true,
                   localServer.canServeLocally("test:webcache"));
  }

  // Verify we can create a resource store object
  var wcs = localServer.createStore(STORE_NAME);
  insertNotExpected("localServer.createStore", null, wcs);
  webCaptureStore = wcs;

  testCaptureFile(wcs);
  testFileSubmitter(wcs);

  // Verify isCaptured() returns false when it should.
  insertExpected("webCaptureStore.isCaptured", false,
                 wcs.isCaptured(CAPTURE_URI));

  // Initiate the capture of some files, a single url and an array
  var uri_array = [];
  for (var uri in CAPTURE_TEST_FILES) {
    uri_array.push(uri);
  }
  captureExpectedCount = 2 + uri_array.length;
  wcs.capture(CAPTURE_URI, webCaptureOneCallback);
  wcs.capture(CAPTURE_TEST_FILE_FRAGMENT + "#foo", webCaptureFragmentCallback);
  wcs.capture(uri_array, webCaptureArrayCallback);
  try {
    var crossDomainUrl = "http://cross.domain.not/";
    wcs.capture(crossDomainUrl, webCaptureShouldNotCallback);
  } catch (ex) {
    insertRow("webCaptureStore.capture(crossDomainUrl)", true,
              "Failed as expected");
  }
  
  // null completion callback should be allowed
  wcs.capture('nonexistent_file', null);

  // Give it 30 seconds to finish.
  setTimeout(webCapture_timeout, 30000);
}

function testCaptureFile(wcs) {
    // We call captureFile and expect it to throw given an empty
    // fileInputElement
    var fileInputElement = document.getElementById("fileinput");
    try {
      wcs.captureFile(fileInputElement, "shouldFail");
      insertRow("webCaptureStore.captureFile(emptyFileInput)", false,
                "Should have failed, but did not");
    } catch (e) {
      if (e.message == "Invalid file input parameter.") {
        insertRow("webCaptureStore.captureFile(emptyFileInput)", false,
                  "We need to add an nsIContent IID to our list");
      } else {
        insertRow("webCaptureStore.captureFile(emptyFileInput)", true,
                  "Failed as expected");
      }
    }

    // Test that script can't spoof a file input element, applies to Firefox
    var fileInputSpoofer = {
      type: "file",
      value: "c:\\autoexec.bat",
      QueryInterface: function(iid) {
        return this;
      }
    }
    try {
      wcs.captureFile(fileInputSpoofer, "shouldFailAgain");
      insertRow("webCaptureStore.captureFile(bogusFileInput)", false,
                "Should have failed, but did not");
    } catch (e) {
      if (e.message != "Invalid file input parameter.") {
        insertRow("webCaptureStore.captureFile(bogusFileInput)", false,
                  "Wrong error message, e.message = " + e.message);
      } else {
        insertRow("webCaptureStore.captureFile(bogusFileInput)", true,
                  "Failed as expected");
      }
    }
    
    // Test that script can't pass a filepath as a string and have it captured
    try {
      wcs.captureFile("c:\\autoexec.bat", "shouldFailAgain");
      insertRow("webCaptureStore.captureFile(string)", false,
                "Should have failed, but did not");
    } catch (e) {
      insertRow("webCaptureStore.captureFile(string)", true,
                "Failed as expected");
    }
    // TODO(michaeln): create a test that actually grabs a local file
}

function testFileSubmitter(wcs) {
  // Verify we can create a file submitter object
  var submitter = wcs.createFileSubmitter();
  insertNotExpected("webCaptureStore.createFileSubmitter", null, submitter);
  // TODO(michaeln): create a test that actually submits to a server
}

function webCapture_timeout() {
  if (!captureComplete) {
    insertRow("webCaptureStore.capture tests did not complete", false);
  }
}

function webCaptureOneCallback(url, success, id) {
  ++captureCompleteCount;
  insertRow(url + " : capture complete", success);

  if (success) {
    var wcs = webCaptureStore;

    // Our uri should be captured now, verify isCaptured() returns true.
    insertExpected("webCaptureStore.isCaptured", true,
                   wcs.isCaptured(CAPTURE_URI));

    // Make a copy of it
    var copyUri = "Copy_of_" + CAPTURE_URI;
    wcs.copy(CAPTURE_URI, copyUri);
    insertExpected("webCaptureStore.copy", true,
                   wcs.isCaptured(copyUri) && wcs.isCaptured(CAPTURE_URI));

    // Rename it
    var renameUri = "Renamed_" + CAPTURE_URI;
    wcs.rename(CAPTURE_URI, renameUri);
    insertExpected("webCaptureStore.rename", true,
                   wcs.isCaptured(renameUri) && !wcs.isCaptured(CAPTURE_URI));

    // Verify the local server claims to be able to serve renameUri now
    insertExpected("webCaptureLocalServer.canServeLocally", true,
                   localServer.canServeLocally(renameUri));

    // Fetch the contents of renameUri and see if its what we expect
    var content = httpGet(renameUri);
    insertExpected("webCaptured expected content", true,
                   stringStartsWith(content, EXPECTED_CAPTURE_URI_CONTENT));

    // Disable our store and verify we can no longer fetch renameUri
    // note: depends on renameUri not being available on the server)
    wcs.enabled = false;
    insertExpected("localServer.canServeLocally disabled", false,
                   localServer.canServeLocally(renameUri));
    insertExpected("local serving disabled", true,
                    httpGet(renameUri) == null);

    // Re-enable the store and test redirects back into our cache
    // note: depends on renameUri not being available on the server
    wcs.enabled = true;
    insertExpected("redirects back into our cache", true,
        httpGet("server_redirect.php?location=" + renameUri) == content);

    // Now remove the uris, and verify isCaptured() returns false.
    wcs.remove(renameUri);
    wcs.remove(copyUri);
    insertExpected("webCaptureStore.remove", false,
                   wcs.isCaptured(renameUri) || wcs.isCaptured(copyUri));
  }
}

function webCaptureFragmentCallback(url, success, id) {
  ++captureCompleteCount;
  insertRow(url + " : capture complete", success);

  if (success) {
    var wcs = webCaptureStore;
    insertExpected(CAPTURE_TEST_FILE_FRAGMENT + " isCaptured", true,
                   wcs.isCaptured(CAPTURE_TEST_FILE_FRAGMENT));
    insertExpected(CAPTURE_TEST_FILE_FRAGMENT + "#foo isCaptured", true,
                   wcs.isCaptured(CAPTURE_TEST_FILE_FRAGMENT + "#foo"));
    insertExpected(CAPTURE_TEST_FILE_FRAGMENT + "#bar isCaptured", true,
                   wcs.isCaptured(CAPTURE_TEST_FILE_FRAGMENT + "#bar"));
  }
}

function webCaptureArrayCallback(url, success, id) {
  var expectedLength = CAPTURE_TEST_FILES[url]
  var shouldSucceed = expectedLength != -1;

  ++captureCompleteCount;
  insertRow(url + " : capture complete", shouldSucceed == success);

  if (success && shouldSucceed) {
    var content = httpGet(url);
    insertRow(url + " : fetched captured content", content != null);
    insertExpected(url + " : corrent content type header",
                   "text/plain",
                   webCaptureStore.getHeader(url, "Content-Type"));
    if (content != null) {
      insertExpected(url + " : correct content length",
                     expectedLength,
                     content.length);
    }
  }

  if (captureCompleteCount == captureExpectedCount) {
    captureComplete = true;
    localServer.removeStore(STORE_NAME);
    var wcs = localServer.openStore(STORE_NAME);
    insertExpected("localServer.removeStore", null, wcs);
  }
}

function webCaptureShouldNotCallback(url, success, id) {
  insertRow("store.capture should not callback", false, url);
}

var manifest_url1_content;
function runManifestTests() {
  if (location.protocol.indexOf("http") != 0) {
    insertRow("manifest tests", false,
              "Manifest tests must be run from an http URL.");
    return;
  }

  // Note dependency on content of manifest-good.txt
  // Fetch prior to updating to get it from the server
  manifest_url1_content = httpGet("manifest-url1.txt");
  testManifest("manifest-good.txt", checkGoodManifest);
}

function checkGoodManifest(app) {
  insertExpected("updateStatus should be OK after good manifest",
                 g.UPDATE_STATUS_OK,
                 app.updateStatus);

  // TODO(aa): It would be cool if we could actually return null in this case
  insertExpected("lastErrorMessage should be empty string after good manifest",
                 "",
                 app.lastErrorMessage);

  if (app.updateStatus == g.UPDATE_STATUS_OK) {
    var test_file_1_content = httpGet("test_file_1.txt")
    insertExpected("can serve expected urls", true,
        localServer.canServeLocally("manifest-url1.txt") &&
        localServer.canServeLocally("manifest-url1.txt?query") &&
        localServer.canServeLocally("alias-to-manifest-url1.txt") &&
        localServer.canServeLocally("redirect-to-manifest-url1.txt") &&
        localServer.canServeLocally("unicode?foo=bar"));
    insertExpected("is serving expected content", true,
        manifest_url1_content == httpGet("manifest-url1.txt") &&
        test_file_1_content == httpGet("manifest-url1.txt?another") &&
        manifest_url1_content == httpGet("alias-to-manifest-url1.txt") &&
        manifest_url1_content == httpGet("redirect-to-manifest-url1.txt") &&
        test_file_1_content == httpGet("unicode?foo=bar"));
  }

  // now check again with a bad manifest
  testManifest("manifest-bad.txt", checkBadManifest);
}

function checkBadManifest(app) {
  insertExpected("updateStatus should be FAILED after bad manifest",
                 g.UPDATE_STATUS_FAILED,
                 app.updateStatus);

  var re = /^Download of '.+?url2.txt' returned response code 404$/;
  insertExpected("correct lastErrorMessage after bad manifest", true,
                 re.test(app.lastErrorMessage));

  testManifest("manifest-ugly.txt", checkInvalidManifest);
}

function checkInvalidManifest(app) {
  insertExpected("updateStatus should be FAILED after invalid manifest",
                 g.UPDATE_STATUS_FAILED,
                 app.updateStatus);

  insertExpected("correct lastErrorMessage after invalid manifest", true,
                 stringStartsWith(app.lastErrorMessage, "Invalid manifest"));

  testManifest("manifest-illegal-redirect.txt", checkIllegalRedirectManifest);
}

function checkIllegalRedirectManifest(app) {
  insertExpected("updateStatus should be FAILED after illegal-redirect manifest",
                 g.UPDATE_STATUS_FAILED,
                 app.updateStatus);

  insertExpected("correct lastErrorMessage after illegal-redirect manifest",
                 true, stringEndsWith(app.lastErrorMessage, "302"));
}

function testManifest(url, callback) {
  try {
    if (!localServer)
      localServer =
          google.gears.factory.create("beta.localserver", "1.0");
    localServer.removeManagedStore(g.CAPTURED_APP_NAME);

    var app = localServer.createManagedStore(g.CAPTURED_APP_NAME);
    app.manifestUrl = url;
    app.checkForUpdate();
  } catch(e) {
    insertRow("testManifest " + url + " exception - " + e.message, false);
  }

  // Wait for the update check to complete

  var kTimeoutMsec = 10000;     // 10 seconds
  var kPollIntervalMsec = 100;  // 10 times per second
  var start = new Date().getTime();

  function waitForUpdateComplete() {
    var status = app.updateStatus;
    if (status != g.UPDATE_STATUS_FAILED && status != g.UPDATE_STATUS_OK) {
      var now = new Date().getTime();
      if (now - start < kTimeoutMsec) {
        window.setTimeout(waitForUpdateComplete, kPollIntervalMsec);
      } else {
        callback(app);
      }
    } else {
      callback(app);
    }
  }
  window.setTimeout(waitForUpdateComplete, kPollIntervalMsec);
}

// Synchronously get a url and return the body as text. If the request
// fails, returns null.
function httpGet(url) {
  var xmlhttp;
  if (window.XMLHttpRequest) {
    xmlhttp = new XMLHttpRequest();
  } else if (window.ActiveXObject) {
    xmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
  }
  xmlhttp.open("GET", url, false);
  xmlhttp.send(null);
  if (xmlhttp.status != 200) {
    return null;
  } else {
    return xmlhttp.responseText;
  }
}


// Returns true if str starts with prefix. Returns false if
// str does not start with prefix, or if str or prefix are null or undefined.
function stringStartsWith(str, prefix) {
  return str && prefix &&
         str.length >= prefix.length &&
         str.substring(0, prefix.length) == prefix;
}

// Returns true if str ends with postfix.
function stringEndsWith(str, postfix) {
  return str && postfix &&
         str.length >= postfix.length &&
         str.substring(str.length - postfix.length) == postfix;
}


//------------------------------------------------------------------------------
// Gears.HttpRequest tests
//------------------------------------------------------------------------------


function runHttpRequestTests() {
  httpRequestTestSuite(false);    // run on the main thread
  runHttpRequestTestsOnWorker();  // run on a worker thread
}


// Called upon completion of each distinct test. Both main and worker
// test results get plumbed thru this function.
function httpRequestTestComplete(testName, result) {
  insertRow(testName, result);
}


// Helper that runs our test suite in a worker
function runHttpRequestTestsOnWorker() {
  function myOnmessage(text, sender, m) {
    var retvals = eval('(' + m.text + ')');
    httpRequestTestComplete('<b>[WORKER THREAD]</b> ' + retvals[0], // testName
                            retvals[1]); // success;
  }
  
  function childTestComplete(testName, result) {
    google.gears.workerPool.sendMessage('["' + testName + '", ' + result + ']',
                                        0);  // parent id
  }

  var childCode = 'var httpRequestTestComplete = ' + childTestComplete + ';' + 
                  'var runIt = ' + httpRequestTestSuite +  ';' + 
                  'runIt(true);';

  var workerPool = google.gears.factory.create('beta.workerpool', '1.1');
  workerPool.onmessage = myOnmessage;
  try {
    workerPool.createWorker(childCode);
  } catch (e) {
    insertRow('While creating worker in runHttpRequestTestsOnWorker...',
              false,  // success
              e.message,  // reason
              0); // execTime
  }
}


// Function that encapsulates the HttpRequest test suite. Upon completion
// of each test the external function httpRequestTestComplete is invoked.
// Other than that, this function is self contained.
function httpRequestTestSuite(inWorker) {
  // TODO(michaeln): detect if tests fail to complete and report those as errors
  var tests = [
    'httpGet_200',
    'httpPost_200',
    'httpGet_404',
    'httpGet_302_200',
    'httpGet_302_404',
    'httpPost_302_200',
    'httpGet_NoCrossOrigin',
    'httpGet_302_NoCrossOrigin',
    'httpRequestReuse',
    'httpRequest_DisallowedHeaders',
    'httpGet_CapturedResource',
    'nullOnReadyStateChange'
  ];

  for (var testIndex = 0; testIndex < tests.length; ++testIndex) {
    var testName = tests[testIndex];
    try {
      var testProc = eval(testName);
      testProc(testName);
    } catch (e) {
      httpRequestTestComplete(testName + ', ' + e.message, false);    
    }
  }
  return;  // Only nested functions follow

  function httpGet_200(testName) {
    testRequest(testName,
        'test_file_1.txt', 'GET', null, null, // url, method, data, reqHeaders[]
        200, '1', null);  // expected status, responseText, responseHeaders[]
  }

  function httpPost_200(testName) {
    var data = 'hello';
    var headers = [["Name1", "Value1"],
                   ["Name2", "Value2"]];
    var expectedHeaders = getExpectedEchoHeaders(headers);
    testRequest(testName,
        'echo_request.php', 'POST', data, headers,
        200, data, expectedHeaders); 
  }

  function httpPost_302_200(testName) {
    // A POST that gets redirected should GET the new location
    var data = 'hello';
    var expectedHeaders = [["echo-Method", "GET"]];
    testRequest(testName,
        'server_redirect.php?location=echo_request.php',
        'POST', data, null,
        200, null, expectedHeaders); 
  }
  
  function httpGet_404(testName) {
    testRequest(testName,
        'nosuchfile___', 'GET', null, null, // url, method, data, reqHeaders[]
        404, null, null);  // expected status, responseText, responseHeaders[]
  }

  function httpGet_302_200(testName) {
    testRequest(testName,
        'server_redirect.php?location=test_file_1.txt', 'GET', null, null,
        200, '1', null);  // expected status, responseText, responseHeaders[]
  }

  function httpGet_302_404(testName) {
    testRequest(testName,
        'server_redirect.php?location=nosuchfile___', 'GET', null, null,
        404, null, null);  // expected status, responseText, responseHeaders[]
  }  

  function httpGet_NoCrossOrigin(testName) {
    try {
      testRequest(testName,
          'http://www.google.com/', 'GET', null, null,
          0, null, null);  // expected status, responseText, responseHeaders[]
      httpRequestTestComplete(testName + ', should fail to initiate', false);    
    } catch(e) {
      httpRequestTestComplete(testName + ', ' + e.message, true);
    }
  }

  function httpGet_302_NoCrossOrigin(testName) {
    var headers = [["location", "http://www.google.com/"]];
    testRequest(testName,
        'server_redirect.php?location=http://www.google.com/',
        'GET', null, null,
        302, "", headers);
  }

  function httpRequest_DisallowedHeaders(testName) {
    var headers = [["Referer", "http://somewhere.else.com/"]];
    try {
      testRequest(testName,
          'should_fail',
          'GET', null, headers,
          null, null, null);
      httpRequestTestComplete(testName + ', should fail to initiate', false);    
    } catch(e) {
      httpRequestTestComplete(testName + ', ' + e.message, true);
    }
  }
  
  function httpRequestReuse(testName) {
    var reusedRequest = google.gears.factory.create('beta.httprequest', '1.0');
    var numGot = 0;
    var numToGet = 2;
  
    getOne();
    
    function getOne() {          
      if (numGot >= numToGet) {
        httpRequestTestComplete(testName, true);
        return;
      }  
      var url = 'echo_request.php?' + numGot++;
      reusedRequest.onreadystatechange = function() {
        if (reusedRequest.readyState==4) {
          try {
            if (reusedRequest.status != 200)
              throw 'status != 200';
            getOne();
          } catch (e) {
            httpRequestTestComplete(testName + ', ' + e.message, false);
          }
        }
      }
      reusedRequest.open('GET', url, true);
      reusedRequest.send(null);
    }    
  }
  
  function httpGet_CapturedResource(testName) {
    var myLocalServer = google.gears.factory.create('beta.localserver', '1.0');
    // We don't delete and recreate the store or captured url to avoid
    // interfering with this same test running in the other thread. 
    var myStore = myLocalServer.createStore('HTTP_TEST_STORE');
    myStore.capture('echo_request.php?httprequest_a_captured_url',
                    myCaptureComplete);
        
    function myCaptureComplete(url, success, id) {
      try {
        if (!success)
          throw 'Capture failed'
        testRequest(testName, url, 'GET', null, null, 200, null, null);
      } catch (e) {
        httpRequestTestComplete(testName + ', ' + e.message, false);
      }
    }
  }
    
  function httpGet_BinaryResponse(testName) {
    // TODO(michaeln): do something reasonable with binary responses
  }

  function nullOnReadyStateChange(testName) {
    var nullHandlerRequest = google.gears.factory.create('beta.httprequest',
                                                         '1.0');
    nullHandlerRequest.onreadystatechange = function() {};
    nullHandlerRequest.onreadystatechange = null;
    nullHandlerRequest.open('GET', 'nosuchfile___');
    nullHandlerRequest.send();

    var unsetHandlerRequest = google.gears.factory.create('beta.httprequest',
                                                          '1.0');
    unsetHandlerRequest.open('GET', 'nosuchfile___');
    unsetHandlerRequest.send();

    httpRequestTestComplete(testName, true);
}

  // Generates header name value pairs echo_requests.php will respond
  // with for the given request headers.
  function getExpectedEchoHeaders(requestHeaders) {
    var echoHeaders = [];
    for (var i = 0; i < requestHeaders.length; ++i) {
      var name = 'echo-' + requestHeaders[i][0];
      var value = requestHeaders[i][1];
      echoHeaders.push([name, value]);
    }
    return echoHeaders;
  }

  // A helper that initiates a request and examines the response.
  function testRequest(testName,
                       url, method, data, requestHeaders,
                       expectedStatus,
                       expectedResponse,
                       expectedHeaders) {
    var request = google.gears.factory.create('beta.httprequest', '1.0');
    var isComplete = false;

    request.onreadystatechange = function() {
      try {
        var state = request.readyState;
        if (state < 0 || state > 4) {
          throw "Invalid readyState value, " + state;
        }
        if (request.readyState >= 3) {
          // fetch all of the values we can fetch
          var status = request.status;
          var statusText = request.statusText;
          var headers = request.getAllResponseHeaders();

          if (request.readyState == 4) {
            if (isComplete) {
              httpRequestTestComplete(
                  testName + ', completion called too many times',
                  false);
              return;
            }
            isComplete = true;
            var responseText = request.responseText;
          }

          // see if we got what we expected to get
          var result = true;
          var msg = '';
          if (expectedStatus != null) {
            if (expectedStatus != status) {
              result = false;
              msg += ', status!=expected'
            }
          }
          if (expectedHeaders != null) {
            for (var i = 0; i < expectedHeaders.length; ++i) {
              var name = expectedHeaders[i][0];
              var expectedValue = expectedHeaders[i][1];
              var actualValue = request.getResponseHeader(name);
              if (actualValue != expectedValue) {
                result = false;
                msg += ', hdr[' + name + '] ' +
                       actualValue + '!=' +
                       expectedValue;
              }
            }
          }
          if (isComplete && expectedResponse != null) {
            if (expectedResponse != responseText) {
              result = false;
              msg += ', resposeText!=expected'
            }
          }

          if (isComplete || !result) {
            httpRequestTestComplete(testName + msg, result);
          }
        }
      } catch(e) {
        httpRequestTestComplete(testName + ', ' + e.message, false);
      }
    };
    request.open(method, url, true);
    if (requestHeaders) {
      for (var i = 0; i < requestHeaders.length; ++i) {
        request.setRequestHeader(requestHeaders[i][0],
                                 requestHeaders[i][1]);
      }
    }
    request.send(data);  
  }
}
