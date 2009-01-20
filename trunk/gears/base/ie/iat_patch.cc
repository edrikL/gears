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

#include "gears/base/ie/iat_patch.h"

#include <atlbase.h>  // for _pAtlModule
#include <assert.h>

// redifinitions of things from logging.h which is not part of the project
#define NOTREACHED() assert(false)
#define DCHECK(x) assert(x)
#define DCHECK_EQ(val1, val2) assert(val1 == val2)
#define DCHECK_NE(val1, val2) assert(val1 != val2)
#define COMPILE_ASSERT(x, label) assert(x)

namespace iat_patch {

struct InterceptFunctionInformation {
  bool finished_operation;
  const char* imported_from_module;
  const char* function_name;
  void* new_function;
  void** old_function;
  IMAGE_THUNK_DATA** iat_thunk;
  DWORD return_code;
};

static void* GetIATFunction(IMAGE_THUNK_DATA* iat_thunk) {
  if (NULL == iat_thunk) {
    NOTREACHED();
    return NULL;
  }

  // Works around the 64 bit portability warning:
  // The Function member inside IMAGE_THUNK_DATA is really a pointer
  // to the IAT function. IMAGE_THUNK_DATA correctly maps to IMAGE_THUNK_DATA32
  // or IMAGE_THUNK_DATA64 for correct pointer size.
  union FunctionThunk {
    IMAGE_THUNK_DATA thunk;
    void* pointer;
  } iat_function;

  iat_function.thunk = *iat_thunk;
  return iat_function.pointer;
}

static HMODULE GetModuleHandleFromAddress(void *address) {
  MEMORY_BASIC_INFORMATION mbi;
  SIZE_T result = VirtualQuery(address, &mbi, sizeof(mbi));
  return static_cast<HMODULE>(mbi.AllocationBase);
}

static bool InterceptEnumCallback(const PEImage &image, const char* module,
                                  DWORD ordinal, const char* name, DWORD hint,
                                  IMAGE_THUNK_DATA* iat, void* cookie) {
  InterceptFunctionInformation* intercept_information =
    reinterpret_cast<InterceptFunctionInformation*>(cookie);

  if (NULL == intercept_information) {
    NOTREACHED();
    return false;
  }

  DCHECK(module);

  if ((0 == lstrcmpiA(module, intercept_information->imported_from_module)) &&
     (NULL != name) &&
     (0 == lstrcmpiA(name, intercept_information->function_name))) {
    // Save the old pointer.
    if (NULL != intercept_information->old_function) {
      void* old_function = GetIATFunction(iat);
      if (image.module() == GetModuleHandleFromAddress(old_function)) {
        // If the old pointer points back into the importing module, we don't
        // trust it. It's most likely a deferred loading stub. In this case
        // we lookup the original function pointer in the exporting module
        // and return that value instead.
        HMODULE exporting_module = LoadLibraryA(module);
        void* exported_function = (exporting_module != NULL)
                                      ? GetProcAddress(exporting_module, name)
                                      : NULL;
        if (NULL == exported_function) {
          // Terminate the enumeration and fail to patch
          intercept_information->return_code = GetLastError();
          intercept_information->finished_operation = true;
          DCHECK(false);
          return false;
        }
        old_function = exported_function;
      }
      *(intercept_information->old_function) = old_function;
    }

    if (NULL != intercept_information->iat_thunk) {
      *(intercept_information->iat_thunk) = iat;
    }

    // portability check
    COMPILE_ASSERT(sizeof(iat->u1.Function) ==
      sizeof(intercept_information->new_function), unknown_IAT_thunk_format);

    // Patch the function.
    intercept_information->return_code =
      ModifyCode(&(iat->u1.Function),
                 &(intercept_information->new_function),
                 sizeof(intercept_information->new_function));

    // Terminate further enumeration.
    intercept_information->finished_operation = true;
    return false;
  }

  return true;
}

DWORD InterceptImportedFunction(HMODULE module_handle,
                                const char* imported_from_module,
                                const char* function_name, void* new_function,
                                void** old_function,
                                IMAGE_THUNK_DATA** iat_thunk) {
  if ((NULL == module_handle) || (NULL == imported_from_module) ||
     (NULL == function_name) || (NULL == new_function)) {
    NOTREACHED();
    return ERROR_INVALID_PARAMETER;
  }

  PEImage target_image(module_handle);
  if (!target_image.VerifyMagic()) {
    NOTREACHED();
    return ERROR_INVALID_PARAMETER;
  }

  InterceptFunctionInformation intercept_information = {
    false,
    imported_from_module,
    function_name,
    new_function,
    old_function,
    iat_thunk,
    ERROR_GEN_FAILURE};

  // First go through the IAT. If we don't find the import we are looking
  // for in IAT, search delay import table.
  target_image.EnumAllImports(InterceptEnumCallback, &intercept_information);
  if (!intercept_information.finished_operation) {
    target_image.EnumAllDelayImports(InterceptEnumCallback,
                                     &intercept_information);
  }

  return intercept_information.return_code;
}

DWORD RestoreImportedFunction(void* intercept_function,
                              void* original_function,
                              IMAGE_THUNK_DATA* iat_thunk) {
  if ((NULL == intercept_function) || (NULL == original_function) ||
      (NULL == iat_thunk)) {
    NOTREACHED();
    return ERROR_INVALID_PARAMETER;
  }

  if (GetIATFunction(iat_thunk) != intercept_function) {
    // Check if someone else has intercepted on top of us.
    // We cannot unpatch in this case, just raise a red flag.
    NOTREACHED();
    return ERROR_INVALID_FUNCTION;
  }

  return ModifyCode(&(iat_thunk->u1.Function),
                    &original_function,
                    sizeof(original_function));
}

DWORD ModifyCode(void* old_code, void* new_code, int length) {
  if ((NULL == old_code) || (NULL == new_code) || (0 == length)) {
    NOTREACHED();
    return ERROR_INVALID_PARAMETER;
  }

  // Change the page protection so that we can write.
  DWORD error = NO_ERROR;
  DWORD old_page_protection = 0;
  if (VirtualProtect(old_code,
                     length,
                     PAGE_READWRITE,
                     &old_page_protection)) {

    // Write the data.
    CopyMemory(old_code, new_code, length);

    // Restore the old page protection.
    error = ERROR_SUCCESS;
    VirtualProtect(old_code,
                  length,
                  old_page_protection,
                  &old_page_protection);
  } else {
    error = GetLastError();
    NOTREACHED();
  }

  return error;
}

IATPatchFunction::IATPatchFunction()
    : original_function_(NULL),
      iat_thunk_(NULL),
      intercept_function_(NULL) {
}

IATPatchFunction::~IATPatchFunction() {
  if (NULL != intercept_function_) {
    DWORD error = Unpatch();
    DCHECK_EQ(NO_ERROR, error);
  }
}

DWORD IATPatchFunction::Patch(HMODULE module_handle,
                              const char* imported_from_module,
                              const char* function_name,
                              void* new_function) {
  DCHECK_EQ(static_cast<void*>(NULL), original_function_);
  DCHECK_EQ(static_cast<IMAGE_THUNK_DATA*>(NULL), iat_thunk_);
  DCHECK_EQ(static_cast<void*>(NULL), intercept_function_);

  DWORD error = InterceptImportedFunction(module_handle,
                                          imported_from_module,
                                          function_name,
                                          new_function,
                                          &original_function_,
                                          &iat_thunk_);

  if (NO_ERROR == error) {
    DCHECK(original_function_);
    intercept_function_ = new_function;
    DCHECK_NE(original_function_, intercept_function_);
    DCHECK(check_patch());

    // Since we've now patched this process, we must lock the module
    // to prevent from being unloaded.
    _pAtlModule->Lock();
  }

  return error;
}

DWORD IATPatchFunction::Unpatch() {
  DWORD error = RestoreImportedFunction(intercept_function_,
                                        original_function_,
                                        iat_thunk_);

  if (NO_ERROR == error) {
    intercept_function_ = NULL;
    original_function_ = NULL;
    iat_thunk_ = NULL;

    // Give back the reference we held while the patch was in place.
    _pAtlModule->Unlock();
  }

  return error;
}

bool IATPatchFunction::check_patch() const {
  DCHECK(is_patched());
  return GetIATFunction(iat_thunk_) == intercept_function_;
}
}  // namespace iat_patch
