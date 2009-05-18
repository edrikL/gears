// Copyright 2009, Google Inc.
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

#include <set>
#include <windows.h>
#include <shlobj.h>

#include "gears/desktop/drag_and_drop_utils_cr.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/mime_detect.h"
#include "gears/base/chrome/module_cr.h"
#include "gears/desktop/file_dialog.h"

// Global drag state

static std::set<std::string16> g_mime_set;
static std::set<std::string16> g_type_set;
static std::vector<std::string16> g_mimes;
static std::vector<std::string16> g_files;
static double g_file_total_bytes = 0;
static size_t g_drag_files = 0;
static int32 g_identity = 0;

// DragSession

class DragSession {
 public:
  DragSession(NPP npp, JsObject *event)
      : identity_(0), event_id_(0), type_(NULL), data_(NULL) {
    event_ = event ? NPVARIANT_TO_OBJECT(event->token()) : NULL;
    context_ = CP::GetBrowsingContext(npp);
  }

  bool GetDragType() {
    if (!BrowserSupportsDragDrop())
      return false;
    assert(!type_ && !data_);
    browser().get_drag_data(g_cpid, context_, event_, false,
        /* out parameters */ &identity_, &event_id_, &type_, &data_);
    return identity_ && type_;
  }

  bool GetDragData() {
    if (!BrowserSupportsDragDrop())
      return false;
    ReleaseTypeAndData();
    browser().get_drag_data(g_cpid, context_, event_, true,
        /* out parameters */ &identity_, &event_id_, &type_, &data_);
    return identity_ && data_;
  }

  const int32 identity() const { return identity_; }
  const int32 event_id() const { return event_id_; }

  const NPUTF8 *type() const {
    return identity_ && type_ ? type_ : const_cast<NPUTF8*>("");
  }

  const NPUTF8 *data() const {
    return identity_ && data_ ? data_ : const_cast<NPUTF8*>("");
  }

  bool SetDropEffect(int effect) {
    if (!BrowserSupportsDragDrop())
      return false;
    if (browser().set_drop_effect(g_cpid, context_, event_, effect))
      return false;  // Failed.
    return true;
  }

  ~DragSession() {
    ReleaseTypeAndData();
  }

  static bool BrowserSupportsDragDrop() {
    static const CPB_GetDragDataFunc kFunc(browser().get_drag_data);
    return kFunc != NULL;
  }

 private:
  static const CPBrowserFuncs &browser() {
    static const CPBrowserFuncs &kBrowser(CP::browser_funcs());
    return kBrowser;
  }

  void ReleaseTypeAndData() {
    if (type_) browser().free(type_);
    if (data_) browser().free(data_);
    type_ = data_ = NULL;
  }

  CPBrowsingContext context_;
  NPObject *event_;
  int32 identity_;
  int32 event_id_;
  NPUTF8 *type_;
  NPUTF8 *data_;

  DISALLOW_EVIL_CONSTRUCTORS(DragSession);
};

// FileAttributes

class FileAttributes {
 public:
  explicit FileAttributes(const std::string16 &path) {
    if (!::lstrcpyn(path_, path.c_str(), MAX_PATH))
      path_[0] = static_cast<WCHAR>(0);  // Fail: path exceeds MAX_PATH
    ResolvePath(path_);
  }

  bool Found() const {
    if (find_handle_ != INVALID_HANDLE_VALUE)
      return attributes_ & SFGAO_CANCOPY;
    return false;
  }

  bool IsDirectory() const {
    return !!(file_find_data_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  }

  ULONGLONG GetFileSize() const {
    const ULARGE_INTEGER size =
      { file_find_data_.nFileSizeLow, file_find_data_.nFileSizeHigh };
    return size.QuadPart;
  }

  const WCHAR *GetBaseName() const {
    return file_find_data_.cFileName;
  }

  const WCHAR *GetFileExtension() const {
    return ::PathFindExtension(GetBaseName());
  }

  const WCHAR *GetFilePath() const {
    return path_;
  }

  ~FileAttributes() {
    ::FindClose(find_handle_);
  }

 private:
  void ResolvePath(WCHAR *path) {
    LOG16((L"%s\n", path));

    find_handle_ = ::FindFirstFile(path, &file_find_data_);
    attributes_ = 0;

    if (find_handle_ != INVALID_HANDLE_VALUE) {
      if (ResolveAttributes(path)) {
        ResolveLinks(path);
      } else {
        ::FindClose(find_handle_);
        find_handle_ = INVALID_HANDLE_VALUE;
        attributes_ = 0;
      }
    }
  }

  DWORD ResolveAttributes(const WCHAR *path) {
    SHFILEINFO info;
    if (::SHGetFileInfo(path, 0, &info, sizeof(info), SHGFI_ATTRIBUTES))
      return (attributes_ = info.dwAttributes) & SFGAO_FILESYSTEM;
    return attributes_ = 0;
  }

  void ResolveLinks(WCHAR *path) {
    if (attributes_ & SFGAO_LINK) {
      if (FAILED(ResolveShortcut(path, path))) {
        LOG16((L" link resolve failed\n"));
      } else {
        ::FindClose(find_handle_);
        ResolvePath(path);
      }
    }
  }

  static HRESULT ResolveShortcut(const WCHAR *shortcut, WCHAR *target) {
    LOG16((L" resolving %s\n", shortcut));

    static CComPtr<IShellLink> link;
    if (!link && FAILED(link.CoCreateInstance(CLSID_ShellLink)))
      return E_FAIL;

    static CComQIPtr<IPersistFile> file(link);
    if (!file) return E_FAIL;

    // Open the shortcut file, load its content.
    HRESULT hr = file->Load(shortcut, STGM_READ);
    if (FAILED(hr)) return hr;

    // Get the shortcut target path.
    hr = link->GetPath(target, MAX_PATH, 0, SLGP_UNCPRIORITY);
    if (FAILED(hr)) return hr;

    return S_OK;
  }

 private:
  WCHAR path_[MAX_PATH];
  WIN32_FIND_DATA file_find_data_;
  HANDLE find_handle_;
  DWORD attributes_;

  DISALLOW_EVIL_CONSTRUCTORS(FileAttributes);
};

static bool FindDragFiles(JsRunnerInterface *runner, DragSession *drag) {
  if (std::string(drag->type()) != "Files")
    return false;

  // "Say" there's 1 drag file until we know more; extract the files.
  g_drag_files = 1;
  if (!drag->GetDragData())
    return false;
  static const std::string16 kDelimiter(STRING16(L"\b"));
  std::vector<std::string16> files;
  if (!Tokenize(UTF8ToString16(drag->data()), kDelimiter, &files))
    return false;

  LOG16((L"Dragging %lu files\n", files.size()));
  g_drag_files = files.size();
  g_file_total_bytes = 0;
  g_type_set.clear();
  g_mime_set.clear();
  g_mimes.clear();
  g_files.clear();

  for (size_t i = 0; i < g_drag_files; ++i) {
    FileAttributes file(files[i]);
    if (!file.Found() || file.IsDirectory())
      return false;

    LOG16((L" found %s\n", file.GetBaseName()));
    g_type_set.insert(file.GetFileExtension());
    g_file_total_bytes += file.GetFileSize();
    g_files.push_back(file.GetFilePath());

    g_mimes.push_back(DetectMimeTypeOfFile(g_files.back()));
    LOG16((L" mimeType %s\n", g_mimes.back().c_str()));
    g_mime_set.insert(g_mimes.back());
  }

  assert(g_files.size() == g_mimes.size());
  return true;
}

// AddFileMetaData() helper.

static bool ToJsArray(std::set<std::string16> const &s, JsArray *array) {
  if (!array) return false;

  std::set<std::string16>::const_iterator it = s.begin();
  for (int index = 0; it != s.end(); ++it) {
    if (!it->empty() && !array->SetElementString(index++, *it))
      return false;
  }

  return true;
}

static bool AddFileMetaData(JsRunnerInterface *runner, JsObject *context) {
  assert(g_drag_files);

  static const std::string16 kCount(STRING16(L"count"));
  if (!context->SetPropertyInt(kCount, static_cast<int32>(g_files.size())))
    return false;

  if (g_files.empty())  // But there are no files we can drop.
    return true;

  static const std::string16 kBytes(STRING16(L"totalBytes"));
  if (!context->SetPropertyDouble(kBytes, g_file_total_bytes))
    return false;

  static const std::string16 kExtensions(STRING16(L"extensions"));
  scoped_ptr<JsArray> extensions_array(runner->NewArray());
  if (!ToJsArray(g_type_set, extensions_array.get()))
    return false;
  if (!context->SetPropertyArray(kExtensions, extensions_array.get()))
    return false;

  static const std::string16 kMimes(STRING16(L"mimeTypes"));
  scoped_ptr<JsArray> mime_array(runner->NewArray());
  if (!ToJsArray(g_mime_set, mime_array.get()))
    return false;
  if (!context->SetPropertyArray(kMimes, mime_array.get()))
    return false;

  return true;
}

void SetDragCursor(ModuleEnvironment *module_environment, JsObject *event,
                   DragAndDropCursorType cursor, std::string16 *error) {
  JsRunnerInterface *runner = module_environment->js_runner_;

  DragSession drag(runner->GetContext(), event);
  if (!drag.SetDropEffect(cursor)) {
    if (drag.BrowserSupportsDragDrop()) {
      *error = STRING16(L"The drag-and-drop event is invalid.");
    }
  }
}

bool GetDragData(ModuleEnvironment *module_environment, JsObject *event,
                 JsObject *data, std::string16 *error) {
  JsRunnerInterface *runner = module_environment->js_runner_;

  DragSession drag(runner->GetContext(), event);
  if (!drag.GetDragType()) {
    if (drag.BrowserSupportsDragDrop()) {
      *error = STRING16(L"The drag-and-drop event is invalid.");
    }
    return false;
  }

  if (!g_identity || (g_identity != drag.identity())) {
    g_identity = drag.identity();
    g_drag_files = 0;

    if (!g_identity || !FindDragFiles(runner, &drag)) {
      g_file_total_bytes = 0;
      g_mime_set.clear();
      g_type_set.clear();
      g_files.clear();
      g_mimes.clear();
    }
  }

  if (g_drag_files && !AddFileMetaData(runner, data))
    *error = STRING16(L"Failed adding file meta data.");

  if (drag.event_id() != DRAG_AND_DROP_EVENT_DROP)
    return g_drag_files > 0;
  if (!error->empty())
    return g_drag_files > 0;

  assert(drag.event_id() == DRAG_AND_DROP_EVENT_DROP);
  if (!g_drag_files) {
    return false;  // There are no files.
  }

  scoped_ptr<JsArray> array(runner->NewArray());
  if (!array.get()) {
    *error = STRING16(L"Failed creating javascript array.");
    return true;
  }

  assert(std::string(drag.type()) == "Files");

  if (!FileDialog::FilesToJsObjectArray(
          g_files, module_environment, array.get(), error)) {
    assert(!error->empty());
    return true;
  }

  static const std::string16 kFiles(STRING16(L"files"));
  bool success = data->SetPropertyArray(kFiles, array.get());
  LOG16((L"drop %s\n", success ? L"success" : L"failed"));
  return success;
}
