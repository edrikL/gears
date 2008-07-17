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

#if defined(LINUX) && !defined(OS_MACOSX)

#include "gears/desktop/file_dialog_gtk.h"

#include "gears/base/common/string_utils.h"
#include "gears/desktop/file_dialog.h"

namespace {

// TODO(bpm): Localize
const char* kDialogTitle = "Open File";

// TODO(bpm): Localize and unify with other copies of these strings for other
// platforms.
const char* kDefaultFilterLabel = "All Readable Documents";
const char* kAllDocumentsLabel = "All Documents";

// Degenerate filter callbacks which allow any file to be selected.
gboolean AnyFileFilter(const GtkFileFilterInfo* /*filter_info*/,
                       gpointer /*data*/) {
  return TRUE;
}

void AnyFileFilterDestroy(gpointer /*data*/) {
}

void ResponseHandler(GtkWidget* dialog, gint response_id, gpointer data) {
  FileDialogGtk* file_dialog = static_cast<FileDialogGtk*>(data);
  file_dialog->HandleResponse(response_id);
}

#if 0  // TODO(bpm): Determine if we'd ever need these.
void UnmapHandler(GtkWidget* dialog, gpointer data) {
  FileDialogGtk* file_dialog = static_cast<FileDialogGtk*>(data);
  file_dialog->HandleClose();
}

gint DeleteHandler(GtkWidget* dialog, GdkEventAny* event, gpointer data) {
  FileDialogGtk* file_dialog = static_cast<FileDialogGtk*>(data);
  file_dialog->HandleClose();
  return TRUE;  // Prevent destruction
}

void DestroyHandler(GtkWidget* dialog, gpointer data) {
  // TODO: Determine if this is called in addition to unmap.
  FileDialogGtk* file_dialog = static_cast<FileDialogGtk*>(data);
  file_dialog->HandleClose();
}
#endif

}  // anonymous namespace

FileDialogGtk::FileDialogGtk(const ModuleImplBaseClass* module,
                             GtkWindow* parent)
    : FileDialog(module), parent_(parent) {
}

FileDialogGtk::~FileDialogGtk() {
}

void FileDialogGtk::HandleResponse(gint response_id) {
  StringList selected_files;
  std::string16 error;
  if (GTK_RESPONSE_ACCEPT == response_id) {
    if (!ProcessSelection(&selected_files, &error)) {
      HandleError(error);
    }
  }
  CompleteSelection(selected_files);
}

void FileDialogGtk::HandleClose() {
  StringList selected_files;
  CompleteSelection(selected_files);
}

bool FileDialogGtk::BeginSelection(const FileDialog::Options& options,
                                   std::string16* error) {
  if (!InitDialog(parent_, options, error))
    return false;
  if (!SetFilter(options.filter, error))
    return false;
  if (!Display(error))
    return false;
  return true;
}

void FileDialogGtk::CancelSelection() {
  // TODO(bpm): Nothing calls CancelSelection yet, but it might someday.
}

bool FileDialogGtk::InitDialog(GtkWindow* parent,
                               const FileDialog::Options& options,
                               std::string16* error) {
  dialog_.reset(gtk_file_chooser_dialog_new(kDialogTitle, parent,
                                            GTK_FILE_CHOOSER_ACTION_OPEN,
                                            GTK_STOCK_CANCEL,
                                            GTK_RESPONSE_CANCEL,
                                            GTK_STOCK_OPEN,
                                            GTK_RESPONSE_ACCEPT,
                                            NULL));
  if (!dialog_.get()) {
    *error = STRING16(L"Failed to create dialog.");
    return false;
  }
  if (parent && parent->group) {
    gtk_window_group_add_window(parent->group, GTK_WINDOW(dialog_.get()));
  }
  bool multipleFiles = (MULTIPLE_FILES == options.mode);
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog_.get()),
                                       multipleFiles ? TRUE : FALSE);
  return true;
}

bool FileDialogGtk::SetFilter(const StringList& filter, std::string16* error) {
  if (!filter.empty()) {
    GtkFileFilter* gtk_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(gtk_filter, kDefaultFilterLabel);
    for (size_t i = 0; i < filter.size(); ++i) {
      std::string filter_item;
      if (!String16ToUTF8(filter[i].c_str(), &filter_item))
        continue;
      if ('.' == filter_item[0]) {
        std::string pattern("*");
        pattern.append(filter_item);
        gtk_file_filter_add_pattern(gtk_filter, pattern.c_str());
      } else {
        gtk_file_filter_add_mime_type(gtk_filter, filter_item.c_str());
      }
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog_.get()), gtk_filter);
  }

  GtkFileFilter* gtk_filter = gtk_file_filter_new();
  gtk_file_filter_set_name(gtk_filter, kAllDocumentsLabel);
  gtk_file_filter_add_custom(gtk_filter, static_cast<GtkFileFilterFlags>(0),
                             &AnyFileFilter, NULL, &AnyFileFilterDestroy);
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog_.get()), gtk_filter);
  return true;
}

bool FileDialogGtk::Display(std::string16* error) {
  g_signal_connect(dialog_.get(), "response", G_CALLBACK(ResponseHandler),
                   this);
#if 0
  // It appears these handlers are unnecessary.  I can't find a way to close
  // the dialog that doesn't trigger "response".
  g_signal_connect(dialog_.get(), "unmap", G_CALLBACK(UnmapHandler), this);
  g_signal_connect(dialog_.get(), "delete-event", G_CALLBACK(DeleteHandler),
                   this);
  g_signal_connect(dialog_.get(), "destroy", G_CALLBACK(DestroyHandler), this);
#endif
  gtk_widget_show_all(dialog_.get());
  return true;
}

bool FileDialogGtk::ProcessSelection(StringList* selected_files,
                                     std::string16* error) {
  GSList* files =
      gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog_.get()));
  if (!files) {
    *error = STRING16(L"Failed to get selected files from dialog.");
    return false;
  }

  std::string16 file;
  for (GSList* curr = files; curr != NULL; curr = curr->next) {
    if (UTF8ToString16(static_cast<const char*>(curr->data), &file)) {
      selected_files->push_back(file);
    } else {
      // Failed to convert string to unicode... ignore it.
    }
    g_free(curr->data);
  }
  g_slist_free(files);
  return true;
}

#endif  // defined(LINUX) && !defined(OS_MACOSX)
