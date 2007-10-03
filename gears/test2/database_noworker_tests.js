var db = google.gears.factory.create('beta.database', '1.0');
db.open('database_noworker_tests');

// On IE, the specialEmpty value below will be a NULL BSTR.  This is
// supposed to be treated the same as the empty string, but some code
// dereferenced it.
function testSpecialEmptyString() {
  // This one cannot run in a worker
  if (typeof document == 'undefined') {
    return;
  }

  var elm = document.createElement('textarea');
  document.body.appendChild(elm);
  var specialEmpty = elm.value;

  assertError(function() {
    // NULL-dereference crash.
    db.execute(specialEmpty).close();
  });

  // Sanity check for same bug in bind parameters (there wasn't
  // one).
  db.execute('select ?', [specialEmpty]).close();

  handleResult(db.execute('select ? as field', ['value']), function(rs) {
    assertError(function() {
      // NULL-dereference crash.
      rs.fieldByName(specialEmpty);
    });
  });
}
