## Introduction ##

The purpose of this module is to provide secure access to files on the client machine. The  FileSystem module has now been merged with the Desktop module. Therefore this document describes the FileSystem functionality in the Desktop module.

## NOTE ##

We should do something API wise that fits as close as possible with 

&lt;input type="file"&gt;

 and the webforms file picker extensions.

See also:
http://www.whatwg.org/specs/web-forms/current-work/#upload
http://groups.google.com/group/google-gears-eng/browse_thread/thread/f73cb640a1c845/8026291efe8fc8a5

The one remaining issue preventing us from designing this is that we also have some desire to implement file _writing_. It's unclear how this relates and what the API would look like.

## Features ##

Multiple files can be provided by the user.
  * The API then provides access to these files.
Once the files are available, potential processing options are:
  * Compress files.
  * Send files to a server.
  * Capture files to the local resource store.


## Security ##

Access to the user's file system can only be granted by the user.
  * The permission is granted via the use of a file picker dialog.
A malicious application will NOT be able to access arbitrary files on the user's system.
  * Do not allow random access to the user's file system.
    * filesystem.open('/etc/shadow');
  * Do not allow access above the specified location in the user's file system.
    * filesystem.changedir('../../etc/shadow');
Do not allow a malicious application to display a file open dialog in the wrong context.
  * The minimum behavior required is that of alert() in JavaScript. If the tab is inactive when it displays a new window it must switch to be the active tab. Firefox exhibits this behavior but IE7 acts differently. In IE7 the inactive tab flashes and when the tab is selected then the window is displayed.


## Trade-offs ##

Selecting directories
  * Directory selection was rejected because it could not be done in the same dialog using native file pickers (win32, gtk, carbon). The native file pickers do not support the concept of selecting files and directories simultaneously. It is possible to select directories in a native file picker. However, this would require gears to have multiple functions for retrieving local files. This would have resulted in desktop.getLocalFiles() and desktop.getLocalDirectory().
  * Recursive access to directories was considered and rejected. It poses a security problem where the user is unlikely to be aware what they are actually sharing. If directory access were to be added it was decided that it should only be one level deep.
    * For example the user selected "/home/foo/bar/" and it contains files "baz.jpg, bar.gif and foo.png". It also contains the directories "foo, bar and baz" containing many more files within. The API would only return the files "baz.jpg, bar.gif and foo.pnf" from "/home/foo/bar/". It would not return any files from the directories contained within "/home/foo/bar/".



## Use cases ##

Name: input files

Description: a user provides details of files to make available for further processing

Author: Chris De Vries

Date: 11/01/2008

Steps:
  1. The user indicates files they wish to make available to the system.
  1. The files are made available for iteration via the API.


## JavaScript Interfaces ##

```

interface Desktop {
  //
  // ... other apis ...
  //

  // Displays a file chooser to the user, from which they can select files on the local
  // disk.  The callback is invoked with an array of the files which were selected.
  void openFiles(OpenFilesCallback callback, optional OpenFilesOptions options);
}

void OpenFilesCallback(File[] files);

interface File {
  // The name of the file, excluding the path.
  readonly attribute string name;

  // The contents of the file.
  readonly attribute Blob blob;
}

interface OpenFileOptions {
  // By default, the user may select multiple files.  If true, the user is limited to
  // selecting only one file.
  bool singleFile;

  // By default, all files on the local disk are shown as selectable.  If a filter is
  // provided, only files matching the filter are shown.  The user can turn the filter
  // off, so be aware that selected files may not match the filter.  The filter is an
  // array of Internet content types and file extensions.
  // Example: ['text/html', '.txt', 'image/jpeg']
  string[] filter;
}

```


## C++ Internal Interfaces ##

```
// Constructs a file dialog according to mode.
// Caller is responsible for freeing the returned memory.
// Returns: null pointer on failure
// Parameters:
//   mode - in - the type of dialog to construct
FileDialog* NewFileDialog(const FileDialog::Mode mode,
                          const ModuleImplBaseClass& module);

// This interface will be implemented for every supported operating system.
class FileDialog {
  // Used for constructing dialogs.
  // This is intended to allow for different types of file pickers in future.
  // For example, FILES_AND_DIRECTORIES, for a custom dialog that allows the
  // user to select files and directories at the same time.
  enum Mode {
    MULTIPLE_FILES  // one or more files
  };

  struct Filter {
    // The text the user sees. For example "Images".
    std::string16 description;

    // A semi-colon separated list of filters. For example "*.jpg;*.gif;*.png".
    std::string16 filter;
  };

  // Displays a file dialog.
  // Returns: false on invalid input or failure
  // Parameters:
  //   filter - in, optional - An array consisting of pairs of strings.
  //     Every first string is a description of a filter.
  //     Every second string is a semi-colon separated list of filters.
  //     Example "Images" and "*.jpg;*.gif;*.png".
  //
  //   files - out - An array of files selected by the user.
  //     If the user canceled the dialog this will be an empty array.
  //     A new array is constructed and placed here.
  //     Caller is responsible for freeing returned memory.
  //
  //   error - out - The error message if the function returned false.
  //
  virtual bool OpenDialog(const std::vector<Filter>& filters,
                          std::vector<std::string16>* selected_files,
                          std::string16* error) = 0;
}
```


## Code Example ##

```
var desktop = google.gears.factory.create('beta.desktop');
var files = desktop.getLocalFiles(['Images (*.jpg, *.gif)', '*.jpg;*.gif', 'All files', '*']);
for (var i = 0; i < files.length; ++i) {
  var img = document.createElement("img");
  img.src = files[i];
  document.body.appendChild(img);
}
```