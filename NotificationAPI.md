# Summary #

The Gears Notificatations APIs are responsible for managing notifications from web applications, and showing them on the user's desktop. Incoming notifications are not displayed when the user is considered not available for the notifications, i.e. the user is away, idle or in full-screen presentation mode.

Notifications are displayed in top-most balloon-styled windows. On Windows, balloons always appear starting in the lower right corner of the primary screen's working area and go upward. On Macs, they start at the upper right corner and proceed downward. The balloons are constrained to ensure that the balloons will not take up more than 75% of the height of the working area.

# Example #

```
<script type="text/javascript" src="gears_init.js"></script>
<script type="text/javascript">
var desktop = google.gears.factory.create('beta.desktop')
var notification = desktop.createNotification();
notification.title = Barbecue on Saturday';
notification.icon = 'http://mail.google.com/mail.gif';
notification.subtitle = 'Thu Mar 27 1:25pm - 2:25pm';
notification.description = 'Hey everyone, looks like great weather this ' +
    'weekend so I thought we might get together';
notification.displayAtTime = new Date(2008, 5, 27, 14, 0, 0);
notification.displayUntilTime = new Date(2008, 5, 27, 14, 0, 15);
notification.addAction('View', 'http://mail.google.com/view?id=...');
desktop.showNotification(notification);
</script>
```

![http://gears.team.jian.googlepages.com/mail_notif.jpg](http://gears.team.jian.googlepages.com/mail_notif.jpg)

# Details #

## Notifier functionality in Desktop class ##
(created by calling `google.gears.factory.create('beta.desktop')`)

```
// Creates a notification object.
Notification createNotification()

// Shows a notification. If the notification with the same ID is on display, 
// it will get updated.
// If the notification can be shown successfully, the notification ID will be
// returned. Otherwise, an empty string is returned.
string showNotification(Notification notification)

// Removes the notification with the given ID. If the notification is on display,
// it will get dismissed immediately.
// If the notification with the specified ID can be found, 1 will be returned.
// Otherwise, 0 will be returned.
int removeNotification(string id)
```

## Notification class ##

```
/// Recommended

// Specifies the title of the notification.
readwrite attribute string               title

// Specifies the URL of the icon to display.
readwrite attribute string               icon

/// Optional

// A unique ID for the notification. It can be used for duplicate detection.
// If this is not set, a unique ID will be generated.
readwrite attribute string               id

// Specifies the additional subtitle. For example, this could be used to
// display the appointment time for the calendar notification.
readwrite attribute string               subtitle

// Specifies the main content of the notification. For example, email 
// notifications could put snippet in this field.
readwrite attribute string               description

// This is the time when the notification is meant to be displayed. If this is
// not provided, the notification will be displayed immediately.
readwrite attribute Date                 displayAtTime

// This is the time when the notification is meant to go away. If this is not
// set, the notification just get displayed and goes away after the default
// timeout. If it is set, then once displayed, the notification will remain on
// screen until the user dismisses it or the desired time has arrived.
readwrite attribute Date                 displayUntilTime

// Adds an action for this notification.
// The parameter text specifies the text to display for the action.
// The parameter url either denotes the URL to launch in the browser that (first)
// sent the notification. If that browser is no longer running, then the
// default browser is launched. 
// If the url starts with "http://gears.google.com/notifier/actions", then
// it has a special behavior.
// Currently, the only defined special behavior is
// "http://gears.google.com/notifier/actions/snooze?duration=X", which means
// to snooze for X seconds.
void addAction(string text, string url)
```