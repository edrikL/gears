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

function Gears() {
  this.initFactory_();

  this.hasGears = Boolean(this.factory_);
  if (!this.hasGears) {
    return;
  }

  this.db_ = this.factory_.create('beta.database', '1.0');
  this.localServer_ = 
      this.factory_.create('beta.localserver', '1.0');

  this.db_.open();
  try {
    this.db_.execute('select * from user');
    this.hasDb = true;
  } catch (e) {
    this.hasDb = false;
  }

  this.app_ = this.localServer_.openManagedStore('gearpad');
  this.isCaptured = this.app_ && this.app_.currentVersion;
  this.canGoLocal = this.hasGears && this.hasDb && this.isCaptured;
}

Gears.NOBODY_USER_ID = -1;

Gears.prototype.transact = function(fn) {
  // TODO: Implement nested transaction support if necessary.
  this.execute('begin');
  try {
    fn();
  } catch (e) {
    this.execute('rollback');
    throw e;
  }
  this.execute('commit');
};

Gears.prototype.capture = function() {
  console.log('Checking for updates...');
  this.app_ = this.localServer_.createManagedStore('gearpad');
  this.app_.manifestUrl = 'manifest.php';
  this.app_.checkForUpdate();

  var self = this;
  var timerId = window.setInterval(function() {
    console.log('update status: ' + self.app_.updateStatus);

    if (self.app_.updateStatus == 3) { // error
      window.clearInterval(timerId);
      console.warn('update failed: ' + self.app_.lastErrorMessage);
    }

    if (self.app_.updateStatus == 0) { // ok
      location.reload();
    }
  }, 500);
};

Gears.prototype.createDatabase = function() {
  var schema = 'create table user (' +
      // the id of the user on the server
      'id int not null primary key, ' +
      // the last version from the server that we got
      'version int not null, ' + 
      // whether the localstore is dirty wrt the server
      'dirty int not null, ' +
      // cookie that should be used when this user is active
      'cookie text not null, ' +
      // current contentof note
      'content text not null)';

  this.execute(schema);
};

Gears.prototype.addUser = function(userid, cookie, content, version) {
  var self = this;
  this.transact(function() {
    var rslt = self.executeToObjects('select * from user where id = ?', 
        [userid])[0];

    if (rslt) {
      return;
    }

    self.execute('insert into user values (?, ?, ?, ?, ?)', 
                 [userid, version, 0, cookie, content]);
  });
};

Gears.prototype.getUser = function(userId) {
  return this.executeToObjects('select * from user where id = ?', [userId])[0];
};

Gears.prototype.executeToObjects = function(sql, args) {
  var rs = this.execute(sql, args);
  try {
    var rv = [];
    if (rs && rs.isValidRow()) {
      var cols = rs.fieldCount();
      var colNames = [];
      for (var i = 0; i < cols; i++) {
	colNames.push(rs.fieldName(i));
      }

      while (rs.isValidRow()) {
	var h = {};
	for (i = 0; i < cols; i++) {
	  h[colNames[i]] = rs.field(i);
	}
	rv.push(h);
	rs.next();
      }
    }
  } catch (e) {
    throw e;
  } finally {
    rs.close();
    return rv;
  }
}

/**
 * Helper that executes a sql and throws any error that occurs. Errors that come
 * from Gears are not real errors in Firefox, so they don't get handled by the
 * error reporting UI correctly.
 */
Gears.prototype.execute = function(sql, args) {
  try {
    if (args) {
      return this.db_.execute(sql, args);
    } else {
      return this.db_.execute(sql);
    }
  } catch (e) {
    // e is not a real error, so we can't just let it bubble up -- it won't 
    // display in error uis correctly. So we basically convert it to a real 
    // error.
    throw new Error("Error executing SQL: " + sql + ". Error was: " + e.message);
  }
};

Gears.prototype.initFactory_ = function() {
  // Firefox
  if (typeof GearsFactory != 'undefined') {
    this.factory_ = new GearsFactory();
  }

  try {
    this.factory_ = new ActiveXObject("Gears.Factory");
  } catch (e) {
    // ignore, probably we are not IE?
  }
};
