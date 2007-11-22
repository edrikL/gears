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
//
// Implementation of Windows specific functions that don't exist in Windows
// Mobile 5.

// TODO(andreip): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented
#ifdef WINCE
#include "gears/base/common/wince_compatibility.h"

#include <shellapi.h>

#include "gears/base/common/common.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/string_utils.h"

// Used by SHCreateDirectoryEx.
static void SkipTokens(const std::string16 &path,
                       int &pos,
                       bool skip_separators);

// GetSystemTimeAsFileTime does not exist on Windows Mobile,
// so we implement it here by getting the system time and
// then converting it to file time.
void GetSystemTimeAsFileTime(LPFILETIME filetime) {
  SYSTEMTIME systemtime;
  GetSystemTime(&systemtime);
  SystemTimeToFileTime(&systemtime, filetime);
}

// There seem to be no way to implement this properly on Windows Mobile
// since the algorithm for path shortening isn't fully specified, according
// to http://en.wikipedia.org/wiki/8.3_filename. Using FindFirstFileA isn't
// an alternative either since that method isn't exported by coredll.lib.
// FindFirstFileW uses a different structure to return the file information
// and that structure is missing exactly the cAlternateFilename field, which
// would have been the one that contained the short name.
DWORD GetShortPathNameW(LPCTSTR path_long,
                        LPTSTR path_short,
                        DWORD path_short_max_size) {
 int long_path_size = wcslen(path_long) + 1;  // +1 for the ending \0
 if (long_path_size <= static_cast<int>(path_short_max_size)) {
   wcsncpy(path_short, path_long, path_short_max_size);
   return long_path_size - 1;
 } else {
   return long_path_size;
 }
}

// This function creates a file system folder whose fully qualified
// path is given by full_dirpath. If one or more of the intermediate folders
// do not exist, they are created as well.
// According to http://msdn2.microsoft.com/en-us/library/aa365247.aspx
// Windows API functions accept both "\" and "/", so we will do the same.
int SHCreateDirectoryEx(HWND window,
                        LPCTSTR full_dirpath,
                        const SECURITY_ATTRIBUTES *security_attributes) {
  std::string16 path(full_dirpath);
  if (!path.length()) return ERROR_BAD_PATHNAME;

  // Traverse the path and create the directories one by one.
  int pos = 0;
  // Skip leading separators.
  SkipTokens(path, pos, true);
  while (pos < static_cast<int>(path.length())) {
    // Find next separator.
    SkipTokens(path, pos, false);
    // Skip next consecutive separators, if any.
    SkipTokens(path, pos, true);
    // Create the directory.
    if (!CreateDirectory(path.substr(0, pos).c_str(), NULL)) {
      DWORD error = GetLastError();
      if (error != ERROR_ALREADY_EXISTS) return error;
    }
  }
  return GetLastError();
}

HRESULT SHGetFolderPath(HWND hwndOwner,
                        int nFolder,
                        HANDLE hToken,
                        DWORD dwFlags,
                        LPTSTR pszPath) {
  return SHGetSpecialFolderPath(hwndOwner, pszPath, nFolder, false);
}

// Internal

static bool IsSeparator(const char16 token) {
  static const char16 kPathSeparatorAlternative = L'/';
  return ((token == kPathSeparator) || (token == kPathSeparatorAlternative));
}

// Skips tokens of the given type (separators or non-separators).
static void SkipTokens(const std::string16 &path,
                       int &pos,
                       bool skip_separators) {
  while (pos < static_cast<int>(path.length()) &&
        (IsSeparator(path[pos]) == skip_separators)) {
    pos++;
  }
}
#endif  // WINCE
