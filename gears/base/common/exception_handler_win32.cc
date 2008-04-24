// Copyright 2007, Google Inc.
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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <assert.h>
#include <shellapi.h>
#include <time.h>

#include "gears/base/common/exception_handler_win32.h"

#include "client/windows/handler/exception_handler.h"  // from breakpad/src
#include "client/windows/sender/crash_report_sender.h"  // from breakpad/src


// Product-specific constants.  MODIFY THESE TO SUIT YOUR PROJECT.
#include "genfiles/product_constants.h"
const wchar_t *kCrashReportProductName = L"Google_Gears";  // [naming]
const wchar_t *kCrashReportProductVersion = PRODUCT_VERSION_STRING
#if BROWSER_FF
                                            L" (win32 firefox)";
#elif BROWSER_IE
                                            L" (win32 ie)";
#elif BROWSER_NPAPI
                                            L" (win32 npapi)";
#endif


// Product-independent constants.
const wchar_t *kCrashReportUrl = L"http://www.google.com/cr/report";

const wchar_t *kCrashReportThrottlingRegKey = L"Software\\Google\\Breakpad\\Throttling";

const wchar_t *kCrashReportProductParam = L"prod";
const wchar_t *kCrashReportVersionParam = L"ver";

const int kCrashReportAttempts         = 3;
const int kCrashReportResendPeriodMs   = (1 * 60 * 60 * 1000);
const int kCrashReportsMaxPerInterval  = 5;
const int kCrashReportsIntervalSeconds = (24 * 60  * 60);

using namespace google_breakpad;


ExceptionManager* ExceptionManager::instance_ = NULL;

ExceptionManager::ExceptionManager(bool catch_entire_process)
    : catch_entire_process_(catch_entire_process),
      exception_handler_(NULL) {
  assert(!instance_);
  instance_ = this;
}

ExceptionManager::~ExceptionManager() {
  if (exception_handler_)
    delete exception_handler_;
  assert(instance_ == this);
  instance_ = NULL;
}

bool ExceptionManager::CanSendMinidump() {
  bool can_send = false;

  time_t current_time;
  time(&current_time);

  // For throttling, we remember when the last N minidumps were sent.

  time_t past_send_times[kCrashReportsMaxPerInterval];
  DWORD bytes = sizeof(past_send_times);
  memset(&past_send_times, 0, bytes);

  HKEY reg_key;
  DWORD create_type;
  if (ERROR_SUCCESS != RegCreateKeyExW(HKEY_CURRENT_USER,
                                       kCrashReportThrottlingRegKey, 0, NULL, 0,
                                       KEY_READ | KEY_WRITE, NULL,
                                       &reg_key, &create_type)) {
    return false;  // this should never happen, but just in case
  }

  if (ERROR_SUCCESS != RegQueryValueEx(reg_key, kCrashReportProductName, NULL,
                                       NULL,
                                       reinterpret_cast<BYTE*>(past_send_times),
                                       &bytes)) {
    // this product hasn't sent any crash reports yet
    can_send = true;
  } else {
    // find crash reports within the last interval
    int crashes_in_last_interval = 0;
    for (int i = 0; i < kCrashReportsMaxPerInterval; ++i) {
      if (current_time - past_send_times[i] < kCrashReportsIntervalSeconds) {
        ++crashes_in_last_interval;
      }
    }

    can_send = crashes_in_last_interval < kCrashReportsMaxPerInterval;
  }

  if (can_send) {
    memmove(&past_send_times[1],
            &past_send_times[0],
            sizeof(time_t) * (kCrashReportsMaxPerInterval - 1));
    past_send_times[0] = current_time;
  }

  RegSetValueEx(reg_key, kCrashReportProductName, 0, REG_BINARY,
                reinterpret_cast<BYTE*>(past_send_times),
                sizeof(past_send_times));

  return can_send;
}

static HMODULE GetModuleHandleFromAddress(void *address) {
  MEMORY_BASIC_INFORMATION mbi;
  SIZE_T result = VirtualQuery(address, &mbi, sizeof(mbi));

  return static_cast<HMODULE>(mbi.AllocationBase);
}

// Gets the handle to the currently executing module.
static HMODULE GetCurrentModuleHandle() {
  // pass a pointer to the current function
  return GetModuleHandleFromAddress(GetCurrentModuleHandle);
}

static bool IsAddressInCurrentModule(void *address) {
  return GetCurrentModuleHandle() == GetModuleHandleFromAddress(address);
}

static bool FilterCallback(void *context,
                           EXCEPTION_POINTERS *exinfo,
                           MDRawAssertionInfo *assertion) {
  ExceptionManager* this_ptr = reinterpret_cast<ExceptionManager*>(context);
  if (this_ptr->catch_entire_process())
    return true;

  if (!exinfo)
    return true;

  return IsAddressInCurrentModule(exinfo->ExceptionRecord->ExceptionAddress);
}

// Is called by Breakpad when an exception occurs and a minidump has been
// written to disk.
static bool MinidumpCallback(const wchar_t *minidump_folder,
                             const wchar_t *minidump_id,
                             void *context,
                             EXCEPTION_POINTERS *exinfo,
                             MDRawAssertionInfo *assertion,
                             bool succeeded) {
  bool handled_exception;
  ExceptionManager *this_ptr = reinterpret_cast<ExceptionManager*>(context);
  if (this_ptr->catch_entire_process()) {
    // Tell Windows we handled the exception so that the user doesn't see a
    // crash dialog.
    handled_exception = true;
  } else {
    // Returning false makes Breakpad behave as though it didn't handle the
    // exception.  This allows the browser to display its crash dialog instead
    // of abruptly terminating.  This is what some Google apps do when they're
    // inside a browser.
    handled_exception = false;
  }

  // rundll32 is a convenient way to send a minidump from an extension DLL.
  // Another option is to bundle an executable that uploads a given minidump.
  wchar_t module_path[MAX_PATH];  // folder + filename
  if (0 == GetModuleFileNameW(GetCurrentModuleHandle(),
                              module_path, MAX_PATH)) {
    return handled_exception;
  }

  // get a version without spaces, to use it as a command line argument
  wchar_t module_short_path[MAX_PATH];
  if (0 == GetShortPathNameW(module_path, module_short_path, MAX_PATH)) {
    return handled_exception;
  }

  // construct the minidump path the same way
  wchar_t minidump_path[MAX_PATH];
  _snwprintf(minidump_path, sizeof(minidump_path), L"%s\\%s.dmp",
             minidump_folder, minidump_id);

  wchar_t minidump_short_path[MAX_PATH];
  if (0 == GetShortPathNameW(minidump_path, minidump_short_path, _MAX_PATH)) {
    return handled_exception;
  }

  // execute the rundll32 command
  std::wstring command;
  command += module_short_path;
  command += L",HandleMinidump ";
  command += minidump_short_path;
  ShellExecuteW(NULL, NULL, L"rundll32", command.c_str(), L"", 0);

  return handled_exception;
}

void ExceptionManager::StartMonitoring() {
  if (exception_handler_) { return; }  // don't init more than once

  wchar_t temp_path[MAX_PATH];
  if (!GetTempPathW(MAX_PATH, temp_path)) { return; }

  exception_handler_ = new google_breakpad::ExceptionHandler(temp_path,
                                                             FilterCallback,
                                                             MinidumpCallback,
                                                             this, true);
}

// static
bool ExceptionManager::ReportAndContinue() {
  if (!instance_ || !instance_->exception_handler_) {
    return false;
  }

  // Pass parameters to WriteMinidump so the reported call stack ends here,
  // instead of including all frames down to where the dump file gets written.
  //
  // This requires a valid EXCEPTION_POINTERS struct.  GetExceptionInformation()
  // can generate one for us.  But that function can only be used in an __except
  // filter statement.  And the value returned only appears to be valid for the
  // lifetime of the filter statement.  Hence the comma-separated statement
  // below, which is actually common practice.
  bool retval;
  google_breakpad::ExceptionHandler *h = instance_->exception_handler_;

  __try {
    int *null_pointer = NULL;
    *null_pointer = 1;
  } __except (retval = h->WriteMinidump(GetExceptionInformation(), NULL),
              EXCEPTION_EXECUTE_HANDLER) {
    // EXCEPTION_EXECUTE_HANDLER causes execution to continue here.
    // We have nothing more to do, so just continue normally.
  }
  return retval;
}


void ExceptionManager::SendMinidump(const char *minidump_filename) {
  if (CanSendMinidump()) {
    map<std::wstring, std::wstring> parameters;
    parameters[kCrashReportProductParam] = kCrashReportProductName;
    parameters[kCrashReportVersionParam] = kCrashReportProductVersion;

    std::string  minidump_str(minidump_filename);
    std::wstring minidump_wstr(minidump_str.begin(), minidump_str.end());

    for (int i = 0; i < kCrashReportAttempts; ++i) {
      ReportResult result = CrashReportSender::SendCrashReport(kCrashReportUrl,
                                                               parameters,
                                                               minidump_wstr,
                                                               NULL);
      if (result == RESULT_FAILED) {
        Sleep(kCrashReportResendPeriodMs);
      } else {
        // RESULT_SUCCEEDED or RESULT_REJECTED
        break;
      }
    }
  }

  DeleteFileA(minidump_filename);
}


// This is the function that rundll32 calls to upload a minidump.  Uses
// extern "C" so we can pass the unmangled function name to ShellExecute.
extern "C"
__declspec(dllexport) void __cdecl HandleMinidump(HWND window,
                                                  HINSTANCE instance,
                                                  LPSTR command_line,
                                                  int command_show) {
  ExceptionManager::SendMinidump(command_line); 
}
