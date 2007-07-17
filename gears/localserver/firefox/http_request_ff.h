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

#ifndef GEARS_LOCALSERVER_FIREFOX_HTTP_REQUEST_FF_H__
#define GEARS_LOCALSERVER_FIREFOX_HTTP_REQUEST_FF_H__

#include <nsIStreamListener.h>
#include <nsIInterfaceRequestor.h>
#include "gears/third_party/gecko_internal/nsIChannelEventSink.h"
#include "ff/genfiles/localserver.h" // from OUTDIR
#include "gears/base/common/common.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"
#include "gears/localserver/common/http_request.h"

class nsIChannel;
class nsIHttpChannel;
class nsIRequest;


//------------------------------------------------------------------------------
// FFHttpRequest
//------------------------------------------------------------------------------
class FFHttpRequest : public HttpRequest,
                      public nsIStreamListener,
                      public nsIChannelEventSink,
                      public nsIInterfaceRequestor,
                      public SpecialHttpRequestInterface {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR

  // Scour's HttpRequest interface

  // refcounting
  virtual int AddReference();
  virtual int ReleaseReference();

  // properties
  virtual bool GetReadyState(long *state);
  virtual bool GetResponseBody(std::vector<unsigned char> *body);
  virtual std::vector<unsigned char> *GetResponseBody();
  virtual bool GetStatus(long *status);
  virtual bool GetStatusText(std::string16 *status_text);
  virtual bool GetStatusLine(std::string16 *status_line);

  virtual bool SetFollowRedirects(bool follow);
  virtual bool WasRedirected();
  virtual bool GetRedirectUrl(std::string16 *full_redirect_url);

  // methods
  virtual bool Open(const char16 *method, const char16* url, bool async);
  virtual bool SetRequestHeader(const char16* name, const char16* value);
  virtual bool Send();
  virtual bool GetAllResponseHeaders(std::string16 *headers);
  virtual bool GetResponseHeader(const char16* name, std::string16 *header);
  virtual bool Abort();

  // events
  virtual bool SetOnReadyStateChange(ReadyStateListener *listener);

 private:
  friend HttpRequest *HttpRequest::Create();

  FFHttpRequest();
  ~FFHttpRequest();

  already_AddRefed<nsIHttpChannel> GetCurrentHttpChannel();

  void SetReadyState(int state);

  static NS_METHOD StreamReaderFunc(nsIInputStream* in,
                                    void* closure,
                                    const char* fromRawSegment,
                                    PRUint32 toOffset,
                                    PRUint32 count,
                                    PRUint32 *writeCount);
  int state_;
  scoped_ptr< std::vector< unsigned char > > response_body_;
  bool was_sent_;
  bool is_complete_;
  bool was_aborted_;
  bool follow_redirects_;
  bool was_redirected_;
  std::string16 redirect_url_;
  nsCOMPtr<nsIChannel> channel_;
  ReadyStateListener *listener_;
};

#endif  // GEARS_LOCALSERVER_FIREFOX_HTTP_REQUEST_FF_H__
