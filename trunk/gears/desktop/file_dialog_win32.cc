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

#if defined(WIN32)

#include "gears/desktop/file_dialog_win32.h"

#include "gears/base/common/string_utils.h"

FileDialogWin32::FileDialogWin32(HWND parent)
  : parent_(parent) {
}

FileDialogWin32::~FileDialogWin32() {
}

// Initialize an open file dialog to open multiple files.
static void InitDialog(HWND parent,
                       OPENFILENAME* ofn,
                       std::vector<TCHAR>* file_buffer) {
  const static int kFileBufferSize = 32768;
  file_buffer->resize(kFileBufferSize);

  // Initialize OPENFILENAME
  memset(ofn, 0, sizeof(*ofn));
  ofn->lStructSize = sizeof(*ofn);
  ofn->hwndOwner = parent;
  ofn->lpstrFile = &file_buffer->at(0);
  ofn->lpstrFile[0] = '\0';
  ofn->lpstrFilter = L"All Files\0*.*\0\0";
  ofn->nFilterIndex = 1;
  ofn->nMaxFile = kFileBufferSize;
  ofn->lpstrFileTitle = NULL;
  ofn->nMaxFileTitle = 0;
  ofn->lpstrInitialDir = NULL;
  ofn->Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT
              | OFN_EXPLORER;

}

static void AddToBuffer(const std::string16& str,
                        std::vector<TCHAR>* buffer) {
  buffer->insert(buffer->end(), str.begin(), str.end());
}

// Add filters to an open file dialog.
static bool AddFilters(const std::vector<FileDialog::Filter>& filters,
                       OPENFILENAME* ofn,
                       std::vector<TCHAR>* filter_buffer,
                       std::string16* error) {
  // process filter pairs
  typedef std::vector<FileDialog::Filter>::const_iterator FILTER_ITER;
  for (FILTER_ITER it = filters.begin(); it != filters.end(); ++it) {
    AddToBuffer(it->description, filter_buffer);
    filter_buffer->push_back('\0');
    AddToBuffer(it->filter, filter_buffer);
    filter_buffer->push_back('\0');
  }

  // empty string terminates the list
  filter_buffer->push_back('\0');

  // set filter
  ofn->lpstrFilter = &filter_buffer->at(0);
  ofn->nFilterIndex = 1;
  return true;
}

// Process the selection from GetOpenFileName.
// Parameters:
//  selection - in - The string from OPENFILENAME member lpstrFile after
//                   calling ::GetOpenFileName().
//
//  selected_files - out - The files selected by the user are placed in here.
//
static bool ProcessSelection(const TCHAR* selection,
                             std::vector<std::string16>* selected_files,
                             std::string16* error) {
  std::vector<std::string16> temp_files;
  const TCHAR* p = selection;
  while (*p) { // empty string indicates end of list
    temp_files.push_back(p);
    // move to next string
    // + 1 is for NULL terminator
    p += temp_files.back().length() + 1;
  }

  if (temp_files.empty()) {
    *error = STRING16(L"Selection contained no files. A minimum of one was "
                      L"expected.");
    return false;
  } else if (temp_files.size() == 1) {
    // if there is one string, the user selected a single file
     *selected_files = temp_files;
  } else {
    assert(temp_files.size() >= 2);

    // the first string is the path
    // all following strings are files at that path
    std::vector<std::string16>::iterator itPath = temp_files.begin();
    for (std::vector<std::string16>::iterator itFile = ++(temp_files.begin());
      itFile != temp_files.end(); ++itFile) {
        selected_files->push_back(*itPath + L"\\" + *itFile);
    }
  }
  return true;
}

// Display a dialog to open multiple files and return the selected files.
static bool Display(OPENFILENAME* ofn,
                    std::vector<std::string16>* selected_files,
                    std::string16* error) {
  if (::GetOpenFileName(ofn)) {
    return ProcessSelection(ofn->lpstrFile, selected_files, error);
  } else {
    // user selected cancel
    return true;
  }
}

bool FileDialogWin32::OpenDialog(const std::vector<Filter>& filters,
                                 std::vector<std::string16>* selected_files,
                                 std::string16* error) {
  OPENFILENAME ofn;
  std::vector<TCHAR> file_buffer;
  InitDialog(parent_, &ofn, &file_buffer);

  std::vector<TCHAR> filter_buffer;
  if (!AddFilters(filters, &ofn, &filter_buffer, error))
    return false;

  return Display(&ofn, selected_files, error);
}

#endif  // WIN32
