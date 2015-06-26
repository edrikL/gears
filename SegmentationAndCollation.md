# Introduction #

Fts and Gears in general need to expose functionality to let people write good offline i18n web apps.


# Details #

Collation is the part of sorting which indicates what order values should go in.  Something like `memcmp()` would be binary collation.  ASCII collation is very easy, for either case-sensitive or case-insensitive.  Various languages additionally need to collate accented characters correctly.  If you used binary collation, then 'o' and 'ô' might be widely separated in the output.  Lastly, some languages use Unicode characters which are not ordered at all lexographically.

See also http://en.wikipedia.org/wiki/Collation and http://www.icu-project.org/userguide/Collate_Intro.html .


Segmentation is the process of breaking text up into words.  In English, you can usually get by using spaces and punctuation, though things like contractions can break this assumption.  Some languages, such as Japanese, do not have explicit word boundaries of this sort, and the grouping of characters into words is dependent on context.

See also http://en.wikipedia.org/wiki/Text_segmentation and http://www.icu-project.org/userguide/boundaryAnalysis.html .


# Proposal 1 #

Build i18n segmentation and collation support into SQLite, and expose it with new methods on Gears.Database.  For segmentation in SQLite:

```
CREATE VIRTUAL TABLE t USING fts2(l LOCALE, content);
INSERT INTO t (l, content) VALUES ('en_us', 'This is a test');
```

`l` would be exposed as a hidden column, sort of like rowid, where you don't see it unless you refer to it directly.  On Gears.Database:

```
  db.split(s, l);   // Split using the given locale.
```

If you inserted `db.split(s, l).join(' ')` into an fts table, you'd get the same exact hits as if you inserted `s` (though it might not result in an identical string, of course).  The `split()` function may allow for a default locale, which should probably be English.  It is tempting to make it the browser locale, but since the segmentation is baked into the fts index, that may cause future problems.

For collation, the SQLite would look like:

```
SELECT x FROM y ORDER BY x COLLATE en_us;

CREATE TABLE t(
  a TEXT COLLATE en_us
);
SELECT a FROM t ORDER BY a;  -- collates with en_us.
```

**TBD:** I bet the locale cannot be a bind parameter.

Gears.Database additionally adds:

```
var a1 = db.sort(ary);          // sort using global ordering
var a2 = db.sort(ary, 'en_us'); // sort using named SQLite collator
```

This function would be literally the same as:

```
  db.execute('CREATE TEMPORARY TABLE t (c TEXT COLLATE my_collator)');
  for (var ii = 0; ii < a.length; ++ii) {
    db.execute('INSERT INTO t VALUES (?)', [a[i]]);
  }
  var r = new Array(a.length);
  var rs = db.execute('SELECT c FROM t ORDER BY c');
  while (rs.isValidRow()) {
    r.push(rs.field(0));
  }
  rs.close();
  db.execute('DROP TEMPORARY TABLE t');
  return r;
```

The main difference is that db.sort() would call the collator
directly, and would use whatever refcounting tricks JavaScript allows
to make the output array efficient.

# Proposal 1a #

Just like the first proposal, but modifying the Gears.Database addition like:

```
var a = ary.sort(db.sorter('en_us'));
```

This may be more appropriate for JavaScript, but I'm a little nervous about whether it's theoretically pure but practically hard to use.  The first proposal is pretty easy to follow.

# Proposal 2 #

Expose a new Gears.Locale API.  It would be used like:

```
var l = google.gears.factor.create('locale', '1.0');
l.open('en_us');
var a1 = l.split('This is a test');  // Segmentation.
var a2 = l.sort(a1);                 // Collation.
```

The Locale object would become an obvious place to hang other features which ICU includes, like date and number formatting.

This would become the canonical source for such things, and would be referenced by the Gears.Database API.  At this time, I hope we can avoid requiring explicit loading of locale information in SQLite, but if we did, it could be exposed like:

```
var l = google.gears.factor.create('locale', '1.0');
l.open('en_us');
db.addLocale(l);
```

# Proposal 2a #

Don't expose the new API, but do structure the internals so that locale is a first-class object, rather than embedding it too deeply into SQLite.  Then we can expose things later as appropriate.

# To Be Determined #

Do we require a way to apply a specific locale as a hint to the query?  Possibly.  If so, we could provide a MATCH alternative which also adds the locale.  This would _not_ imply a localte MATCH, though, that should be done using standard SQLite WHERE syntax.

ICU appears to expose collators that are locale-specific, in addition to a global collator.  _I previously implied that there was just the global version._  We should expose all of them, but I expect that the global one will be the most used.