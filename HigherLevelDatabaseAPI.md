# Problem #

This proposal seeks to address two issues that affect many people using the current database API:

  * The ResultSet API is too fine grained. It's hard to remember all the APIs to retrieve individual fields.
  * It's easy to forget to close a ResultSet. This can result in locked databases. For more information, see: http://code.google.com/p/google-gears/issues/detail?id=40.


# Solution #

This can be solved by adding two new APIs:

```
class ResultSet {
  readonly property JsObject currentRow;
}

class Database {
  JsArray executeToArray(String sql, optional JsArray params);
}
```


Example usage:

```
var resultSet = db.execute("SELECT Name, NumCLs FROM GearsTeam");
while (resultSet.currentRow) {
  alert(resultSet.currentRow.Name + " has submitted " +
        resultSet.currentRow.NumCLs + " change lists!");
  resultset.next();
}

var rows = db.executeToArray("SELECT Name, NumCLs FROM GearsTeam");
for (var i = 0; i < rows.length; i++) {
  alert(row.Name + " has submitted " + row.NumCLs + " change lists!");
}
```


# Details #

  * _currentRow_ is a property, not a method.
    * This makes it a little more convenient because you don't have to declare a local variable if you don't need to.
    * We should cache it in the resultset after the first access.
    * It also doubles as a way to end the loop because it is null when the resultset is empty.


# Alternatives #

  * Put something like _toArray()_ on the resultset object.
    * One downside of this is that toArray() is destructive -- it closes the resultset.
  * Call _executeToArray()_ something like _select()_ instead.
    * This is cute, but it's a little weird that you can actually pass an INSERT statement to the select() method, or a SELECT statement to the execute() method.