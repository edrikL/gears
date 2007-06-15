// Copyright 2007, Google Inc.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//
//  Test.mm
//  ScourSafari
//

#include "gears/base/common/string_utils.h"
#include "gears/base/common/thread_locals.h"
#include "gears/localserver/common/async_task.h"
#include "gears/localserver/common/localserver_tests.h"
#include "gears/localserver/safari/http_cookies.h"

bool gCanTerminate = NO;

//=============================================================================
#if 0
extern "C" void ScourSafariLog(const char *msg, ...) {
  NSString *str = [NSString stringWithUTF8String:msg];
  va_list args;
  va_start(args, msg);
  NSLogv(str, args);
  va_end(args);
}
#endif

class MyListener : public AsyncTask::Listener {
 public:
  void HandleEvent(int msg_code, int msg_param, AsyncTask *source);
};

void MyListener::HandleEvent(int msg_code, int msg_param, AsyncTask *source) {
  printf("MyListener:code: %d param: %d source: %p\n", msg_code, msg_param, source);
  if (msg_code == 4)
    gCanTerminate = YES;
}

class TestAsyncTask : public AsyncTask {
public:
  void LoadURL(const char *url) {
    Init(); url_ = url;
  }
  void Run();
  const char *url_;
};

void TestAsyncTask::Run() {
  WebCacheDB::PayloadInfo info;
  std::string16 url16;
  UTF8ToString16(url_, &url16);
  bool was_redirected;
  std::string16 redirected_url;
  ThreadLocals::SetValue(42, (char *)"Foobar", NULL);
  printf("TestAsyncTask:ThreadLocal: %s\n", ThreadLocals::GetValue(42));
  bool capture = false;
  std::string16 error;
  bool result = HttpGet(url16.c_str(), capture, NULL, NULL, &info,
                        &was_redirected, &redirected_url, &error);
  std::string utf8;
  printf("TestAsyncTask:result: %d\n", result);
  String16ToUTF8(error.c_str(), &utf8);
  printf("TestAsyncTask:error: %s\n", utf8.c_str());
  printf("TestAsyncTask:status: %d\n", info.status_code);
  String16ToUTF8(info.status_line.c_str(), &utf8);
  printf("TestAsyncTask:status line: %s\n", utf8.c_str());
  String16ToUTF8(info.headers.c_str(), &utf8);
  printf("TestAsyncTask:Headers: %s\n", utf8.c_str());
  std::vector<unsigned char> *data_vec = info.data.get();
  
  if (data_vec) {
    unsigned char *data = &(*data_vec)[0];
    int len = info.data->size();
    if (len)
      data[len - 1] = 0;
    printf("TestAsyncTask:Data: %d bytes\n", strlen((const char *)data));
//    printf("TestAsyncTask:Data: %s\n", data);
  } else
    printf("NO DATA!!!\n");
  
  NotifyListener(4, 0);
  
  if (was_redirected) {
    std::string redirect8;
    String16ToUTF8(redirected_url.c_str(), &redirect8);
    printf("TestAsyncTask:Redirect to %s\n", redirect8.c_str());
  }
  gCanTerminate = YES;
}

//=============================================================================
static void Test(int argc, const char *argv[]) {
//  TestWebCacheAll();
  const char *url_str = "http://www.apple.com";
#endif
  
#if 1
  MyListener l;
  TestAsyncTask *at = new TestAsyncTask;
  
  printf("TestAsyncTask: %p\n", at);
  printf("Test:Going to load %s\n", url_str);
  at->SetListener(&l);
  at->LoadURL(url_str);
  at->Start();
  at->DeleteWhenDone();
  
  while (!gCanTerminate) {
    printf("Test:loop...\n");
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1, true);
  }
  
  printf("Test:Loaded\n");
#else
  // Cookie test
  NSString *str = (NSString *)GetHTTPCookieString(CFSTR("http://answers.com"));
  NSLog(@"Cookie: >>%@<<", str);
  gCanTerminate = YES;
#endif
}

//=============================================================================
int main (int argc, const char *argv[]) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSRunLoop *rl;
  BOOL hasStarted = NO;
  
  // Setup application and runloop so that Cocoa, threaded, and RL 
  // tests can run properly
  [NSApplication sharedApplication];
  rl = [NSRunLoop currentRunLoop];
  
  while (!gCanTerminate) {
    [rl runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    
    if (!hasStarted) {
      Test(argc, argv);
      hasStarted = YES;
    }
  }
  
  // Cleanup
  [pool release];
  
  return(0);
}
