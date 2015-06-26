# Factory Module #

The Factory module is used to instantiate all other Google Gears objects.

Use [gears\_init.js](GearsInitJS.md) in your application to initialize Gears and get access to a Factory object.


# Example #

```
<!-- gears_init.js defines google.gears.factory. -->
<script type='text/javascript' src='gears_init.js'></script>
<script type='text/javascript'>
// Check whether Google Gears is installed.
if (window.google && google.gears) {
  // Instantiate Gears objects
  var db = google.gears.factory.create('beta.database');
  db.open();
}
</script>
```

# Factory Class #

## Methods ##

  * [create()](FactoryCreateMethod.md)
  * [getBuildInfo()](FactoryGetBuildInfoMethod.md)
  * [getPermission()](FactoryGetPermissionMethod.md)

## Properties ##

| Property | Type | Description |
|:---------|:-----|:------------|
| `version` | readonly string | Returns the version of Gears installed, as a string of the form Major.Minor.Build.Patch (e.g., `'0.10.2.0'`). |
| `hasPermission` | readonly boolean | Returns true if the site already has permission to use Gears. |