# gears\_init.js #

The [gears\_init.js](http://google-gears.googlecode.com/svn/trunk/gears/sdk/gears_init.js) file is a short JavaScript library that initializes Google Gears in the proper way for each browser.

**[Download gears\_init.js](http://google-gears.googlecode.com/svn/trunk/gears/sdk/gears_init.js)**

To use it, copy the file into your web application and then add a reference to it in each page you want to use Gears on. Then, access Gears through the `google.gears.factory` object.

```
<script language="gears_init.js"></script>
<script>
  var database = google.gears.factory.create("beta.database");
  var workerPool = google.gears.factory.create("beta.workerpool");
  // .. etc ...
</script>
```

# Important #

Always use the logic from `gears_init.js` to initialize Gears. Do not circumvent it by going directly to the underlying Gears plugin objects. Future versions of Gears may change the exact mechanism of plugging into the browser, and `gears_init.js` is designed to deal with this situation gracefully. Other ways of accessing Gears may stop working at any time.