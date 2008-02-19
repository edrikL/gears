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
//
// Provides a scoped container for automatic destruction.
// Is based on scoped_ptr_malloc from google3 but makes some important changes:
//   (a) supports pointers to objects that are not fully defined
//   (b) support non-pointer values (e.g. handles)
// I assume there was some good reason for the old restrictions, so prefer
// scoped_ptr[_malloc] whenever possible, and use this new container carefully.

#ifndef GEARS_BASE_COMMON_SCOPED_TOKEN_H__
#define GEARS_BASE_COMMON_SCOPED_TOKEN_H__

#include <assert.h>

template<typename T, typename FreeFunctor>
class scoped_token {
 private:

  T val;

  scoped_token(scoped_token const &);
  scoped_token & operator=(scoped_token const &);

 public:

  typedef T element_type;

  explicit scoped_token(T v): val(v), has_value_(true) {}

  ~scoped_token() {
    if (has_value_) {
      free_(val);
    }
  }

  void reset(T v) {
    if (has_value_) {
      free_(val);
    }
    val = v;
    has_value_ = true;
  }

  bool operator==(T v) const {
    assert(has_value_);
    return val == v;
  }

  bool operator!=(T v) const {
    assert(has_value_);
    return val != v;
  }

  T get() const {
    assert(has_value_);
    return val;
  }

  void swap(scoped_token & b) {
    T v_tmp = b.val;
    b.val = val;
    val = v_tmp;

    bool b_tmp = b.has_value_;
    b.has_value_ = has_value_;
    has_value_ = b_tmp;
  }

  T release() {
    has_value_ = false;
    return val;
  }

 private:

  // no reason to use these: each scoped_token should have its own object
  template <typename U, typename GP>
  bool operator==(scoped_token<U, GP> const& v) const;
  template <typename U, typename GP>
  bool operator!=(scoped_token<U, GP> const& v) const;

  bool has_value_;
  static FreeFunctor const free_;
};

template<typename T, typename FF>
FF const scoped_token<T,FF>::free_ = FF();

template<typename T, typename FF> inline
void swap(scoped_token<T,FF>& a, scoped_token<T,FF>& b) {
  a.swap(b);
}

template<typename T, typename FF> inline
bool operator==(T v, const scoped_token<T,FF>& b) {
  return v == b.get();
}

template<typename T, typename FF> inline
bool operator!=(T v, const scoped_token<T,FF>& b) {
  return v != b.get();
}


#endif // GEARS_BASE_COMMON_SCOPED_TOKEN_H__
