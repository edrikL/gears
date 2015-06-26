# PROBLEM #

We wish to add a Google Gears API to let web sites:
  * Provide a better setup experience than today's security dialog.
  * Add shortcuts on the desktop for easier offline access to apps.

The API will **not** introduce a new security model.  Google Gears will continue to use a same-origin security model.


# SOLUTION #

We can meet our goals by adding a single method to the Gears `Factory` class:

```
bool registerApp(name, launchUrl, iconUrl, installMessage)
// Arguments:
//   name - the user-visible name for this web app
//   launchUrl - the URL to load when the user launches this app
//   iconUrl - the URL of a .ico file containing the app's icon
//   installMessage - a custom message to display during the app-install process
// Returns true if the app was successfully registered.
```

This method will trigger a "wizard"-style UI flow:
  * The UI will include a step for granting Gears security permissions, unless the app's origin already has permission to use Gears.
  * The UI will then let users optionally add shortcuts to the desktop and start menu.


# DETAILS #

**Q: What are the details of the UI flow?**

Othman has UI mocks, I believe?  We should post them here.

**Q: How will the Google Gears Settings dialog change?**

The Settings dialog should let users easily launch registered application, or remove them from the list.

Because most users are more interested in applications than security origins, we should consider emphasizing apps, and de-emphasizing the security-origin settings as an advanced setting.

Removing an application could prompt for whether to remove the associated security origin, if no other apps are installed for that origin.

**Q: What happens if the application has previously called `registerApp()`?**

If the user previously agreed to register the app, the method returns immediately, with a value of `true`.

If the user previously chose to permanently disallow the app (or security origin), the method also returns immediately, but with a value of `false`.

**Q: Why is this a method on `GearsFactory`?**

Mostly for convenience; `google.gears` is not a C++ object today.  We have briefly discussed moving in that direction, to help remove the need for "gears\_init.js", but that work is not currently scheduled.

**Q: Could the `installMessage` parameter be used to trick users?**

We should structure the install flow to help users understand what they are doing.  Perhaps a Gears-controlled message should be shown before the app-defined message.

**Q: Why use .ico as the image format?**

There are several reasons for only supporting .ico files in the initial version:
  * Windows requires the .ico format for desktop shortcuts.
  * Web developers have experience using .ico for favicons. (Internet Explorer only supports .ico favicons.)
  * .ico files also work in the other places we need the app icon (e.g. in the <img> tags in our dialogs).<br>
<ul><li>.ico is a simple format that can be converted to other formats easily. Going the other direction (from arbitrary .jpg or .png files to .ico) would be harder.<br>
</li><li>.ico supports multiple resolutions in a single file, meaning developers can customize their small 16x16 icon (shown in the Google Gears Settings dialog) separately from their large icon (shown in the wizard-style UI).</li></ul>

<b>Q: What shortcuts will be supported on non-Windows platforms?</b>

We need more information here.  What shortcuts make sense for Mac users?  And for Linux users?