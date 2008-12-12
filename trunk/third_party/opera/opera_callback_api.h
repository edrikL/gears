// This file is used by both Opera and the Gears code as a common API
// between the two code bases.

#ifndef OPERA_CALLBACK_API_H__
#define OPERA_CALLBACK_API_H__

// This ifdef is introduced to be able to compile this file without changes
// in both Opera and Gears. EXTERNAL_GEARS_SUPPORT will be defined in Opera.
#ifdef EXTERNAL_GEARS_SUPPORT
#include "modules/ns4plugins/src/plug-inc/npapi.h"  // for NPP for Opera
#else
#include "third_party/npapi/npapi.h"  // for NPP for Gears
#endif

struct NPObject;
struct OperaCallbacks;
class OperaGearsApiInterface;
class OperaWorkerThreadInterface;
class OperaLocalServerInterface;

// Struct containing function pointers used to hold callback functions
// from Opera to Gears.
struct OperaCallbacks {
  typedef void (*InitializeFunc)(OperaGearsApiInterface *, OperaCallbacks *);
  typedef OperaWorkerThreadInterface* (*CreateWorkerFunc)();
  typedef NPObject *(*CloneObjectFunc)(NPObject *object);
  typedef void (*OpenSettingsDialogFunc)();
  typedef void (*LocalServerInitializeFunc)
       (OperaLocalServerInterface **opera_local_server_instance);

  // Functions called by Opera when a worker thread is created.
  CreateWorkerFunc create_worker;

  // Functions called by Opera when an object is cloned in a worker thread.
  CloneObjectFunc clone_object;

  // Functions called by Opera to open the Gears settings dialog.
  OpenSettingsDialogFunc open_settings_dialog;

  // Functions called by Opera to initialize the URL data storage.
  LocalServerInitializeFunc local_server_init;
};

// This interface is used by Opera to get the data from Gears needed
// when requesting a URL.
class OperaHttpRequestDataProviderInterface {
 public:
  virtual ~OperaHttpRequestDataProviderInterface() {}

  // Returns true if the request is supposed to be asynchronous.
  virtual bool IsAsync() = 0;

  // Called repeatedly to iterate through the additional headers explicitly
  // set by the Gears code for this request. As long as the funtion returns
  // true name and value will hold the name/value pair of the header.
  virtual bool GetNextAddedReqHeader(const unsigned short **name,
                                     const unsigned short **value) = 0;

  // Returns the URL string for the request.
  virtual const unsigned short *GetUrl() = 0;

  // Returns the HTTP method string for the request.
  virtual const unsigned short *GetMethod() = 0;

  // Only applicable if the method is POST. Returns either the length of
  // the post data or copies chunks of the actual post data.
  // If called with max_size set to -1 it will return the length of the
  // data to be posted.
  // If called with a buffer pointing to a preallocated buffer and the
  // length of the buffer in max_size, the actual data to be posted will be
  // copied to the buffer. Returns the amount of data copied to the buffer.
  // Must be called until it returns a length less than max_size.
  virtual long GetPostData(unsigned char *buffer,
                           long max_size) = 0;
};

// This interface is used by the Opera implementation of the Gears code to
// return the data fetched by Opera after a HTTP request.
class OperaUrlDataInterface {
 public:
  virtual ~OperaUrlDataInterface() {}

  // Returns the HTTP status code
  virtual unsigned int GetStatus() = 0;

  // After calling, status will contain a pointer to the HTTP status text
  virtual void GetStatusText(const char **status) = 0;

  // After calling, content will contain a non-terminated buffer of the
  // data from the requested URL. content_length will hold the length
  // of the data in the content buffer.
  virtual void GetBodyText(const char **content,
                           unsigned long *content_length) = 0;

  // After calling, a continous 0-erminated string of the headers returned
  // from the server separated by linebreaks.
  virtual void GetResponseHeaders(const char **headers) = 0;

  // After calling, charset will contain the character set specified from
  // the server as a string.
  virtual void GetResponseCharset(const char **charset) = 0;

  // Returns true if the loading of the URL is completed.
  virtual bool IsFinished() = 0;

  // Called by the Gears code when it is finished processing the URL data.
  virtual void OnCompleted() = 0;
};

// Listener interface for callbacks during loading of a URL requested from
// the Gears code.
class OperaHttpListenerInterface {
 public:
  // Constants returned from Gears when OnRedirected is called.
  // OK if we should redirect, CANCEL if not.
  enum RedirectStatus {
    REDIRECT_STATUS_OK,
    REDIRECT_STATUS_CANCEL
  };

  // Constants for error codes sent to Gears in an OnError() call.
  enum LoadingError {
    LOADING_ERROR_GENERIC,
    LOADING_ERROR_OUT_OF_MEMORY
  };

  virtual ~OperaHttpListenerInterface() {}

  // Called when a chunk of data is received from the server. Will be called
  // repeatedly until the URL is finished loading.
  virtual void OnDataReceived() = 0;

  // Called when the URL is redirected. The new URL string will be in
  // redirect_url. Return REDIRECT_OK if we should redirect to the new URL
  // or REDIRECT_CANCEL if we should not.
  virtual RedirectStatus OnRedirected(const unsigned short *redirect_url) = 0;

  // Called when an error occurs during loading of the URL. err will hold the
  // error code.
  virtual void OnError(LoadingError err) = 0;

  // This will be called back from Opera immediately when the an HTTP request
  // has been made from Gears. It will contain a pointer to the an object
  // implementing the OpUrlData interface.
  virtual void OnRequestCreated(OperaUrlDataInterface *data) = 0;

  // This will be called once when the request in Opera is deleted for some
  // reason, for instance when we are finished.
  virtual void OnRequestDestroyed() = 0;
};

// Listener interface for callbacks during the display of a dialog requested
// by the Gears code.
class OperaDialogListenerInterface {
 public:
  virtual ~OperaDialogListenerInterface() {}

  // Called when the user closes the dialog. A pointer to the result string
  // will be in result.
  virtual void OnClose(const unsigned short *result) = 0;
};

// The interface between Opera and Gears. Gears will call one of these
// functions when it wants Opera to perform a specific task.
class OperaGearsApiInterface {
 public:
  virtual ~OperaGearsApiInterface() {}

  // Request Opera to load a URL.
  // plugin_instance must contain the plugin object from the NPAPI invocation.
  // It is used internally in Opera to identify the document context which the
  // URL should be loaded in.
  // req_data must point to an implementation of the
  // OpHTTPRequestDataProviderInterface that will deliver the information
  // about the URL to load.
  // listener must point to an implementation of the OpHTTPListener interface
  // that Opera will use to report results of the loading.
  // If req_data->IsAsync() returns true this function will return immediately
  // or else it will block until the URL is loaded.
  virtual void RequestUrl(NPP plugin_instance,
                          OperaHttpRequestDataProviderInterface *req_data,
                          OperaHttpListenerInterface *listener) = 0;

  // Request Opera to display a dialog.
  // plugin_instance must contain the plugin object from the NPAPI invocation.
  // It is used internally in Opera to identify the document context which the
  // dialog should be opened in.
  // html_file must point to a string identifying the html file to be used as
  // a template for the dialog.
  // width and height will be used to set the size of the dialog.
  // arguments_string will contain a JSON formatted string with the arguments
  // that will be passed to the dialog.
  // After this call returns result will contain the JSON formatted results
  // returned from the dialog.
  virtual void OpenDialog(NPP plugin_instance,
                          const unsigned short *html_file,
                          int width,
                          int height,
                          const unsigned short *arguments_string,
                          const unsigned short **result) = 0;

  // Create a Opera native WorkerPool object.
  // Returns the NPAPI representation of the workerpool object.
  // plugin_instance must contain the plugin object from the NPAPI invocation.
  // It is used internally in Opera to identify the document context which the
  // WorkerPool should be created in.
  // factory specifies the NPAPI representation of the Gears factory object.
  virtual NPObject *CreateWorkerPool(NPP plugin_instance,
                                     NPObject *factory) = 0;

  // Sets the global object of the plugin_instance.
  virtual void SetCurrentGlobalObject(NPP plugin_instance,
                                      NPObject *global_object) = 0;

  // Request Opera to return the cookie value associated with the url url.
  // After the function returns, cookie_out will point to the cookie data.
  virtual void GetCookieString(const unsigned short *url,
                               const char **cookie_out) = 0;
};

#endif  // OPERA_CALLBACK_API_H__
