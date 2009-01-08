#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>

#include "gears/base/safari/scoped_cf.h"

// Small command line program to launch a URL in a specific browser.
// This cannot be achieved via open -a 'browser' 'url', see bug 369 for details.
int main (int argc, const char * argv[]) {
  // Simple sanity check.
  if (argc != 3) {
     fprintf(stderr,
             "usage: %s 'path to web browser' 'url to open'\n",
             argv[0]);
     return -1;
  }

  const char *in_browser_path = argv[1];
  scoped_cftype<CFStringRef> browser_path_str(
                                 CFStringCreateWithBytes(
                                    kCFAllocatorDefault,
                                    (const UInt8 *)in_browser_path,
                                    strlen(in_browser_path),
                                    kCFStringEncodingUTF8,
                                    false));
  const char *in_url = argv[2];
  scoped_cftype<CFStringRef> url(CFStringCreateWithBytes(
                                     kCFAllocatorDefault,
                                     (const UInt8 *)in_url,
                                     strlen(in_url),
                                     kCFStringEncodingUTF8,
                                     false));

  scoped_cftype<CFURLRef> browser_path(CFURLCreateWithFileSystemPath(
                                       kCFAllocatorDefault,
                                       browser_path_str.get(),
                                       kCFURLPOSIXPathStyle,
                                       true));
  scoped_cftype<CFURLRef> url_str(CFURLCreateWithString(kCFAllocatorDefault,
                                                        url.get(),
                                                        NULL));
  scoped_cftype<CFArrayRef> urls(CFArrayCreate(kCFAllocatorDefault,
                                               (const void **)&url_str,
                                               1,
                                               NULL));

  if (url.get() == NULL ||
      browser_path.get() == NULL ||
      url_str.get() == NULL ||
      urls.get() == NULL) {
    fprintf(stderr, "Error: Couldn't parse parameters\n");
    return -1;
  }

  LSLaunchURLSpec spec;

  spec.appURL = browser_path.get();
  spec.itemURLs = urls.get();
  spec.passThruParams = NULL;
  spec.launchFlags = kLSLaunchDefaults;
  spec.asyncRefCon = 0;

  OSStatus err = LSOpenFromURLSpec(&spec, NULL);

  if (err != noErr) {
    fprintf(stderr,
            "Error: LSOpenFromURLSpec() returned %d\n",
            static_cast<int>(err));
    return -1;
  }

  return 0;
}
