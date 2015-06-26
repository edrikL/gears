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

```
Object create(string className, string classVersion);
string getBuildInfo();
boolean getPermission(siteName, imageUrl, extraMessage);
readonly attribute boolean hasPermission;
readonly attribute string version;
```


---


## Methods ##

### Object create(string className, string classVersion) ###

Creates a Gears object of the given class.

#### Parameters ####

  * `className` - Name of the object to create.
  * `classVersion` - Deprecated. There is no longer any need to pass this parameter. The only allowed value is `'1.0'`. (To see if the machine has the minimum version of Gears you require, use `factory.version` instead.)

#### Return value ####

The newly created object.

#### Details ####

An exception is thrown if the given className is not recognized.

The supported class names are:

| **className** | **Google Gears class created** |
|:--------------|:-------------------------------|
| beta.database | Database                       |
| beta.desktop  | Desktop                        |
| beta.httprequest | HttpRequest                    |
| beta.localserver | LocalServer                    |
| beta.timer    | Timer                          |
| beta.workerpool | WorkerPool                     |

### string getBuildInfo() ###

Returns a description of the build of Gears installed.

#### Parameters ####

None

#### Return value ####

The build description string.

#### Details ####

This string is purely informational. Application developers should not rely on the format of data returned. The contents and layout may change over time.

### boolean getPermission(siteName, imageUrl, extraMessage) ###

Lets a site manually trigger the Gears security dialog, optionally with UI customizations.

#### Parameters ####
  * `siteName` - Friendly name of the site requesting permission.
  * `imageUrl` - URL of a .png file to display in the dialog.
  * `extraMessage` -  Site-specific text to display to users in the security dialog.

#### Return Value ####

Indicates whether the site has permission to use Gears.

#### Details ####

It is always safe to call this function; it will return immediately if the site already has permission to use Gears.  All arguments are required, but an empty string can be passed for any value a site does not wish to use.


---


## Properties ##

| Property | Type | Description |
|:---------|:-----|:------------|
| `version` | readonly string | Returns the version of Gears installed, as a string of the form Major.Minor.Build.Patch (e.g., `'0.10.2.0'`). |
| `hasPermission` | readonly boolean | Returns true if the site already has permission to use Gears. |