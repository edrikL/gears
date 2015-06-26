# Introduction #

JavaScript is great but sometimes you just need to get at the guts of the machine. The Channel API will provide a way for javascript to launch and communicate with native processes in a secure way.

# Details #

The Channel module will expose one new object in javascript, and an abstract class in C++.

## channel javascript class ##
```
  callback onmessage(messageString)
  callback onerror(errorNum)
  callback ondisconnect()
  
  void send(messageString)
  void connect(channelName)
  void disconnect()
```

## GearsChannel  C++ class ##
```
  class GearsChannel {
   public:
    explicit GearsChannel(const char *requesting_page);
    virtual ~GearsChannel();

    void DispatchMessages();

    void Send(const char *message_string);

    virtual void OnMessage(const char *message_string) = 0;
    virtual void OnError(int error) = 0;
    virtual void OnDisconnect() = 0;

    // no need for 'connect' and 'disconnect'; they are implicit in the
    // processes' creation and destruction.
  };
```

## Registration ##
The Channel module will maintain a registry of native services identified by a URI. For security reasons, this registry will also maintain information about what sites are allowed to access this service. Fields in this registry will include:

  * Full path to the executable
  * URI identifier of the service
  * URIs that may access this service.

When javascript requests a connection to a registered URI identifier, Gears will spin up the referenced executable as a subprocess of the browser process. Every instance of the channel executes its own process, which then communicate through pipes redirecting stdin and stdout. In this sense, you can think of these services as client-side CGI scripts.

Details of where this registration file is stored are tbd, and will be OS-specific.

## Example ##

The HelloWorld service registers with the Channel module as follows:

`"c:\program files\helloworld\helloworld.exe","http://www.example.com/helloworld","http://www.example.com/hellohost.js"`

and is written in C++ as this:

```
class SayHello : public GearsChannel {
 public:
  SayHello(const char *connection_id) : GearsChannel(connection_id) {}
  virtual ~SayHello() {}

  virtual void OnMessage(const char *message) {
    // parse message into my made-up JSONObject
    JSONObject parsed_message;
    parsed_message.parse(message);

    if (parsed_message.getproperty("type") == "sayhello") {
      MessageBox(NULL, "Hello World", "Hello World", MB_OK);
    }
  }
};

int main(int argc, const char *argv[]) {
  if (argc != 2)
    return -1; // invalid args

  SayHello channel(argv[1]);

  // this just waits for messages and hands them to OnMessage
  channel.DispatchMessages();
}
```

Note that the GearsChannel class is just a helper class to read and write from stdin and stdout. As the channel object merely reads and writes strings, such a service could very easily be written as a shell script (on Linux or OS X -- Windows users aren't so lucky). This service is accessed from a web app running on http://www.example.com/hellohost.js as such:

```
var channel = google.gears.factory.create('beta.channel', '1.0');

channel.onmessage = function(message) {
  // don't handle any messages for now
};

channel.connect('http://www.example.com/helloworld');
channel.send({type:'sayhello'});
```

This causes the Channel module to launch:

`c:\program files\helloworld\helloworld.exe "http://www.example.com/hellohost.js"`

which then pops up a Hello World message box.

## Some high-level notes on security ##
Obviously adding any functionality to javascript is a scary thing; functionality to communicate with native processes is particularly scary. We want to make it easy for naïve developers to do the right thing; we will try our best to guard against "oops"-type mistakes which could lead to security holes. A big concern is poorly-written modules that allow information to leak from one web app to another in a sort of cross-domain scripting attack.

Native modules must explicitly register pages that can talk to them. The registration mechanism will allow multiple entries per native module, but each entry is explicit (no wildcards, for instance).

Each instance of each native module exists in a separate process. By sandboxing each instance in its own process, we greatly reduce the risk that information may "leak" from one page to another. We ensure that the bugs in poorly written modules cannot affect other modules, other instances of the same module, or the browser.

## Guarantees and the Lack Thereof ##
Because we're using a robust, time-tested mechanism of communicating between processes (pipes), we should fairly reliably be able to detect errors and ensure successes. However, until we have code written and tested, we provide no guarantee that if you send a message, it will be received by the other side.  Among the scenarios in which a message will not be received are, the other side has been closed, or the communication channel has been corrupted by cosmic rays, or filled with weasels or something.

We do, however, provide a guarantee of order: if you send messages A and B, one of the following conditions will hold true:

  * Message A is received, then message B is received
  * Message A is received, and message B is not
  * Neither message A nor message B is received.