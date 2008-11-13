// Copyright 2008, Google Inc.
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

#include "gears/localserver/opera/http_request_op.h"


//------------------------------------------------------------------------------
// Create
//------------------------------------------------------------------------------
// static
bool HttpRequest::Create(scoped_refptr<HttpRequest>* request) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

// static
bool HttpRequest::CreateSafeRequest(scoped_refptr<HttpRequest>* request) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// Construction, destruction and refcounting
//------------------------------------------------------------------------------
OPHttpRequest::OPHttpRequest() {
  // TODO(steveblock): Implement me.
  assert(false);
}

OPHttpRequest::~OPHttpRequest() {
  // TODO(steveblock): Implement me.
  assert(false);
}

void OPHttpRequest::Ref() {
  // TODO(steveblock): Implement me.
  assert(false);
}

void OPHttpRequest::Unref() {
  // TODO(steveblock): Implement me.
  assert(false);
}

//------------------------------------------------------------------------------
// GetReadyState
//------------------------------------------------------------------------------
bool OPHttpRequest::GetReadyState(ReadyState *state) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// GetResponseBody
//------------------------------------------------------------------------------
bool OPHttpRequest::GetResponseBody(scoped_refptr<BlobInterface>* blob) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// GetStatus
//------------------------------------------------------------------------------
bool OPHttpRequest::GetStatus(int *status) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// GetStatusText
// TODO(michaeln): remove this method from the interface, prefer getStatusLine
//------------------------------------------------------------------------------
bool OPHttpRequest::GetStatusText(std::string16 *status_text) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// GetStatusLine
//------------------------------------------------------------------------------
bool OPHttpRequest::GetStatusLine(std::string16 *status_line) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// WasRedirected
//------------------------------------------------------------------------------
bool OPHttpRequest::WasRedirected() {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// GetFinalUrl
//------------------------------------------------------------------------------
bool OPHttpRequest::GetFinalUrl(std::string16 *full_url) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// GetInitialUrl
//------------------------------------------------------------------------------
bool OPHttpRequest::GetInitialUrl(std::string16 *full_url) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// Open
//------------------------------------------------------------------------------
bool OPHttpRequest::Open(const char16 *method, const char16* url, bool async,
                         BrowsingContext *browsing_context) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// SetRequestHeader
//------------------------------------------------------------------------------
bool OPHttpRequest::SetRequestHeader(const char16* name, const char16* value) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// Send
//------------------------------------------------------------------------------
bool OPHttpRequest::Send(BlobInterface *blob) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// GetAllResponseHeaders
//------------------------------------------------------------------------------
bool OPHttpRequest::GetAllResponseHeaders(std::string16 *headers) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// GetResponseCharset
//------------------------------------------------------------------------------
std::string16 OPHttpRequest::GetResponseCharset() {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// GetResponseHeader
//------------------------------------------------------------------------------
bool OPHttpRequest::GetResponseHeader(const char16* name,
                                      std::string16 *value) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// Abort
//------------------------------------------------------------------------------
bool OPHttpRequest::Abort() {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

//------------------------------------------------------------------------------
// SetListener
//------------------------------------------------------------------------------
bool OPHttpRequest::SetListener(HttpListener *listener,
                                bool enable_data_available) {
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}
