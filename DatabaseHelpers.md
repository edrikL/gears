d#summary A suggested set of helpers for the Database API.

# Introduction #

The Gears Database API is sweet and simple.  Almost **too** simple.  I think an issue is scaling down.  To correctly fetch a very large set of results, the minimum footprint is about this long:

```
  var rs = do.execute('SELECT v FROM t WHERE k > ?', ['17']);
  try {
    while (rs.isValidRow()) {
      // Do stuff with rs.field(0) here.
      rs.next();
    }
  } finally {
    rs.close();
  }
```

while to correctly fetch a very small set of results, the minimum footprint is hardly shorter:

```
  var rs = do.execute('SELECT COALESCE(COUNT(v),0) FROM t WHERE k > ?', ['17']);
  try {
    // rs.field(0) be the count.
  } finally {
    rs.close();
  }
```

Of course, if you're willing to play fast and loose, you could omit the exception-handling, and hope that field() never throws an exception.  We shouldn't encourage people to play fast and loose.

See also [Bug111](http://code.google.com/p/google-gears/issues/detail?id=111)

# Details #

There are a set of common idioms which people are likely to write wrappers to accomplish.  Maybe we could just release the wrappers ourselves?  I think the attached files handle 99% of what people are likely to do with Database, and makes many of the use-cases (like selecting a single result) trivial.  _Sorry, I can't find a place to attach things, see [Bug111](http://code.google.com/p/google-gears/issues/detail?id=111)._  The suggested functions would allow usage like:

```
// Like execute(), but assumes no results returned and closes the
// resultset.  With an API change, could have do() return the number
// of rows changed.
db.do('CREATE TABLE mytable (val TEXT)');

// Wraps the passed function in a transaction, handling rollback on
// exception.
db.transaction(function(db) {
  db.do('INSERT INTO mytable VALUES (?)', ['hello']);
  db.do('INSERT INTO mytable VALUES (?)', ['there']);
  db.do('INSERT INTO mytable VALUES (?)', ['buddy']);
});

// Transactions allow fake nesting, any broken interior transaction
// rolls everything back.
db.transaction(function(db) {
  db.do('INSERT INTO mytable VALUES (?)', ['hello']);
  db.do('INSERT INTO mytable VALUES (?)', ['there']);
  db.do('INSERT INTO mytable VALUES (?)', ['buddy']);
  db.transaction(function(db) {
    db.do('INSERT INTO mytable VALUES (?)', ['die']);
    db.rollback();
  });
});
// The transaction was just a dream at this point.


// Returns the value 3, handles closing the resultset.
db.selectSingle('SELECT COUNT(*) FROM mytable');

// Returns the array [ 'hello', 'there', 'buddy'], closes the
// resultset.
db.selectColumn('SELECT val FROM mytable');

// Returns the map { "rowid": 2, "val": "there" }.
db.selectRow('SELECT rowid, val FROM mytable WHERE val = ?', ['there']);

// Calls the function with a map for each row in the results,
// returning an array of the return values, in this case [5, 5, 5].
db.select('SELECT rowid, val FROM mytable', [], function(map){
  return map['val'].length;
});

// Returns an array of maps.
db.selectAll('SELECT rowid, val FROM mytable');

// Returns an array of arrays.
db.selectAllAsArrays('SELECT rowid, val FROM mytable');

//-----------
// Possibly weirdo items below.

// Alternative for loading the data.  Runs the query for each element
// in vals.
var vals = [['hello'], ['there'], ['buddy']];
db.doArray('INSERT INTO mytable VALUES (?)', vals);

// Run the query for each element a function returns.
db.doForEach('INSERT INTO mytable VALUES (?)', function() {
  return vals.shift();
});

// Cute way to copy a table, ina  transaction.
db.select('select rowid, val from mytable', [], function(map){
  db.do('insert into othertable (rowid, val) VALUES (?, ?)',
        [rowMap['rowid'], rowMap['Phrase']]);
});
```