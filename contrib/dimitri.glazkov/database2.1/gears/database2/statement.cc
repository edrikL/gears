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

#include "gears/database2/statement.h"

#include "gears/database2/transaction.h"

void Database2Statement::InvokeCallback(Database2Transaction *tx) {
  // for now, just return the Database2Statement
  JsParamToSend send_argv[] = {
    { JSPARAM_STRING16, &sql_statement_ }
  };

  if (HasCallback()) {
    tx->GetJsRunner()->InvokeCallback(callback_.get(), ARRAYSIZE(send_argv),
                                      send_argv, NULL);
  }
}

void Database2Statement::InvokeErrorCallback(Database2Transaction *tx,
                                             JsObject *error) {
}

bool Database2Statement::Create(const std::string16 &sql_statement,
                                JsArray *sql_arguments,
                                JsRootedCallback *callback,
                                JsRootedCallback *error_callback,
                                Database2Statement **instance) {
  assert(instance && *instance);

  scoped_ptr<Database2Statement> statement(new Database2Statement());

  // NULL should be passed if no arguments are specified
  assert(!sql_arguments || !JsTokenIsNullOrUndefined(sql_arguments->token()));
  // NULL should be passed if a callback is not specified
  assert(!callback || !JsTokenIsNullOrUndefined(callback->token()));
  assert(!error_callback || !JsTokenIsNullOrUndefined(error_callback->token()));

  statement->sql_statement_.assign(sql_statement);
  statement->callback_.reset(callback);
  statement->error_callback_.reset(error_callback);

  Database2Values *arguments;
  if (!Database2Values::CreateFromJsArray(sql_arguments, &arguments)) {
    return false;
  }

  statement->arguments_.reset(arguments);

  *instance = statement.release();
  return true;
}

// static
bool Database2Values::CreateFromJsArray(const JsArray *js_array,
                                        Database2Values **instance) {
  assert(instance && *instance);

  scoped_ptr<Database2Values> result(new Database2Values());

  // since the arguments are optional, they could be passed as NULL
  if (js_array == NULL) {
    result->length_ = 0;
    *instance = result.release();
    return true;
  }

  int len;
  if (!js_array->GetLength(&len)) {
    // unable to query JsArray, someting's gone horribly wrong
    // returning with failure will trigger an internal error
    assert(false);
    return false;
  }

  result->length_ = len;
  // a JsArray produces one row of values
  result->StartNewRow();
  Variant *row = result->rows_.back();
  for(int i = 0; i < len; i++) {
    switch(js_array->GetElementType(i)) {
      case JSPARAM_INT: {
        int value;
        if (!js_array->GetElementAsInt(i, &value)) {
          return false;
        }
        row[i].type = JSPARAM_INT;
        row[i].int_value = value;
        break;
      }
      case JSPARAM_DOUBLE: {
        double value;
        if (!js_array->GetElementAsDouble(i, &value)) {
          return false;
        }
        row[i].type = JSPARAM_DOUBLE;
        row[i].double_value = value;
        break;
      }
      case JSPARAM_STRING16: {
        std::string16 value;
        if (!js_array->GetElementAsString(i, &value)) {
          return false;
        }
        row[i].type = JSPARAM_STRING16;
        row[i].string_value = new std::string16(value);
        break;
      }
      case JSPARAM_NULL: {
        // Variant's type is set to JSPARAM_NULL by default, no need to do
        // anything here
        break;
      }
      default: {
        // invalid type
        return false;
      }
    }
  }

  *instance = result.release();
  return true;
}

void Database2Values::StartNewRow() {
  rows_.push_back(new Variant[length_]);
}

bool Database2Values::Next() {
  return ++current_row_ >= length_;
}

JsParamType Database2Values::GetType(int index) const {
  return current(index).type;
}

bool Database2Values::GetElementAsInt(int index, int *value) {
  assert(current(index).type == JSPARAM_INT);
  assert(value);
  *value = current(index).int_value;
  return true;
}

bool Database2Values::GetElementAsDouble(int index, double *value) {
  assert(current(index).type = JSPARAM_DOUBLE);
  assert(value);
  *value = current(index).double_value;
  return true;
}

bool Database2Values::GetElementAsString(int index, std::string16 *value) {
  assert(current(index).type = JSPARAM_STRING16);
  assert(value);
  *value = *(current(index).string_value);
  return true;
}
