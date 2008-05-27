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

#ifdef OFFICIAL_BUILD
// File picker is not ready for official builds
#else

#if defined(LINUX) && !defined(OS_MACOSX)

#include "gears/desktop/file_dialog_gtk.h"

#include <gtk/gtk.h>

#include "gears/base/common/string_utils.h"
#include "gears/desktop/file_dialog_utils.h"

FileDialogGtk::FileDialogGtk(GtkWindow* parent)
  : parent_(parent) {
}

FileDialogGtk::~FileDialogGtk() {
}

// Initialize a GTK dialog to open multiple files.
static bool InitDialog(GtkWindow* parent,
                       GtkFileChooser** dialog,
                       std::string16* error) {
  *dialog = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("Open File",
      parent, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
      GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL));
  if (!*dialog) {
    *error = STRING16(L"Failed to create dialog.");
    return false;
  }

  if (parent && parent->group) {
    gtk_window_group_add_window(parent->group, GTK_WINDOW(*dialog));
  }
  gtk_file_chooser_set_select_multiple(*dialog, TRUE);
  return true;
}

// Add filters to a GTK open file dialog.
static bool AddFilters(const std::vector<FileDialog::Filter>& filters,
                       GtkFileChooser* dialog,
                       std::string16* error) {
  // process filter pairs
  typedef std::vector<FileDialog::Filter>::const_iterator FILTER_ITER;
  for (FILTER_ITER it = filters.begin(); it != filters.end(); ++it) {
    // convert strings to UTF8 for GTK
    std::string description, filter;
    if (!String16ToUTF8(it->description.c_str(), &description)
        || !String16ToUTF8(it->filter.c_str(), &filter)) {
      return false;
    }

    std::vector<std::string> tokens;
    const std::string delimiter(";");
    if (0 == Tokenize(filter, delimiter, &tokens)) {
      *error = STRING16(L"Failed to tokenize filter.");
      return false;
    }

    // create filter
    GtkFileFilter* gtk_filter = gtk_file_filter_new();
    if (!gtk_filter) {
      *error = STRING16(L"Failed to create filter.");
      return false;
   }

    // add filter
    gtk_file_filter_set_name(gtk_filter, description.c_str());
    for (std::vector<std::string>::const_iterator it = tokens.begin();
        it != tokens.end(); ++it) {
      gtk_file_filter_add_pattern(gtk_filter, it->c_str());
    }
    gtk_file_chooser_add_filter(dialog, gtk_filter);
  }

  return true;
}

// Display a GTK open file dialog and return the files selected by the user.
static bool Display(GtkFileChooser* dialog,
                    std::vector<std::string16>* selected_files,
                    std::string16* error) {
  // display dialog and process selection
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    GSList* files = gtk_file_chooser_get_filenames(dialog);
    if (!files) {
      *error = STRING16(L"Failed to get selected files from dialog.");
      return false;
    }

    std::string16 file;
    for (GSList* curr = files; curr != NULL; curr = curr->next) {
      if (UTF8ToString16(static_cast<const char*>(curr->data), &file)) {
        selected_files->push_back(file);
      } else {
        *error = STRING16(L"Failed to convert string to unicode.");
        g_slist_free(files);
        return false;
      }
    }

    g_slist_free(files);
  }

  return true;
}

bool FileDialogGtk::OpenDialog(const std::vector<Filter>& filters,
                               std::vector<std::string16>* selected_files,
                               std::string16* error) {
  bool success = false;
  GtkFileChooser* dialog = NULL;

  if ((success = InitDialog(parent_, &dialog, error))) {
    if ((success = AddFilters(filters, dialog, error))) {
      success = Display(dialog, selected_files, error);
    }
  }

  if (dialog)
    gtk_widget_destroy(GTK_WIDGET(dialog));

  return success;
}

#endif  // defined(LINUX) && !defined(OS_MACOSX)

#endif  // OFFICIAL_BUILD
