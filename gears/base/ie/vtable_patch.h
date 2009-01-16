// Copyright 2006-2009, Google Inc.
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

#ifndef GEARS_BASE_IE_VTABLE_PATCH_H__
#define GEARS_BASE_IE_VTABLE_PATCH_H__

#include <atlbase.h>
#include <assert.h>
#include "gears/base/common/basictypes.h"


/**
* Patches a single function in a vtable.
* In the destructor the patch is reverted.
*/
class VTablePatch {
  DISALLOW_EVIL_CONSTRUCTORS(VTablePatch);
 public:
  VTablePatch() : original_(NULL), original_address_(NULL) {
  }

  ~VTablePatch() {
    Unpatch();
  }

  /**
  * Patches a single function of a COM class.
  * @pre a patch must not have been applied already.
  */
  HRESULT Patch(IUnknown* p, uint32 fn_index, PROC hook_fn) {
    assert(p != NULL);
    assert(original_ == NULL);
    assert(original_address_ == NULL);
    assert(!IsHooked(p, fn_index, hook_fn));

    //
    // If you ever see the !IsHooked() assert (above), please fix the
    // code that caused it and don't rely on this last defense.  There's a
    // chance you might be hooking something you don't intended to hook or
    // are running your initialization code more than once.
    //
    if (IsHooked(p, fn_index, hook_fn))
      return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);

    PROC* fn_arr = *reinterpret_cast<PROC**>(p);
    original_address_ = &fn_arr[fn_index];

    DWORD old_protect = NULL;
    if (!::VirtualProtect(original_address_, sizeof(PROC),
                          PAGE_EXECUTE_READWRITE, &old_protect)) {
      assert(false);
      return AtlHresultFromLastError();
    }

    // Since we've now patched this process, we must lock the module
    // to prevent from being unloaded.
    _pAtlModule->Lock();

    // keep the original value for later and replace it with the hook function
    original_ = *original_address_;
    assert(original_ != hook_fn);
    *original_address_ = hook_fn;

    // Restore the previous protection flags
    if (!::VirtualProtect(original_address_, sizeof(PROC), old_protect,
                          &old_protect)) {
      assert(false);
      return AtlHresultFromLastError();
    }
    return S_OK;
  }

  /**
  * Unpatches the patch applied by a previous call to the Patch()
  * function.
  */
  HRESULT Unpatch() {
    if (original_ == NULL)
      return S_FALSE;

    assert(original_address_ != NULL);

    DWORD old_protect = NULL;
    if (!::VirtualProtect(original_address_, sizeof(PROC),
                          PAGE_EXECUTE_READWRITE, &old_protect)) {
      assert(false);
      return AtlHresultFromLastError();
    }

    // restore the original value
    *original_address_ = original_;

    // Restore the previous protection flags
    if (!::VirtualProtect(original_address_, sizeof(PROC), old_protect,
                          &old_protect)) {
      return AtlHresultFromLastError();
    }

    original_ = NULL;
    original_address_ = NULL;

    // Give back the reference we held while the patch was in place.
    _pAtlModule->Unlock();

    return S_OK;
  }

  /**
  * @returns a pointer to the original pointer that was replaced with
  *   a call to Patch()
  */
  inline PROC original() const {
    return original_;
  }

  /**
  * @returns true if the hook_fn has already been set at the given 
  *   location in the vtable of the supplied com object, p.
  */
  static bool IsHooked(IUnknown* p, uint32 fn_index, PROC hook_fn) {
    assert(p != NULL);
    PROC* fn_arr = *reinterpret_cast<PROC**>(p);
    return fn_arr[fn_index] == hook_fn;
  }

 protected:
  /// The original function pointer
  PROC original_;
  /// The address of the function pointer location in the vtable
  PROC* original_address_;
};

#endif  // GEARS_BASE_IE_VTABLE_PATCH_H__
