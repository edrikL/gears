// Copyright 2006, Google Inc.
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

#import <WebKit/WebKit.h>

#import "gears/base/common/common_sf.h"
#import "gears/base/common/file.h"
#import "gears/base/safari/string_utils.h"
#import "gears/factory/safari/factory.h"
#import "gears/factory/safari/factory_utils.h"
#import "gears/localserver/safari/file_submitter_sf.h"
#import "gears/localserver/safari/resource_store_sf.h"

@interface GearsFileSubmitter(PrivateMethods)
- (void)setFileInput:(id)input url:(NSString *)urlStr;
@end

// Get the underlying implementation
@interface DOMHTMLInputElement (GoogleGearsWebCoreInternal)
- (void /*WebCore::HTMLInputElement*/ *)_HTMLInputElement;
@end

static const char16 *kMissingFilename = STRING16(L"Unknown");

@implementation GearsFileSubmitter
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || SafariGearsBaseClass ||
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"setFileInputElement", @"setFileInput:url:",
    nil];
}

//------------------------------------------------------------------------------
- (id)initWithStore:(GearsResourceStore *)store {
  if ((self = [super initWithFactory:[store factory]])) {
    gearsResourceStore_ = [store retain];
  }
  
  return self;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Private ||
//------------------------------------------------------------------------------
- (void)setFileInput:(id)element url:(NSString *)url {
  NSString *fullURL = [factory_ resolveURLString:url];
  std::string16 url16;
  
  if (![fullURL string16:&url16])
    ThrowExceptionKeyAndReturn(@"invalidParameter");
  
  if (!base_->EnvPageSecurityOrigin().IsSameOriginAsUrl(url16.c_str()))
    ThrowExceptionKeyAndReturn(@"invalidDomainAccess");
  
  // Must be some kind of input element...
  if (![element isKindOfClass:[DOMHTMLInputElement class]])
    ThrowExceptionKeyAndReturn(@"invalidParameter");
  
  // Retrieve data from the store
  ResourceStore *store = [gearsResourceStore_ resourceStore];
  
  if (!store)
    return;
  
  // Read data from our local store
  ResourceStore::Item item;
  if (!store->GetItem(url16.c_str(), &item))
    ThrowExceptionKeyAndReturn(@"failedToGetItem");
  
  std::string16 filename_str;
  item.payload.GetHeader(HttpConstants::kXCapturedFilenameHeader,
                         &filename_str);
  if (filename_str.empty()) {
    // This handles the case where the URL didn't get into the database
    // using the captureFile API. It's arguable whether we should support this.
    filename_str = kMissingFilename;
  }
  
  // Create a temporary directory to hold the file
  std::string16 temp_dir;
  if (!File::CreateNewTempDirectory(&temp_dir))
    ThrowExceptionKeyAndReturn(@"failedToCreateDir");
  
  // Create the file with the contents
  NSString *tempDirStr = [NSString stringWithString16:temp_dir.c_str()];
  NSString *fileName = [NSString stringWithString16:filename_str.c_str()];
  NSString *path = [tempDirStr stringByAppendingPathComponent:fileName];
  std::string16 full_path;

  [path string16:&full_path];
  
  if (!File::WriteVectorToFile(full_path.c_str(), item.payload.data.get()))
    ThrowExceptionKeyAndReturn(@"failedToCreateFile");

  // TODO(waylonis): Figure out how to send file data in form
  [WebScriptObject throwException:@"File Submitter not implemented"];
}

@end
