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

#ifdef OS_MACOSX
#include "gears/desktop/file_dialog_osx.h"

#include <Carbon/Carbon.h>

#include <algorithm>
#include <sys/syslimits.h>

#include "gears/base/common/string_utils.h"
#include "gears/base/common/common.h"
#include "gears/base/safari/scoped_cf.h"

FileDialogCarbon::FileDialogCarbon(bool multiselect)
    : multiselect_(multiselect) {
}

FileDialogCarbon::~FileDialogCarbon() {
}

// A filter to use with callbacks. Does filter matching and conversion to the
// correct format.
class CarbonFilter {
 public:
  CarbonFilter() {
  }

  ~CarbonFilter() {
  }

  CarbonFilter(const CarbonFilter& other)
      : description_(other.description_), filters_(other.filters_) {
  }

  CarbonFilter& operator=(const CarbonFilter& other) {
    if (this != &other) {
      description_ = other.description_;
      filters_ = other.filters_;
    }
    return *this;
  }

  bool init(const std::string16& description, const std::string16& filter) {
    if (!String16ToUTF8(description.c_str(), &description_))
      return false;

    std::string filter_8;
    if (!String16ToUTF8(filter.c_str(), &filter_8))
      return false;

    static const std::string kDelimeter(";");
    return Tokenize(filter_8, kDelimeter, &filters_) != 0;
  }

  const std::string& description() const {
    return description_;
  }

  const std::vector<std::string>& filters() const {
    return filters_;
  }

  bool Match(const std::string& file) const {
    for (std::vector<std::string>::const_iterator it = filters_.begin();
         it != filters_.end(); ++it) {
      if (StringMatch(file.c_str(), it->c_str()))
        return true;
    }
    return false;
  }

 private:
  std::string description_;  // display name
  std::vector<std::string> filters_;  // tokenized filter string
};

// Holds filters and current selection for use during callbacks.
class Filters {
 public:
  Filters() : selected_(0) {
  }

  ~Filters() {
  }

  typedef std::vector<CarbonFilter> CarbonFilterList;
  typedef CarbonFilterList::const_iterator const_iterator;

  const_iterator begin() const {
    return filters_.begin();
  }

  const_iterator end() const {
    return filters_.end();
  }

  size_t empty() const {
    return filters_.empty();
  }

  size_t size() const {
    return filters_.size();
  }

  // returns NULL when the selection is invalid
  const CarbonFilter* SelectedFilter() const {
    if (SelectionValid(selected_)) {
      return &filters_[selected_];
    } else {
      return NULL;
    }
  }

  bool Add(const std::string16& description, const std::string16& filter) {
    CarbonFilter new_filter;
    if (new_filter.init(description, filter)) {
      filters_.push_back(new_filter);
      return true;
    } else {
      return false;
    }
  }

  size_t selected() const {
    return selected_;
  }

  bool set_selected(const size_t selected) {
    if (SelectionValid(selected)) {
      selected_ = selected;
      return true;
    } else {
      return false;
    }
  }

 private:
  bool SelectionValid(const size_t selected) const {
    return selected < filters_.size();
  }

  CarbonFilterList filters_;
  size_t selected_;

  DISALLOW_EVIL_CONSTRUCTORS(Filters);
};

// Add filters to an open file dialog.
static bool AddFilters(const std::vector<FileDialog::Filter>& filters,
                       Filters* data,
                       NavDialogCreationOptions* dialog_options,
                       std::string16* error) {
  // convert to format easy to deal with while using carbon
  typedef std::vector<FileDialog::Filter>::const_iterator FilterIter;
  for (FilterIter it = filters.begin(); it != filters.end(); ++it) {
    if (!data->Add(it->description, it->filter)) {
      *error = STRING16(L"Failed to convert filter.");
      return false;
    }
  }

  if (data->empty()) {
    *error = STRING16(L"Failed to convert any filters.");
    return false;
  }

  // add filters to dialog
  scoped_cftype<CFMutableArrayRef> popup(CFArrayCreateMutable(
      kCFAllocatorDefault, data->size(), &kCFTypeArrayCallBacks));
  if (!popup.get()) {
    *error = STRING16(L"Failed to create CFArrayCreateMutable.");
    return false;
  }

  for (Filters::const_iterator it = data->begin(); it != data->end(); ++it) {
    scoped_cftype<CFStringRef> name(CFStringCreateWithCString(
        kCFAllocatorDefault, it->description().c_str(), kCFStringEncodingUTF8));
    if (!name.get()) {
      *error = STRING16(L"Failed to create CFStringRef.");
      return false;
    }
    // TODO(cdevries): Determine if name needs to be cleaned up later, after
    //                 the dialog has been used (outside this function).
    CFArrayAppendValue(popup.get(), name.release());
  }

  // TODO(cdevries): Determine if name needs to be cleaned up later, after
  //                 the dialog has been used (outside this function).
  dialog_options->popupExtension = popup.release();
  return true;
}

// Process events from dialog to add filter selections or set selection.
static void EventCallBack(NavEventCallbackMessage message,
                          NavCBRecPtr parameters,
                          void* user_data) {
  Filters* data = reinterpret_cast<Filters*>(user_data);

  if (message == kNavCBStart) {
    // dialog is ready to be displayed
    const CarbonFilter* selected = data->SelectedFilter();
    if (selected) {
      // set the default selection to the current selection
      const std::string& description = selected->description();
      scoped_cftype<CFStringRef> cf_description(
          CFStringCreateWithCString(
              kCFAllocatorDefault, description.c_str(), kCFStringEncodingUTF8));
      if (!cf_description.get())
        return;
      NavMenuItemSpec menu_item;
      menu_item.version = kNavMenuItemSpecVersion;
      menu_item.menuCreator = 'GEAR';
      menu_item.menuType = data->selected();
      // menuItemName is a Str255 and only holds a 255 character pascal string.
      // ^ is substituted for characters that can not be converted to the
      // Mac Roman encoding.
      int length = std::min(CFStringGetLength(cf_description.get()),
          static_cast<CFIndex>(255));
      menu_item.menuItemName[0] = CFStringGetBytes(cf_description.get(),
          CFRangeMake(0, length), kTextEncodingMacRoman, '^', false,
          &(menu_item.menuItemName[1]), 255, NULL);
      if (0 == menu_item.menuItemName[0])
        return;
      NavCustomControl(parameters->context, kNavCtlSelectCustomType,
          &menu_item);
    }
  } else if (message == kNavCBPopupMenuSelect) {
    // a selection was made from the available filters
    NavMenuItemSpec* menu = reinterpret_cast<NavMenuItemSpec*>(
        parameters->eventData.eventDataParms.param);

    data->set_selected(menu->menuType);
  }
}

// returns true if the file/directory should be displayed
static Boolean FilterFileCallBack(AEDesc* theItem, void* info,
    void* user_data, NavFilterModes filterMode) {
  if (!user_data || !info)
    return true;

  NavFileOrFolderInfo* file_info = reinterpret_cast<NavFileOrFolderInfo*>(info);
  if (file_info->isFolder)
    return true;

  Filters* data = reinterpret_cast<Filters*>(user_data);
  if (data->empty())
    return true;

  AECoerceDesc(theItem, typeFSRef, theItem);
  FSRef ref;
  if (AEGetDescData(theItem, &ref, sizeof(FSRef)) == noErr) {
    char file_path_8[PATH_MAX];
    OSStatus status = FSRefMakePath(&ref,
        reinterpret_cast<UInt8*>(file_path_8), PATH_MAX);
    if (noErr != status)
      return false;

    const CarbonFilter* selected = data->SelectedFilter();
    if (!selected)
      return true;

    if (selected->Match(file_path_8))
      return true;
  }

  return false;
}

// Initialize an open file dialog to open multiple files.
static bool InitDialog(bool multiselect,
    const std::vector<FileDialog::Filter>& filters,
    scoped_NavDialogRef* dialog, Filters* data, std::string16* error) {
  NavDialogCreationOptions dialog_options;
  OSStatus status = NavGetDefaultDialogCreationOptions(&dialog_options);
  if (noErr != status) {
    *error = STRING16(L"Failed to create dialog options.");
    return false;
  }

  // set application wide modality and multiple file selections
  dialog_options.modality = kWindowModalityAppModal;
  if (multiselect) {
    dialog_options.optionFlags |= kNavAllowMultipleFiles;
  } else {
    dialog_options.optionFlags &= ~kNavAllowMultipleFiles;
  }

  if (!AddFilters(filters, data, &dialog_options, error))
    return false;

  NavDialogRef new_dialog = NULL;
  status = NavCreateGetFileDialog(&dialog_options, NULL, &EventCallBack, NULL,
               &FilterFileCallBack, data, &new_dialog);
  if (noErr != status) {
    *error = STRING16(L"Failed to create dialog.");
    return false;
  }

  dialog->reset(new_dialog);
  return true;
}

// Display a dialog to open multiple files and return the selected files.
static bool Display(NavDialogRef dialog,
    std::vector<std::string16>* selected_files,
    std::string16* error) {
  // display the dialog
  OSStatus status = NavDialogRun(dialog);
  if (noErr != status) {
    *error = STRING16(L"Failed to display dialog.");
    return false;
  }

  // get the user's input
  NavUserAction action = NavDialogGetUserAction(dialog);
  if (action == kNavUserActionNone || action == kNavUserActionCancel)
    return true;
  NavReplyRecord reply_record;
  status = NavDialogGetReply(dialog, &reply_record);
  if (noErr != status) {
    *error = STRING16(L"Failed to get the dialog reply.");
    return false;
  }

  // ensure the reply record is cleaned up after use
  scoped_NavReplyRecord free_reply_record(&reply_record);

  // get the files
  long file_count = -1;
  status = AECountItems(&reply_record.selection, &file_count);
  if (noErr != status) {
    *error = STRING16(L"Failed to get the number of selected files.");
    return false;
  }

  UInt8 file_path_8[PATH_MAX];

  for (long i = 1; i <= file_count; ++i) {
    // get current file
    FSRef file;
    status = AEGetNthPtr(&reply_record.selection, i, typeFSRef, NULL,
        NULL, &file, sizeof(FSRef), NULL);
    if (noErr != status) {
      *error = STRING16(L"Failed to get selected files.");
      return false;
    }

    // convert it to a string
    status = FSRefMakePath(&file, file_path_8, PATH_MAX);
    if (noErr != status) {
      *error = STRING16(L"Failed to get string of selected file.");
      return false;
    }
    std::string16 file_path_16;
    if (!UTF8ToString16(reinterpret_cast<const char*>(&file_path_8),
            &file_path_16)) {
      *error = STRING16(L"Failed to convert string to unicode.");
      return false;
    }
    selected_files->push_back(file_path_16);
  }

  return true;
}

bool FileDialogCarbon::OpenDialog(const std::vector<Filter>& filters,
    std::vector<std::string16>* selected_files,
    std::string16* error) {
  bool success = false;
  scoped_NavDialogRef dialog(NULL);
  Filters data;

  if ((success = InitDialog(multiselect_, filters, &dialog, &data, error))) {
    success = Display(dialog.get(), selected_files, error);
  }

  return success;
}

#endif  // OS_MACOSX
