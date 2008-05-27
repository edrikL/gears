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

#ifndef GEARS_DATABASE2_STATEMENT_H__
#define GEARS_DATABASE2_STATEMENT_H__

#include <vector>

#include "gears/base/common/common.h"
#include "gears/base/common/js_types.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

// forward declarations
class Database2;
class Database2Transaction;
class Database2Values;

// represents statement, for both synchronous and asynchronous operations
class Database2Statement {
 public:
  bool HasCallback() const {
    return callback_.get() != NULL;
  }

  bool HasErrorCallback() const {
    return error_callback_.get() != NULL;
  }

  void InvokeCallback(Database2Transaction *tx);
  void InvokeErrorCallback(Database2Transaction *tx, JsObject *error);

  // create a statement instance
  // must passs NULL for arguments or callbacks if they are not specified
  static bool Create(const std::string16 &sql_statement,
                     JsArray *sql_arguments,
                     JsRootedCallback *callback,
                     JsRootedCallback *error_callback,
                     Database2Statement **instance);

  std::string16 sql() const { return sql_statement_; }
  Database2Values *arguments() const { return arguments_.get(); }
 private:
  Database2Statement() {}
  // if true, the statement has invalid arguments
  bool bogus_;
  std::string16 sql_statement_;
  scoped_ptr<Database2Values> arguments_;
  scoped_ptr<JsRootedCallback> callback_;
  scoped_ptr<JsRootedCallback> error_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(Database2Statement);
};

// Used for marshaling statement arguments to the database thread and results to
// the main thread. A union-based structure is used to store value-type pairs.
// TODO(dimitri.glazkov): Clean me up
class Database2Values {
 public:
  Database2Values() {}
  ~Database2Values() {
    // remove all rows
    for(unsigned int i = 0; i < rows_.size(); i++) {
      delete[] rows_[i];
    }
  }

  static bool CreateFromJsArray(const JsArray *js_array,
                                Database2Values **instance);

  // methods for reading values
  int length() const { return length_; }
  void Reset() { current_row_ = 0; }
  bool Next();
  JsParamType GetType(int index) const;
  bool GetElementAsInt(int index, int *value);
  bool GetElementAsDouble(int index, double *value);
  bool GetElementAsString(int index, std::string16 *value);

 private:
  void StartNewRow();

  // used to store value-type pairs
  struct Variant {
    Variant() : type(JSPARAM_NULL) {}
    ~Variant() {
      if (type == JSPARAM_STRING16) { delete string_value; }
    }

    JsParamType type;
    union {
      int int_value;
      double double_value;
      std::string16 *string_value;
    };
  };

  inline Variant &current(int index) const {
    assert(index >= 0 && index < length_);
    assert(current_row_ < length_);
    return rows_[current_row_][index];
  }

  friend class vector;
  std::vector<Variant*> rows_;
  int length_;
  int current_row_;

  DISALLOW_EVIL_CONSTRUCTORS(Database2Values);
};

#endif // GEARS_DATABASE2_STATEMENT_H__
