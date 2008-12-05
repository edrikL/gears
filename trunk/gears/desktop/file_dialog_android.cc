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

#if defined(OS_ANDROID)

#include "gears/base/android/java_class.h"
#include "gears/base/android/java_class_loader.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/java_string.h"
#include "gears/base/android/webview_manager.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/basictypes.h"
#include "gears/base/common/common.h"
#include "gears/base/common/message_service.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/npapi/module.h"
#include "gears/desktop/file_dialog_android.h"
#include "gears/ui/common/i18n_strings.h"

#include "third_party/jsoncpp/json.h"

namespace {

const char16* kSelectionCompleteTopic = STRING16("selection complete");

static const char* kShowDialogSignature =
  "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)V";

static const char* kNativeDialogClass = GEARS_JAVA_PACKAGE "/NativeDialog";
static const char* kWebviewClass = "android/webkit/WebView";

static const char* kSingleFile = "SINGLE_FILE";
static const char* kMultipleFiles = "MULTIPLE_FILES";

static FileDialogAndroid* staticInstance;

// Utility class for passing StringList between threads.
// (Copied from file_dialog_win32.cc)
class FileDialogData : public NotificationData {
 public:
  FileDialogData() {}
  virtual ~FileDialogData() {}

  FileDialog::StringList selected_files;

 private:
  // (Copied from geolocation.cc)
  // NotificationData implementation.
  //
  // We do not wish our messages to be delivered across process boundaries. To
  // achieve this, we use an unregistered class ID. On receipt of an IPC
  // message, InboundQueue::ReadOneMessage will attempt to deserialize the
  // message data using CreateAndReadObject. This will fail silently because no
  // factory function is registered for our class ID and Deserialize will not be
  // called.
  virtual SerializableClassId GetSerializableClassId() const {
    return SERIALIZABLE_DESKTOP;
  }
  virtual bool Serialize(Serializer * /* out */) const {
    // The serialized message is not used.
    return true;
  }
  virtual bool Deserialize(Deserializer * /* in */) {
    // This method should never be called.
    assert(false);
    return false;
  }

  DISALLOW_EVIL_CONSTRUCTORS(FileDialogData);
};

void closeAsynchronousDialog(JNIEnv* env, jclass clazz, jobject files) {
  JniSetCurrentThread(env);

  jstring result = static_cast<jstring>(files);

  JavaString result_string = result;
  std::string16 str = result_string.ToString16();
  std::string result_string_utf8;
  if(!String16ToUTF8(str, &result_string_utf8)) {
    LOG(("Couldn't convert the result string from the filepicker"));
    return;
  }

  Json::Value res;
  Json::Reader reader;
  if (!reader.parse(result_string_utf8, res)) {
    LOG(("Error parsing return value from filepicker. Error was: %s",
         reader.getFormatedErrorMessages().c_str()));
    return;
  }

  FileDialog::StringList selected_files;
  if (res.isArray()) {
    int size = res.size();
    for (int i = 0; i < size; i++) {
      std::string16 strs;
      strs = UTF8ToString16(res[i].asString());
      selected_files.push_back(strs);
    }
  }

  if (staticInstance)
    staticInstance->ProcessSelection(selected_files);
}

static JavaClass::Method methods[] = {
    { JavaClass::kNonStatic, "<init>", "()V" },
    { JavaClass::kNonStatic, "showAsyncDialog", kShowDialogSignature },
};

static JavaClass::Method webview_methods[] = {
    { JavaClass::kNonStatic, "getContext",
      "()Landroid/content/Context;" },
};

JNINativeMethod fp_native_methods[1] = {
    { "closeAsynchronousDialog", "(Ljava/lang/String;)V",
          (void*) closeAsynchronousDialog },
};

}  // anonymous namespace


void FileDialogAndroid::ProcessSelection(StringList files) {
  scoped_ptr<FileDialogData> data(new FileDialogData);

  StringList::iterator iterator = files.begin();
  for (StringList::iterator file = iterator; file != files.end(); ++file) {
    (data.get())->selected_files.push_back(*file);
  }

  MessageService::GetInstance()->NotifyObservers(kSelectionCompleteTopic,
                                                 data.release());
}

FileDialogAndroid::FileDialogAndroid() {
  // We register the native method callback
  static bool isRegistered;
  if (isRegistered == false) {
    jniRegisterNativeMethods(JniGetEnv(),
                           kNativeDialogClass,
                           fp_native_methods,
                           NELEM(fp_native_methods));
    isRegistered = true;
  }

  MessageService::GetInstance()->AddObserver(this, kSelectionCompleteTopic);
}

FileDialogAndroid::~FileDialogAndroid() {
  MessageService::GetInstance()->RemoveObserver(this, kSelectionCompleteTopic);
}

void FileDialogAndroid::OnNotify(MessageService *service,
                                 const char16 *topic,
                                 const NotificationData *data) {
  const FileDialogData* dialog_data = static_cast<const FileDialogData*>(data);
  CompleteSelection(dialog_data->selected_files);
}

bool FileDialogAndroid::BeginSelection(NativeWindowPtr parent,
                                       const FileDialog::Options& options,
                                       std::string16* error) {
  if (!SetOptions(options, error))
    return false;
  if (!Display(error))
    return false;
  return true;
}

void FileDialogAndroid::CancelSelection() {
  // TODO(bpm): Nothing calls CancelSelection yet, but it might someday.
}

void FileDialogAndroid::DoUIAction(UIAction action) {
  // On Android, we only have one dialog at a time:
  // it is handled by an activity and we cannot switch to another
  // tab without closing it. There is thus no need to handle
  // the display actions.
}

bool FileDialogAndroid::SetOptions(const FileDialog::Options& options,
                                   std::string16* error) {
  StringList filter = options.filter;
  Json::Value json_options;
  Json::Value array;

  for (size_t i = 0; i < filter.size(); ++i) {
    std::string filter_item;
    if (!String16ToUTF8(filter[i].c_str(), &filter_item))
      continue;
    array.append(filter_item);
  }

  json_options["filters"] = array;

  if (options.mode == SINGLE_FILE) {
    json_options["mode"] = kSingleFile;
  } else {
    json_options["mode"] = kMultipleFiles;
  }

  Json::FastWriter writer;
  options_ = writer.write(json_options);
  LOG(("File picker options: %s", options_.c_str()));

  return true;
}

jobject FileDialogAndroid::GetContext() {
  JavaLocalFrame frame;
  jobject context = NULL;
  jobject java_webview = NULL;

  // To call the dialog, we need an android.content.Context instance. We get it
  // by getting the WebView and calling the getContext() method on it
  if (WebViewManager::GetWebView(&java_webview)) {
    // Now we can call the getContext() method on the WebView instance
    JavaClass java_webview_class;

    // get the JavaClass for WebView
    if (!java_webview_class.FindClass(kWebviewClass)) {
      LOG(("Couldn't load the WebView class\n"));
      return false;
    }

    if (!java_webview_class.GetMultipleMethodIDs(webview_methods,
        NELEM(webview_methods))) {
      LOG(("Couldn't get WebView methods\n"));
      return false;
    }

    // call getContext()
    context = JniGetEnv()->CallObjectMethod(java_webview,
        webview_methods[0].id);
  }
  return context;
}

bool FileDialogAndroid::Display(std::string16* error) {
  JavaLocalFrame frame;
  jobject context = GetContext();

  if (context == NULL) {
    LOG(("FileDialog: we could not get an android.content.Context instance\n"));
    return false;
  }

  // If we do not find the native dialog class (e.g. old version of android)
  // we bail out.
  JavaClass java_dialog_class;
  if (!java_dialog_class.FindClass(kNativeDialogClass)) {
    LOG(("Couldn't load the NativeDialog class!\n"));
    return false;
  }

  if (!java_dialog_class.GetMultipleMethodIDs(methods, NELEM(methods))) {
    LOG(("Couldn't get NativeDialog class methods\n"));
    return false;
  }

  // We create an instance of the Dialog class
  JavaGlobalRef<jobject> java_dialog_instance(
      JniGetEnv()->NewObject(java_dialog_class.Get(), methods[0].id));

  staticInstance = this;

  // And finally we call the showDialog() method, passing the options_
  // string (a json formatted string containing the filters and the
  // singleFile boolean).
  JniGetEnv()->CallVoidMethod(
      java_dialog_instance.Get(), methods[1].id, context,
      JavaString("filepicker_dialog").Get(),
      JavaString(options_).Get());

  return true;
}

#endif  // defined(OS_ANDROID)
