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

#ifdef OFFICIAL_BUILD
  // The Drag-and-Drop API has not been finalized for official builds.
#else

#include <set>
#include <windows.h>
#include <shlobj.h>

#include "gears/desktop/drop_target_cr.h"

#include "gears/base/common/js_runner.h"
#include "gears/base/common/leak_counter.h"
#include "gears/base/common/mime_detect.h"
#include "gears/base/npapi/plugin.h"
#include "gears/desktop/file_dialog.h"

// Global drag state
// For a multi-tabbed browser environment, we'll need to fix this, eg., one
// tab might accept .txt files and another .pdf only, event may get out of
// sequence.  So global state is dicey; make it per-NPP-instance state.
//
// TODO(noel) fix this in V2, fix the FF/Safari drivers since they maintain
// global drag state too.  IE should be fine (IDropTarget).

static std::set<std::string16> g_mime_set;
static std::set<std::string16> g_type_set;
static std::vector<std::string16> g_mimes;
static std::vector<std::string16> g_files;
static double g_file_total_bytes = 0;
static size_t g_drag_files = 0;
static int32 g_identity = 0;

// EventListener

DECLARE_DISPATCHER(EventListener);

class EventListener : public PluginBase {
 public:
  static EventListener *Create(NPP npp) {
    static NPClass np_class = GetNPClassTemplate<EventListener>();
    return static_cast<EventListener*>(NPN_CreateObject(npp, &np_class));
  }

  EventListener(NPP npp)
      : PluginBase(npp), callback_(NULL), dispatcher_(this), data_(NULL) {
    type_id_ = NPN_GetStringIdentifier("type");
    PluginBase::Init(&dispatcher_);
  }

  std::map<std::string, int> &events() {
    return events_;
  }

  void SetCallback(void *data, bool (*callback)(int, JsObject *, void *)) {
    data_ = data;
    callback_ = callback;
  }

  void HandleEvent(JsCallContext *context) {
    if (!callback_) return;

    scoped_ptr<JsObject> event;
    JsArgument argv[] = {
      { JSPARAM_REQUIRED, JSPARAM_OBJECT, as_out_parameter(event) }
    };
    context->GetArguments(ARRAYSIZE(argv), argv);
    if (context->is_exception_set())
      return;

    NPObject *np_event = NPVARIANT_TO_OBJECT(event->token());
    ScopedNPVariant type;
    NPN_GetProperty(instance_, np_event, type_id_, &type);
    if (!NPVARIANT_IS_STRING(type))
      return;

    std::map<std::string, int>::iterator it = events_.find(
        std::string(NPVARIANT_TO_STRING(type).UTF8Characters,
                    NPVARIANT_TO_STRING(type).UTF8Length));
    if (it != events_.end()) {
      (*callback_)(it->second, event.get(), data_);
    }
  }

 private:
  bool (*callback_)(int event_id, JsObject *event, void *data);
  Dispatcher<EventListener> dispatcher_;
  std::map<std::string, int> events_;
  NPIdentifier type_id_;
  void *data_;

  DISALLOW_EVIL_CONSTRUCTORS(EventListener);
};

template <> void Dispatcher<EventListener>::Init() {
  RegisterMethod("handleEvent", &EventListener::HandleEvent);
}

// Chrome-devel NPAPI drag drop event ids

#define ELEMENT_EVENT_DRAGENTER 1
#define ELEMENT_EVENT_DRAGOVER  2
#define ELEMENT_EVENT_DRAGLEAVE 3
#define ELEMENT_EVENT_DRAGDROP  4

// DropTarget

DropTarget *DropTarget::CreateDropTarget(ModuleEnvironment *environment,
    JsDomElement &element, JsObject *options, std::string16 *error) {
  scoped_refptr<DropTarget> target(
      new DropTarget(environment, options, error));
  if (target->AddDropTarget(element, error)) {
    target->Ref();
    return target.get();
  } else {
    LOG16((L"DropTarget::Create %s\n", error->c_str()));
    return NULL;
  }
}

DropTarget::DropTarget(ModuleEnvironment *environment,
    JsObject *options, std::string16 *error)
        : DropTargetBase(environment, options, error),
          unregister_self_has_been_called_(false),
          runner_(environment->js_runner_),
          npp_(runner_->GetContext()),
          listener_(NULL),
          debug_(false) {
  LEAK_COUNTER_INCREMENT(DropTarget);
  options->GetPropertyAsBool(STRING16(L"debug"), &debug_);
}

DropTarget::~DropTarget() {
  LEAK_COUNTER_DECREMENT(DropTarget);
  assert(!listener_);
}

bool DropTarget::AddDropTarget(JsDomElement &element, std::string16 *error) {
  LOG16((L"DropTarget::AddDropTarget\n"));

  if (!ElementAttach(element.js_object())) {
    error->assign(L"Failed to attach to element.");
    ElementDetach();
    return false;
  } else {
    return true;
  }
}

bool DropTarget::ElementAttach(JsObject *element) {
  LOG16((L" ElementAttach 0x%lx\n", this));
  element_.Reset(element->token());
  accept_ = false;
  dragging_ = 0;

  NPObject **window = as_out_parameter(window_);
  NPError error = NPN_GetValue(npp_, NPNVWindowNPObject, window);
  if (error != NPERR_NO_ERROR || !window_.get())
    return false;

  listener_ = EventListener::Create(npp_);
  if (listener_ == NULL)
    return false;

  if (!AddEventListener("dragenter", ELEMENT_EVENT_DRAGENTER))
    return false;
  if (!AddEventListener("dragover", ELEMENT_EVENT_DRAGOVER))
    return false;
  if (!AddEventListener("dragleave", ELEMENT_EVENT_DRAGLEAVE))
    return false;
  if (!AddEventListener("drop", ELEMENT_EVENT_DRAGDROP))
    return false;

  listener_->SetCallback(this, &DropTarget::Event);
  return true;
}

bool DropTarget::AddEventListener(const char *event, int event_id) {
  assert(NPVARIANT_IS_OBJECT(element_));

  NPVariant args[3];
  STRINGZ_TO_NPVARIANT(event, args[0]);
  OBJECT_TO_NPVARIANT(listener_, args[1]);
  BOOLEAN_TO_NPVARIANT(false, args[2]);

  NPIdentifier method = NPN_GetStringIdentifier("addEventListener");
  NPObject *element = NPVARIANT_TO_OBJECT(element_);

  ScopedNPVariant result;
  if (NPN_Invoke(npp_, element, method, args, 3, &result)) {
    listener_->events()[event] = event_id;
    return true;
  } else {
    LOG16((L" addEventListener failed\n"));
    return false;
  }
}

void DropTarget::ElementDetach() {
  LOG16((L" ElementDetach 0x%lx\n", this));
  if (!listener_) return;

  listener_->SetCallback(this, NULL);  // Block callbacks.

  std::map<std::string, int>::const_iterator it;
  for (it = listener_->events().begin(); it != listener_->events().end();)
    RemoveEventListener(it++->first.c_str());

  listener_->events().clear();
  NPN_ReleaseObject(listener_);
  listener_ = NULL;
}

bool DropTarget::RemoveEventListener(const char *event) {
  assert(NPVARIANT_IS_OBJECT(element_));

  NPVariant args[3];
  STRINGZ_TO_NPVARIANT(event, args[0]);
  OBJECT_TO_NPVARIANT(listener_, args[1]);
  BOOLEAN_TO_NPVARIANT(false, args[2]);

  NPIdentifier method = NPN_GetStringIdentifier("removeEventListener");
  NPObject *element = NPVARIANT_TO_OBJECT(element_);

  // Chrome increases the reference count on our listener_ NPObject on
  // removeEventListener calls; other browsers decrease the ref-count.
  // The result: our listener_ objects maybe leak on Chrome.

  ScopedNPVariant result;
  if (NPN_Invoke(npp_, element, method, args, 3, &result)) {
    return true;
  } else {
    LOG16((L" removeEventListener failed\n"));
    return false;
  }
}

bool DropTarget::Event(int event_id, JsObject *event, void *data) {
  DropTarget *target = static_cast<DropTarget*>(data);

  switch (event_id) {
    case ELEMENT_EVENT_DRAGENTER :
      return target->HandleOnDragEnter(event);
    case ELEMENT_EVENT_DRAGOVER  :
      return target->HandleOnDragOver(event);
    case ELEMENT_EVENT_DRAGLEAVE :
      return target->HandleOnDragLeave(event);
    case ELEMENT_EVENT_DRAGDROP  :
      return target->HandleOnDragDrop(event);
    default :
      assert(false);  // NOTREACHED
      return true;
  }
}

void DropTarget::SetElementStyle(const char *style) {
  assert(NPVARIANT_IS_OBJECT(element_));

  NPVariant args[2];
  STRINGZ_TO_NPVARIANT("style", args[0]);
  STRINGZ_TO_NPVARIANT(style, args[1]);

  NPIdentifier method = NPN_GetStringIdentifier("setAttribute");
  NPObject *element = NPVARIANT_TO_OBJECT(element_);

  ScopedNPVariant result;
  NPN_Invoke(npp_, element, method, args, 2, &result);
}

// DragSession: more to do here, but gets it going.  TODO(noel): rewrite
// this in terms of Chromes CPAPI.

class DragSession {
 public:
  DragSession(NPP npp, JsObject *event) : object_(event), npp_(npp) {
    event_ = NPVARIANT_TO_OBJECT(object_->token());
    VOID_TO_NPVARIANT(type_);
    VOID_TO_NPVARIANT(data_);
    event_id_ = 0;
    identity_ = 0;
  }

  bool GetDragType() {
    Release(&type_, &data_);
    event_id_ = 0;
    identity_ = 0;
    NPN_GetValue(npp_, static_cast<NPNVariable>(7000), &event_);
    return identity_ && NPVARIANT_IS_STRING(type_);
  }

  bool GetDragData() {
    Release(&type_, &data_);
    event_id_ = 0;
    identity_ = 0;
    NPN_GetValue(npp_, static_cast<NPNVariable>(7001), &event_);
    return identity_ && NPVARIANT_IS_STRING(data_);
  }

  const int32 identity() const { return identity_; }
  const int32 event_id() const { return event_id_; }

  const std::string &type() {
    if (drag_type_.empty() && identity_ && NPVARIANT_IS_STRING(type_))
      drag_type_.assign(NPVARIANT_TO_STRING(type_).UTF8Characters,
                        NPVARIANT_TO_STRING(type_).UTF8Length);
    return drag_type_;
  }

  const std::string &data() {
    if (drag_data_.empty() && identity_ && NPVARIANT_IS_STRING(data_))
      drag_data_.assign(NPVARIANT_TO_STRING(data_).UTF8Characters,
                        NPVARIANT_TO_STRING(data_).UTF8Length);
    return drag_data_;
  }

  struct NPBoolVariant : public NPVariant {
    NPBoolVariant(bool value) { BOOLEAN_TO_NPVARIANT(value, *this); }
  };

  void CaptureEvent(ScopedNPObject &window, bool accept) {
    NPIdentifier value_id = NPN_GetStringIdentifier("returnValue");
    NPN_SetProperty(npp_, event_, value_id, &NPBoolVariant(false));

    NPIdentifier bubble_id = NPN_GetStringIdentifier("cancelBubble");
    NPN_SetProperty(npp_, event_, bubble_id, &NPBoolVariant(true));

    if (!accept) {  // Effect: event.dataTransfer.dropEffect='none'.
      ShowNoDropCursor(window);
    }
  }

  void ShowNoDropCursor(ScopedNPObject &window) {
    NPN_SetValue(npp_, static_cast<NPPVariable>(7000), window.get());
  }

  ~DragSession() {
    Release(&type_, &data_);
  }

 private:
  static void Release(NPVariant *type, NPVariant *data) {
    if (NPVARIANT_IS_STRING(*type)) NPN_ReleaseVariantValue(type);
    if (NPVARIANT_IS_STRING(*data)) NPN_ReleaseVariantValue(data);
  }

  std::string drag_type_;
  std::string drag_data_;
  JsObject *object_;
  NPP npp_;

  NPObject *event_;  // IMPORTANT: keep these variables grouped together.
  int32 identity_;
  int32 event_id_;
  NPVariant type_;
  NPVariant data_;

  DISALLOW_EVIL_CONSTRUCTORS(DragSession);
};

// FileAttributes

class FileAttributes {
 public:
  explicit FileAttributes(WCHAR *path) {  // assumption: path[MAX_PATH]
    ResolvePath(path);
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

  ~FileAttributes() {
    ::FindClose(find_handle_);
  }

 #if defined(DEBUG)
  #define DebugAttributes(attr) this->DisplayAttributes(attr)
 #else
  #define DebugAttributes(attr)
 #endif  // DEBUG

 private:
  void ResolvePath(WCHAR *path) {
    LOG16((L"%s\n", path));

    find_handle_ = ::FindFirstFile(path, &file_find_data_);
    attributes_ = 0;

    if (find_handle_ != INVALID_HANDLE_VALUE) {
      if (ResolveAttributes(path)) {
        DebugAttributes(attributes_);
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

    // Read the shortcut target path.
    hr = link->GetPath(target, MAX_PATH, 0, SLGP_UNCPRIORITY);
    if (FAILED(hr)) return hr;

    return S_OK;
  }

  static void DisplayAttributes(DWORD attributes) {
    LOG16((L" "));
    if (attributes & SFGAO_CANCOPY)
      LOG16((L"CANCOPY "));
    if (attributes & SFGAO_FILESYSTEM)
      LOG16((L"FILESYSTEM "));
    if (attributes & SFGAO_FOLDER)
      LOG16((L"FOLDER "));
    if (attributes & SFGAO_HIDDEN)
      LOG16((L"HIDDEN "));
    if (attributes & SFGAO_ISSLOW)
      LOG16((L"ISSLOW "));
    if (attributes & SFGAO_LINK)
      LOG16((L"LINK "));
    if (attributes & SFGAO_STREAM)
      LOG16((L"STREAM "));
    LOG16((L"\n"));
  }

 private:
  WIN32_FIND_DATA file_find_data_;
  HANDLE find_handle_;
  DWORD attributes_;

  DISALLOW_EVIL_CONSTRUCTORS(FileAttributes);
};

static bool FindDroppedFiles(JsRunnerInterface *runner, DragSession *drag) {
  if (drag->type() != "Files")
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
    WCHAR path[MAX_PATH];
    if (!::lstrcpyn(path, files[i].c_str(), MAX_PATH))
      return false;

    FileAttributes file(path);
    if (!file.Found() || file.IsDirectory())
      return false;

    LOG16((L" found %s\n", file.GetBaseName()));
    g_type_set.insert(file.GetFileExtension());
    g_file_total_bytes += file.GetFileSize();
    g_files.push_back(path);

    g_mimes.push_back(DetectMimeTypeOfFile(path));
    LOG16((L" mimeType %s\n", g_mimes.back().c_str()));
    g_mime_set.insert(g_mimes.back());
  }

  assert(g_files.size() == g_mimes.size());
  return true;
}

bool DropTarget::HandleOnDragEnter(JsObject *event) {
  LOG16((L"OnDragEnter %d\n", dragging_));

  DragSession drag(npp_, event);
  if (!dragging_) {
    if (!drag.GetDragType())
      return false;

    if (!g_identity || g_identity != drag.identity()) {
      g_identity = drag.identity();
      g_drag_files = 0;

      if (!FindDroppedFiles(runner_, &drag)) {
        g_file_total_bytes = 0;
        g_type_set.clear();
        g_mime_set.clear();
        g_mimes.clear();
        g_files.clear();
      }
    }
  }

  // Enter dragging state iff we have drag files.  Do not respond to Text
  // or URL drags.  HHTML5 javascript can already handle those cases, and
  // we might break them if we cancel *their* event bubbling, etc.

  if (g_drag_files) {
    if (++dragging_ == 1) {
      accept_ = OnDragActive(on_drag_enter_.get(), event);
    } else {
      accept_ = OnDragActive(on_drag_over_.get(), event);
    }

    drag.CaptureEvent(window_, accept_);
    if (debug_) {
      SetElementStyle("border: blue 3px solid;");
    }
  }

  return false;
}

bool DropTarget::HandleOnDragOver(JsObject *event) {
  LOG16((L"OnDragOver %d\n", dragging_));

  if (dragging_) {
    accept_ = OnDragActive(on_drag_over_.get(), event);
    DragSession(npp_, event).CaptureEvent(window_, accept_);
  } else {
    HandleOnDragEnter(event);
  }

  return false;
}

bool DropTarget::HandleOnDragLeave(JsObject *event) {
  LOG16((L"OnDragLeave %d\n", dragging_));

  if (dragging_) {
    OnDragLeave(on_drag_leave_.get(), event);
    DragSession(npp_, event).CaptureEvent(window_, accept_);
    if (debug_) {
      SetElementStyle("border: white 3px solid;");
    }
  }

  dragging_ = 0;
  return false;
}

bool DropTarget::HandleOnDragDrop(JsObject *event) {
  LOG16((L"OnDragDrop %d\n", dragging_));

  if (dragging_) {
    ATLASSERT(g_drag_files);

    if (accept_) {
      OnDragDrop(on_drop_.get(), event);
    } else {
      OnDragLeave(on_drag_leave_.get(), event);
    }

    // Regardless of whether CaptureEvent() is called or not, a drop from
    // the gears layer causes the Chrome client area to loose focus.  This
    // doesn't happen for javascript-layer text drag handlers that prevent
    // default, cancel bubble, etc.  TODO(noel): find out why.
    DragSession(npp_, event).CaptureEvent(window_, accept_);
    if (debug_) {
      SetElementStyle("border: white 3px solid;");
    }

    g_file_total_bytes = 0;
    g_type_set.clear();
    g_mime_set.clear();
    g_mimes.clear();
    g_files.clear();
  }

  g_drag_files = 0;
  g_identity = 0;
  dragging_ = 0;
  return false;
}

// AddFileMetaData() helper.

static bool ToJsArray(std::set<std::string16> const &s, JsArray *array) {
  typedef std::set<std::string16> string16_set;

  int length = 0;
  if (!array || !array->GetLength(&length))
    return false;

  for (string16_set::const_iterator it = s.begin(); it != s.end(); ++it) {
    if (!array->SetElementString(length++, *it))
      return false;
  }

  return true;
};

static bool AddFileMetaData(JsRunnerInterface *runner,
                            JsObject *event, JsObject *context) {
  if (!context) return false;

  NPObject *object = NPVARIANT_TO_OBJECT(context->token());
  NPP npp = runner->GetContext();
  NPVariant value;

  NPIdentifier id = NPN_GetStringIdentifier("event");
  if (event && !NPN_SetProperty(npp, object, id, &event->token()))
    return false;

  NPIdentifier count_id = NPN_GetStringIdentifier("count");
  INT32_TO_NPVARIANT(static_cast<int32>(g_files.size()), value);
  if (!NPN_SetProperty(npp, object, count_id, &value))
    return false;

  NPIdentifier bytes_id = NPN_GetStringIdentifier("totalBytes");
  DOUBLE_TO_NPVARIANT(g_file_total_bytes, value);
  if (!NPN_SetProperty(npp, object, bytes_id, &value))
    return false;

  scoped_ptr<JsArray> type_array(runner->NewArray());
  if (!ToJsArray(g_type_set, type_array.get()))
    return false;
  NPIdentifier types_id = NPN_GetStringIdentifier("extensions");
  if (!NPN_SetProperty(npp, object, types_id, &type_array->token()))
    return false;

  scoped_ptr<JsArray> mime_array(runner->NewArray());
  if (!ToJsArray(g_mime_set, mime_array.get()))
    return false;
  NPIdentifier mimes_id = NPN_GetStringIdentifier("mimeTypes");
  if (!NPN_SetProperty(npp, object, mimes_id, &mime_array->token()))
    return false;

  return true;
}

bool DropTarget::OnDragActive(const JsRootedCallback *callback,
                              JsObject *event) {
  if (!callback || g_files.empty())
    return false;
  scoped_ptr<JsObject> context(runner_->NewObject());
  if (!AddFileMetaData(runner_, event, context.get()))
    return false;

  JsParamToSend argv[] = { { JSPARAM_OBJECT, context.get() } };
  JsRootedToken *retval = NULL;
  if (!runner_->InvokeCallback(callback, 0, 1, argv, &retval))
    return false;

  bool accept = false;
  if (retval && NPVARIANT_IS_BOOLEAN(retval->token()))
    accept = (NPVARIANT_TO_BOOLEAN(retval->token()) == false);
  delete retval;
  return accept;
}

void DropTarget::OnDragLeave(const JsRootedCallback *callback,
                             JsObject *event) {
  if (!callback || g_files.empty())
    return;
  scoped_ptr<JsObject> context(runner_->NewObject());
  if (!AddFileMetaData(runner_, event, context.get()))
    return;

  LOG16((L" fire dragleave\n"));
  JsParamToSend argv[] = { { JSPARAM_OBJECT, context.get() } };
  runner_->InvokeCallback(callback, NULL, 1, argv, NULL);
}

void DropTarget::OnDragDrop(const JsRootedCallback *callback,
                            JsObject *event) {
  if (!callback || g_files.empty())
    return;
  scoped_ptr<JsObject> context(runner_->NewObject());
  if (!AddFileMetaData(runner_, event, context.get()))
    return;

  scoped_ptr<JsArray> array(runner_->NewArray());
  if (!array.get()) {
    LOG16((L"Failed creating javascript file array\n"));
    return;
  }

  std::string16 error;
  if (!FileDialog::FilesToJsObjectArray(
          g_files, module_environment_.get(), array.get(), &error)) {
    LOG16((L"FilesToJsObjectArray: %s\n", error.c_str()));
    return;
  }

  scoped_ptr<JsObject> file;  // Add the per-file mimeType.
  for (size_t i = 0; i < g_mimes.size(); ++i) {
    array->GetElementAsObject(i, as_out_parameter(file));
    file->SetPropertyString(STRING16(L"mimeType"), g_mimes[i]);
    file.reset();
  }

  context->SetPropertyArray(STRING16(L"files"), array.get());
  LOG16((L" fire drop\n"));

  JsParamToSend argv[] = { { JSPARAM_OBJECT, context.get() } };
  runner_->InvokeCallback(callback, NULL, 1, argv, NULL);
}

void DropTarget::UnregisterSelf() {
  if (!unregister_self_has_been_called_) {
    unregister_self_has_been_called_ = true;
    ElementDetach();
    Unref();  // Balance the Ref() call in CreateDropTarget.
  }
}

#endif  // OFFICIAL_BUILD
