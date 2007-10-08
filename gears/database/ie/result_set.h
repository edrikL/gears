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

#ifndef GEARS_DATABASE_IE_RESULT_SET_H__
#define GEARS_DATABASE_IE_RESULT_SET_H__

#include <atlcoll.h>
#include "ie/genfiles/interfaces.h" // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"


struct sqlite3_stmt;

class ATL_NO_VTABLE GearsResultSet
    : public GearsBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsResultSet>,
      public IDispatchImpl<GearsResultSetInterface> {
 public:
  BEGIN_COM_MAP(GearsResultSet)
    COM_INTERFACE_ENTRY(GearsResultSetInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsResultSet)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  // End boilerplate code. Begin interface.

  // need a default constructor to CreateInstance objects in IE
  GearsResultSet();
  ~GearsResultSet();

  STDMETHOD(field)(int index, VARIANT *retval);
  STDMETHOD(fieldByName)(const BSTR field_name, VARIANT *retval);
  STDMETHOD(fieldName)(int index, VARIANT *retval);
  STDMETHOD(fieldCount)(int *retval);
  STDMETHOD(close)();
  STDMETHOD(next)();
  STDMETHOD(isValidRow)(VARIANT_BOOL *retval);

 private:
  friend class GearsDatabase;

  // Helper called by GearsDatabase.execute to initialize the result set
  bool InitializeResultSet(sqlite3_stmt *statement,
                           GearsDatabase *db,
                           std::string16 *error_message);


  // Helper shared by next() and SetStatement()
  bool NextImpl(std::string16 *error_message);
  bool Finalize();

  GearsDatabase *database_;
  sqlite3_stmt *statement_;
  CAtlMap<CStringW, int> column_indexes_; // TODO(miket): switch to STL
  bool column_indexes_built_;
  bool is_valid_row_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsResultSet);
};


#endif  // GEARS_DATABASE_IE_RESULT_SET_H__
