Work in progress

---


# Introduction #

This proposal aims to implement a Gears module based on the existing specification, [HTML5 Storage API](http://www.whatwg.org/specs/web-apps/current-work/multipage/section-sql.html). This specification is currently implemented in the [WebKit trunk](http://trac.webkit.org/projects/webkit/browser/trunk/WebCore/storage). There is also an incomplete [player](http://attic.glazkov.com/player/) implementation, written in Javascript as a Gears worker.

# Goals #

The goals of the proposal are:
  * Enable Javascript developers to easily write code that works with both Gears and browser database APIs.
  * Reduce developer "mind-print" by implementing the same API that is available in browsers.
  * Support the proposed HTML5 database standard with an implementation available for all browsers that Gears supports.
  * Implement an asynchronous API that can be called from the UI thread without freezing the UI.
  * Implement a synchronous API to simplify usage inside workers.

Minor/bonus goals:
  * implement a thread pool abstraction that can be used in other modules for asynchronous operations
  * build a new module from scratch using the new Dispatcher model.

Non-goals:
  * Allow current gears databases to work with database2
  * Allow access to database2 from the DOM window object (callers will access database2 through the Gears factory, just like any other Gears object).
  * Allow access to any native browser implementation of HTML5 databases


# Javascript Interface #

The standard interface is defined in [HTML 5 specification](http://www.whatwg.org/specs/web-apps/current-work/multipage/section-sql.html). Gears makes some additions for compatibility with native implementations and to add synchronous DB access.


## DatabaseManager ##

The database manager is the top-level entrypoint to the database2 API in Gears. Callers create instances of it using:

```
  google.gears.factory.create('beta.databasemanager');
```

DatabaseManager takes the place of `window.openDatabase()` in the HTML5 spec. Using a different top-level entrypoint allows us to coexist with native browser implementations of the HTML5 database API.

```
interface DatabaseManager {
  Database2 openDatabase(in DOMString name,
                         in DOMString version, 
                         in DOMString displayName,
                         in unsigned long estimatedSize);
  // Future: add removeDatabase(), listDatabases()
}
```


## Database2 ##

Instances of Database2 are returned by `DatabaseManager.openDatabase()`. Database2 implements the Database interface, as defined in [HTML5](http://www.whatwg.org/specs/web-apps/current-work/multipage/section-sql.html#database0). It also implements the following extensions, which allow synchronous Database access:

```
interface Database2 {
  void synchronousTransaction(SQLSynchronousTransactionCallback callback)
      throws SQLError;
}

interface SQLSynchronousTransactionCallback {
  void handleEvent(in SQLSynchronousTransaction transaction);
}

interface SQLSynchronousTransaction {
  SQLResultSet executeSql(in DOMString sqlStatement, in ObjectArray arguments)
      throws SQLError;
}
```


# Differences from the existing Gears database module #

The API approach differs significantly from the current Gears Database module by introducing an explicit [SQLTransaction](http://www.whatwg.org/specs/web-apps/current-work/multipage/section-sql.html#sqltransaction) interface. In fact, creating single, transactionless statements is not possible using this API. As a benefit, the management of transactions is completely abstracted out (`BEGIN`, `COMMIT`, `ROLLBACK`, etc. are prohibited in statements).

Because the API is asynchronous, the [SQLResultSet](http://www.whatwg.org/specs/web-apps/current-work/multipage/section-sql.html#sqlresultset) is an offline dataset, unlike the one in Database. The user accesses row data using an Object, with each field is mapped is a property (see [SQLResultSetRowList](http://www.whatwg.org/specs/web-apps/current-work/multipage/section-sql.html#sqlresultsetrowlist)).

Additionally, the API includes explicit database version management features in the form of [changeVersion](http://www.whatwg.org/specs/web-apps/current-work/multipage/section-sql.html#changeversion) and an [openDatabase](http://www.whatwg.org/specs/web-apps/current-work/multipage/section-sql.html#opendatabase) version parameter.

There are two types of error callbacks specified, one for transaction, and one for statement. Essentially, the statement error callbacks are used to detect statement sanitizing/execution failures, and the transaction error callbacks are responsible for handling versioning, transaction, and general statement failures.

Unlike the Database module, the spec prescribes limiting available storage for each database.


# Code Example #

```
var dbman = goolge.gears.factory.create('beta.databasemanager');
var db = dbman.open('pages', '0.0.1.0',
  'Collection of crawled pages', 3000000);

function renderPageRow(row) {
  // insert page row into a table
}

function reportError(source, message) {
  // report an error
}

function renderPages() {
  db.transaction(function(tx) {
    tx.executeSql('CREATE TABLE IF NOT EXISTS Pages(title TEXT, lastUpdated INTEGER)', 
      []);
    tx.executeSql('SELECT * FROM Pages', [], function(rs) {
      for(var i = 0; i < rs.rows.length; i++) {
        renderPageRow(rs.rows.item(i));
      }
    })
  })  
}

function insertPage(text, lastUpdated) {
  db.transaction(function(tx) {
    tx.executeSql('INSERT INTO Pages VALUES(?, ?)', [ text, lastUpdated ],
      function() {
        // no result returned, stub success callback
      },
      function(tx, error) {
        reportError('sql', error.message);
      });
  });
}
```

There is also a [working example](http://webkit.org/misc/DatabaseExample.html) (requires [WebKit nightly](http://nightly.webkit.org/)) on [webkit.org](http://webkit.org/).


# Technical Design #

All Javascript interfaces are implemented as `ModuleImplBaseClassVirtual` sub-classes and handle retrieving JS arguments, passing return parameters, and invoking callbacks:

```

// Allows JavaScript to create database objects. Could be extended
// in the future with removeDatabase() and listDatabases().
class Database2Manager : ModuleImplBaseClassVirtual {
 public:
  // IN: string name, string version, string displayName, int estimatedSize)
  // OUT: Database2
  void OpenDatabase(JsCallContext *cx) {
  // create a Database2 instance and pass name and version into it
  // displayName and estimatedSize not used by Gears
  }
};

```

This implementation uses a thread-safe static map to make sure that only _one transaction queue_ exists per database for a given origin. In the next iterations of development, an IE-only solution will be added to ensure the same across multiple processes. Having one transaction queue per database allows us to make sure that all transactions on a single database are serialized without having to use the expensive BEGIN IMMEDIATE transaction type.

The transaction queue map is a static member of `Database2` and is managed by static methods exposed by that class.

Transaction queue's life time is the same as the static member that holds it: once it is created, it sticks around.

When the `Database2` methods that create a transaction are invoked, they acquire the appropriate queue:

```

typedef ThreadSafeQueue<Database2Transaction> Database2TransactionQueue;

// Implements the HTML5 database interface, which allows the creation
// of transactions. We also have our own proprietary
// synchronousTransaction() method. This class also has a reference
// to a Database2Connection object which it shares with all transactions
// it creates.
class Database2 : ModuleImplBaseClassVirtual {
 public:
  // IN: function start_callback, function success_callback, 
  //     function failure_callback)
  // OUT: void
  void Transaction(JsCallContext *cx) {
    Database2Transaction *tx = CreateModule<Database2Transaction>();
    // populate callbacks
    // populate with threaded interpreter

    // ...
    QueueTransaction(tx);
  }

  void QueueTransaction(Database2Transaction *tx) {
    Database2TransactionQueue queue = GetQueue(/* name, origin */);  
    // returns true, if first iterm in queue
    bool first = queue->empty();
    // sketch only, consider making a single method to prevent race conditions
    queue->Push(tx);
    if (first) {
      // start executing transaction, if first
      tx->Start();
    }
  }

  // IN: string old version, string new version, optional function callback,
  //     optional function success, optional function failure
  // OUT: void
  void ChangeVersion(JsCallContext *cx) {
    Database2Transaction *tx = CreateModule<Database2Transaction>();
    // populate callbacks
    // populate with threaded interpreter
    // + populate version
    // ...
    QueueTransaction(tx, interpreter_);
  }

  // IN: function start_callback
  // OUT: void
  void SynchronousTransaction(JsCallContext *cx) {
    Database2Transaction *tx = CreateModule<Database2SyncTransaction>();
    // populate with interpreter (not threaded)
  
    // sync transaction never enters the queue, there's no need for it
    // but it still checks for pending transactions in the queue
    // to prevent deadlocks
    if (GetQueue(/* name, origin */)->empty()) {
      tx->Start();
    }
    else {
      // explicitly disallow nested transactions, thus one can't
      // start a synchronous transaction while another transaction is still open
      // set exception INVALID_STATE_ERR
    }
  }
  
  static Database2TransactionQueue *GetQueue(/* name, origin */) {
    // combine name, origin into one string
    // COMMENT(shess): Use pair<std::string16,std::string16> instead.
    static std::map<std::string16, Database2TransactionQueue*> map;
    // if map doesn't contain the queue, create the queue and insert into map
    // return queue
  }  

 private:
 
  // Shared reference to the connection used by all transactions from this
  // database instance. This is initialized during the first transaction.
  scoped_refptr<Database2Connection> connection_;
  
  Database2Interpreter interpreter_;
  scoped_refptr<Database2ThreadedInterpreter> threaded_interpreter_;

};

```

With the exception of opening a database, which has to occur on the main thread,  all database operations follow the same pattern: a database statement is executed on a separate thread, then an optional action on the main thread follows. This behavior is encapsulated by a [command](http://en.wikipedia.org/wiki/Command_pattern)/[interpreter](http://en.wikipedia.org/wiki/Interpreter_pattern) pattern.

There are two variants of the interpreter: `Database2ThreadedInterpreter` and `Database2Interpreter`.

The `Database2ThreadedInterpreter` first invokes `InBackground` method of the command on a separate thread, then, when the method completes and returns `true`, invokes `OnForeground` method back on the main thread. This is accomplished by passing messages using a `ThreadMessageQueue`.

The `Database2Interpreter` does not use threading: it simply invokes `InBackground` and then optionally `OnForeground`, synchronously and on the same thread:

```

class Database2Command : MessageData {
 // return false, if no need to report results
 virtual void InBackground(bool *needsForeground) = 0;
 virtual void OnForeground() = 0;
};

class Database2Interpreter : RefCounted {
  public:
    ~Database2Interpreter() {}

    virtual void RunCommand(Database2Command* command) {
      if (command) {
        bool needsForeground = true;
        command->InBackground(&needsForeground);
        if (needsForeground) {
          command->OnForeground();
          // delete command when we're done
        }
      }
    }
};
  
// lazily creates a background thread
// could be a thread pool
class Database2ThreadedInterpreter : Database2Interpreter {
 private:
  void ThreadEntryPoint() {
   // pump, pump, pump
  }
 public:
  ~Database2ThreadedInterpreter() {
   // shut down thread, if started
  }
   
  void RunCommand(Database2Command* item) {
   // get ThreadMessageQueue instance
   // post item message
   // on the background thread, invoke InBackground, then
   // if returned true,
   // post item back to the foreground thread,
   // when the message is received, invoke OnForeground
   // delete command when we're done
  }
};

```

It is important to note that `RunCommand` method should be callable from any thread with the same result: main thread is the one in which `Database2ThreadedInterpreter` was instantiated.

Each concrete implementation of `Database2Command` acts as operation context, holding refs to all work objects and results. There are several implementations of `Database2Command`, each for one database operation and each handling the corresponding fallback/error cases:

  * `Database2BeginCommand`
  * `Database2AsyncExecuteCommand`
  * `Database2SyncExecuteCommand`
  * `Database2CommitCommand`
  * `Database2RollbackCommand`

Here's the rough sketch of `*ExecuteCommand`s:

```

class Database2AsyncExecuteCommand : Database2Command {
 public:
  Database2AsyncExecuteCommand(Database2Transacton *tx,
                               Database2Connection *conn,
                               Database2Statement *stmt) : tx_(tx), stmt_(stmt),
                                 conn_(conn) {}
  
  void InBackground(bool *needsForeground) {
    conn_->Execute(stmt, this);
    // if success and no success callback:
      tx->ExecuteNextStatement();
      *needsForeground = false;
      // return
    // otherwise, delegate to foreground thread (by default)
  }
  
  void OnForeground() {
    // create JS objects from collected rows
    stmt_->InvokeCallback(/* collected rows */);
    // if statement succeeded and callback failed, queue rollback op
    // else if statement failed and there is no callback, or callback did
    // not return false, queue rollback op
  }
  
  virtual void HandleRow(/* row data here */) {
   // collect the row into Database2Statement
  }
  
 private:
  int sql_error_code_;
  std::string16 error_message_;

  Database2Transaction *tx_;
  Database2Connection *conn_;
  Database2Statement *stmt_;
};

class Database2SyncExecuteCommand : Database2Command {
  public:
    Database2SyncExecuteCommand(JsContext* cx /* whatever else is necessary */);
  
    void InBackground(bool *needsForeground) {
      conn_->Execute(stmt, this);
    }
    
    // functor for Execute operation
    void operator()(/* collected_row */) {
      // create JS object, if not created
      // add new row
    }
    
    void OnForeground() {
      // if error occured, use JsContext to set exception
      // otherwise, use JsContext to set the return value to the JS object,
      // generated by the functor
    }
}

```

The implementation of the `SQLTransaction` interface holds a queue of statements and provides facilities to invoke transaction callbacks:

```

class Database2Transaction : ModuleImplBaseClassVirtual {
 public:
  void Init(bool async /*, and other args */);
  
  void Start() {
    // queue operation to begin transaction
    interpreter()->RunCommmand(new Database2BeginCommand(this, database()));
  }

  void InvokeCallback();

  void InvokeErrorCallback();
  
  // IN: string sql, optional object[] arguments, optional function on_success,
  //     optional function on_failure
  // OUT: Database2ResultSet
  void ExecuteSql(JsCallContext *cx) {
    if (!is_open) {
      // throw exception saying the transaction is closed
    }
    // create Database2Statement with sql, arguments, and callbacks
    // add to statement queue
    // if first item in the queue, invoke ExecuteNextStatement
    
    // ideally, if the queue is empty prior to this call, this should be done
    // without pushing/popping the statement
  }

  void ExecuteNextStatement(JsCallContext *cx) {
    // pop statement from the end of the queue
    // if no more statements,
    interpreter()->RunCommand(new Database2CommitCommand(this));
    // otherwise
    if (async_) {
      interpreter()->RunCommand(new Database2AsyncExecuteCommand(this, conn,
        stmt));
    }
    else {
      interperter()->RunCommand(new Database2SyncExecuteCommand(cx, conn));
    }
  }

  Database2Connection database() const;
 
  // if async_ is true, returns Database2ThreadedInterpreter, 
  // or Database2Interpreter otherwise
  Database2Interpreter interpreter();
    
 private:
  std::string16 old_version_;
  std::string16 new_version_;

  scoped_ptr<JsRootedCallback> start_callback_;
  scoped_ptr<JsRootedCallback> success_callback_;
  scoped_ptr<JsRootedCallback> failure_callback_;

  scoped_refptr<Database2Connection> connection_;
  scoped_refptr<Database2Interpreter> interpreter_;
  
  ThreadSafeQueue<Database2Statement> statement_queue_;

  bool is_open_;
  bool async_;
};

```

Thus, there are two explicit, thread-safe queues: a transaction queue and a statement queue. Each `Database2Transaction` has a statement queue. The queues are concrete implementations of a `ThreadSafeQueue` template:

```

// thread-safe queue, used to queue up both transactions and statements
template <class T>
class ThreadSafeQueue {
 public:
  void Push(T *t) {
   // lock
   // add t to internal queue
   // unlock
  }
  
  // remove from queue
  T *Pop();
  
  // returns true if the internal queue is empty
  bool empty();
};

```

Database work is encapsulated in the `Database2Connection`. Here is a very rough sketch:

```
// Encapsulates database operations, opens and closes database connection
class Database2Connection : RefCounted {
 public:
  // lazily initialized
  Database2Connection(std::string16 name) : name_(name) {}
  
  ~Database2Connection() {
    // close connection
  }
  
  bool OpenAndVerifyVersion(std::string16 databaseVersion) {
    // read expected_version (user_version value)
    // read version from Permissions.db
    // if databaseVersion is not an empty value or null,
      // if databaseVersion matches version
        // return true
      // otherwise,
        // set error to INVALID_STATE_ERR exception value
    // return false
  }
  
  bool Execute(Database2RowHandlerInterface *row_handler) {
    if (bogus_version_) {
     // set error code to "version mismatch" (error code 2)
    }
    // prepare
    // step, for each row, call row_handler->HandleRow(..);
     // if error, set error code and message, return false;
    // return true upon success
  }
 
  bool Begin() {
    // execute BEGIN    
    // if error, set error code and message, return false
    // read actual_version, if doesn't match expected_version_, 
      // set bogus_version_ flag
    // return true upon success
  }
 
  void Rollback() {
    // execute ROLLBACK
    // don't remember or handle errors
  }

  bool Commit() {
    // execute COMMIT
    // if error, set error code and message, return false
    // return true upon success
  }
  
  int error_code();
  std::string16 error_message();

 private:
  bool GetHandle() {
    // opend database if not already open,
    // used by all operations to obtain the handle
  }
  
  bool bogus_version_;
  int expected_version_;

  sqlite3 *handle_;
  std::string16 name_;
};

class Database2RowHandlerInterface {
  public:
    virtual void HandleRow(/* row */) = 0;
}

```

In order to handle synchronous transactions, `Database2Transaction` has an _async_ flag (set during initialization), which changes the behavior as follows:

  * `thread` method returns `Database2ThreadedInterpreter` if _true_ or `Database2Interpreter` otherwise.
  * `Database2AsyncExecuteCommand` is used if _true_ or `Database2SyncCommand` otherwise.

A statement in transaction is represented by `Database2Statement`:

```

class Database2Statement {
 private:
  std::string16 sql_;
  JsParamToSend *args_;
  scoped_ptr<JsRootedCallback> success_callback_;
  scoped_ptr<JsRootedcallback> failure_callback_;
  JsParamToSend *rows_;

 public:
  InvokeSuccessCallback();
  InvokeFailureCallback();
};

```

Version values are part of `Database2Transaction`, and remain uninitialized when `Database2::Transaction` method is invoked. If they are initialized (this occurs when `Database2::ChangeVersion` is called):

  * `Database2BeginCommand::InBackground` checks for `bogus_version_` and fails, if the flag is set
  * `Database2CommitCommand::InBackground` updates version values after successful commit

## Nested Transactions ##

The spec has no concept of nested transactions. Since all transactions are asynchronous, they are just queued naturally by the locking behavior of the database. This is fine for asynchronous transactions. For synchronous transactions, though, it leads to deadlock:

```
db.synchronousTransaction(function(tx) {
  db.synchronousTransaction(function(tx) {
  });
});
```

Neither transaction will ever complete because they are both waiting on each other. Note that this occurs even if the parent transaction is asynchronous. For this reason, synchronousTransaction() throws if any other transaction is currently running or pending.


## Connection State ##

Many database implementations have per-connection state that should be shared between all statements executing against the connection. For example, SQLite has the last\_insert\_rowid() function. Connection state should be shared between all transactions created from the same GearsDatabase2 object.


## Versioning ##

Version information is tracked using the [user\_version](http://www.sqlite.org/pragma.html#version) pragma, retrieval of which is a cheap and safe operation across threads or processes.

The actual version values are stored in a central table in _Permissions.db_:
```
CREATE TABLE DatabaseVersions(
  name TEXT,
  origin TEXT,
  version TEXT,
  PRIMARY KEY(name, origin)
)
```

### openDatabase ###

After database is opened and if `databaseVersion` is specified, read `expected_version`:
```
PRAGMA user_version
```

Then, read `version` from _Permissions.db_
```
SELECT version from DatabaseVersions WHERE name = ? 
  AND origin = ?
```

If `databaseVersion` and `version` don't match, raise an INVALID\_STATE\_ERR
exception.

### transaction ###

After transaction begins, read `actual_version` using:
```
PRAGMA user_version
```

if the not the same as `expected_version`, set `bogus_version` flag. This will cause all statements to fail with error code 2.

### changeVersion ###

In _preflight_, if `bogus_version` flag is set, fail transaction.

In _postflight_, update _Permissions.db_:
```
REPLACE INTO DatabaseVersions(name, origin, version) VALUES(?,?,?)
```

Then, bump `expected_version` by 1 and update pragma accordingly:
```
PRAGMA user_version = ?
```