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

#if defined(WIN32) && !defined(WINCE) && defined(BROWSER_IE)
#else
#error "We only use this for IE on windows desktop"
#endif

#include <atlsync.h>
#include <windows.h>
#include <deque>
#include "gears/base/common/atomic_ops.h"
#include "gears/base/common/ipc_message_queue.h"
#include "gears/base/common/message_queue.h"
#include "gears/base/ie/atl_headers.h"
#include "gears/factory/common/factory_utils.h"  // for AppendBuildInfo
#include "gears/third_party/linked_ptr/linked_ptr.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"


//-----------------------------------------------------------------------------
// A primitive circular buffer class
// TODO(michaeln): should this be moved to a seperate circular_buffer.h file?
//-----------------------------------------------------------------------------
class CircularBuffer {
 public:
  CircularBuffer() : head_(0), tail_(0), capacity_(0), buffer_(NULL) {}

  size_t head() { return head_; }
  size_t tail() { return tail_; }
  size_t capacity() { return capacity_; }
  void *buffer() { return buffer_; }

  void set_head(size_t head) { head_ = head; }
  void set_tail(size_t tail) { tail_ = tail; }
  void set_capacity(size_t capacity) { capacity_ = capacity; }
  void set_buffer(void *buffer) { buffer_ = buffer; }
  
  void *head_ptr() { return reinterpret_cast<uint8*>(buffer_) + head_; }
  void *tail_ptr() { return reinterpret_cast<uint8*>(buffer_) + tail_; }

  size_t data_available() {
    if (tail_ >= head_ ) 
      return tail_ - head_;
    else
      return (capacity_ - head_) + tail_;
  }

  size_t space_available() {
    return capacity_ - data_available();
  }

  size_t contiguous_data_available() {
    if (tail_ >= head_ ) 
      return tail_ - head_;
    else 
      return capacity_ - head_;
  }

  size_t contiguous_space_available() {
    if (tail_ >= head_)
      return capacity_ - tail_;
    else 
      return head_ - tail_;
  }

  void read(void *data, size_t size) {
    assert(size <= data_available());
    while (size > 0) {
      void *read_pos = head_ptr();
      size_t read_amount = min(size, contiguous_data_available());
      assert(read_amount != 0);
      memcpy(data, read_pos, read_amount);
      head_ += read_amount;
      head_ %= capacity_;
      size -= read_amount;
      data = reinterpret_cast<uint8*>(data) + read_amount;
    }
  }

  void write(const void *data, size_t size) {
    assert(size <= space_available());
    while (size > 0) {
      void *write_pos = tail_ptr();
      size_t write_amount = min(size, contiguous_space_available());
      assert(write_amount != 0);
      memcpy(write_pos, data, write_amount);
      tail_ += write_amount;
      tail_ %= capacity_;
      size -= write_amount;
      data = reinterpret_cast<const uint8*>(data) + write_amount;
    }
  }

 private:
  size_t head_;
  size_t tail_;
  size_t capacity_;
  void *buffer_;
};

//-----------------------------------------------------------------------------
// SharedMemory
// TODO(michaeln): move this to a seperate shared_memory_win32.h file?
//-----------------------------------------------------------------------------
class SharedMemory {
 public:
  SharedMemory() : mapping_(NULL) {}
  ~SharedMemory() { Close(); };
  bool Create(const char16 *name, int size);
  bool Open(const char16 *name);
  void Close();

  class MappedView {
   public:
    MappedView() : view_(NULL) {}
    ~MappedView() { CloseView(); }
    bool OpenView(SharedMemory *shared_memory, bool read_write);
    void CloseView();
    uint8 *view() { return view_; }
   private:
    uint8 *view_;
  };

  template<class T>
  class MappedViewOf : public MappedView {
   public:
    T *get() { return reinterpret_cast<T*>(view()); }
    T* operator->() {
      assert(view() != NULL);
      return reinterpret_cast<T*>(view());
    }
  };

 private:
  friend class MappedView;
  HANDLE mapping_;
};


bool SharedMemory::Create(const char16 *name, int size) {
  assert(mapping_ == NULL);
  mapping_ = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                               0, size, name);
  return mapping_ != NULL;
}


bool SharedMemory::Open(const char16 *name) {
  assert(mapping_ == NULL);
  mapping_ = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE,
                             FALSE, name);
  return mapping_ != NULL;
}


void SharedMemory::Close() {
  if (mapping_ != NULL) {
    CloseHandle(mapping_);
    mapping_ = NULL;
  }
}


bool SharedMemory::MappedView::OpenView(SharedMemory *shared_memory,
                                        bool read_write) {
  assert(!view_);
  DWORD access = FILE_MAP_READ;
  if (read_write) access |= FILE_MAP_WRITE;
  view_ = reinterpret_cast<uint8*>(
              ::MapViewOfFile(shared_memory->mapping_,
                              access, 0, 0, 0));
  return view_ != NULL;
}


void SharedMemory::MappedView::CloseView() {
  if (view_) {
    ::UnmapViewOfFile(view_);
    view_ = NULL;
  }  
}


//-----------------------------------------------------------------------------
// Kernel objects
//-----------------------------------------------------------------------------
static const char16 *kRegistryMutex = L"RegistryMutex";
static const char16 *kRegistryMemory = L"RegistryMemory";
static const char16 *kQueueWriteMutex = L"QWriteMutex";
static const char16 *kQueueDataAvailableEvent = L"QDataAvailableEvent";
static const char16 *kQueueSpaceAvailableEvent = L"QSpaceAvailableEvent";
static const char16 *kQueueIoBuffer = L"QIoBuffer";


inline void GetKernelObjectName(const char16 *object_type,
                                IpcProcessId process_id,
                                CStringW *name) {
  const char16 *kObjectNameFormat = STRING16(L"GearsIpc:%d:%s:%s");
  std::string16 buildinfo;
  AppendBuildInfo(&buildinfo);
  name->Format(kObjectNameFormat, process_id, object_type, buildinfo.c_str());
}


class IpcQueueMutex : public ATL::CMutex {
 public:
  bool Create(IpcProcessId process_id) {
    CStringW name;
    GetKernelObjectName(kQueueWriteMutex, process_id, &name);
    return ATL::CMutex::Create(NULL, FALSE, name) ? true : false;
  }

  bool Open(IpcProcessId process_id) {
    CStringW name;
    GetKernelObjectName(kQueueWriteMutex, process_id, &name);
    return ATL::CMutex::Open(SYNCHRONIZE | MUTEX_MODIFY_STATE,
                             FALSE, name) ? true : false;
  }
};


class IpcQueueEvent : public ATL::CEvent {
 public:
  // Creates a manual-reset event handle in the unsignalled state
  bool Create(const char16 *event_type, IpcProcessId process_id) {
    CStringW name;
    GetKernelObjectName(event_type, process_id, &name);
    return ATL::CEvent::Create(NULL, TRUE, FALSE, name) ? true : false;
  }

  bool Open(const char16 *event_type, IpcProcessId process_id) {
    CStringW name;
    GetKernelObjectName(event_type, process_id, &name);
    return ATL::CEvent::Open(SYNCHRONIZE | EVENT_MODIFY_STATE,
                             FALSE, name) ? true : false;
  }
};


class IpcQueueBuffer {
 public:
  static const int kCapacity = 32 * 1024;  // 32 Kbytes

  // TODO(michaeln): should this be split into two blocks of shared memory?
  // One for the head position, and another for the tail and data array.
  // WriteTransaction would open a readonly view on the former and rw view
  // on the latter, ReadTransaction vice versa.
  struct BufferFormat {
    int head;  // offset to the first unread byte
    int tail;  // offset to the last written byte
    uint8 data[kCapacity];
  };
  SharedMemory shared_memory_;

  bool Create(IpcProcessId process_id) {
    CStringW name;
    GetKernelObjectName(kQueueIoBuffer, process_id, &name);
    if (!shared_memory_.Create(name, sizeof(BufferFormat)))
      return false;

    SharedMemory::MappedViewOf<BufferFormat> mapped_buffer;
    if (!mapped_buffer.OpenView(&shared_memory_, true)) {
      shared_memory_.Close();
      return false; 
    }
    mapped_buffer->head = 0;
    mapped_buffer->tail = 0;
    return true;
  }

  bool Open(IpcProcessId process_id) {
    CStringW name;
    GetKernelObjectName(kQueueIoBuffer, process_id, &name);
    return shared_memory_.Open(name);
  }

  // The 'head' position stored in shared memory is updated when
  // the transaction goes out of scope.
  class ReadTransaction {
   public:
    ReadTransaction() : was_read_(false) {}

    ~ReadTransaction() {
      if (was_read_) {
        mapped_buffer_->head = circular_buffer_.head();
      }
    }

    bool Start(IpcQueueBuffer *io_buffer) {
      if (!mapped_buffer_.OpenView(&io_buffer->shared_memory_, true)) {
        return false; 
      }
      circular_buffer_.set_buffer(mapped_buffer_->data);
      circular_buffer_.set_capacity(kCapacity);
      circular_buffer_.set_head(mapped_buffer_->head);
      circular_buffer_.set_tail(mapped_buffer_->tail);
      return true;
    }

    size_t data_available() {
      return circular_buffer_.data_available();
    }

    size_t space_available() {
      return circular_buffer_.space_available();
    }
  
    void Read(void *data, size_t size) {
      assert(size <= data_available());
      if (size > 0) {
        circular_buffer_.read(data, size);
        was_read_ = true;
      }
    }

   private:
    bool was_read_;
    CircularBuffer circular_buffer_;
    SharedMemory::MappedViewOf<BufferFormat> mapped_buffer_;
  };

  // The 'tail' position stored in shared memory is updated when
  // the transaction goes out of scope.
  class WriteTransaction {
   public:
    WriteTransaction() : was_written_(false) {}

    ~WriteTransaction() {
      if (was_written_) {
        mapped_buffer_->tail = circular_buffer_.tail();
      }
    }

    bool Start(IpcQueueBuffer *io_buffer) {
      if (!mapped_buffer_.OpenView(&io_buffer->shared_memory_, true)) {
        return false; 
      }
      circular_buffer_.set_buffer(mapped_buffer_->data);
      circular_buffer_.set_capacity(kCapacity);
      circular_buffer_.set_head(mapped_buffer_->head);
      circular_buffer_.set_tail(mapped_buffer_->tail);
      return true;
    }

    size_t data_available() {
      return circular_buffer_.data_available();
    }

    size_t space_available() {
      return circular_buffer_.space_available();
    }
  
    void Write(const void *data, size_t size) {
      assert(size <= space_available());
      if (size > 0) {
        circular_buffer_.write(data, size);
        was_written_ = true;
      }
    }

   private:
    bool was_written_;
    CircularBuffer circular_buffer_;
    SharedMemory::MappedViewOf<BufferFormat> mapped_buffer_;
  };
};


//-----------------------------------------------------------------------------
// IpcProcessRegistry
// TODO(michaeln)
// a Perhaps cache the array and notice when it changes and only lock to
//   pick up the changes (keep a revision number in the shared block).
// b Perhaps grow and shrink as needed or alloc new shared blocks as needed?
// c Either a or SingleWriterMultipleReader type of locking might be nice
// d Recover from zombie processes that hold locks
//   registry_mutex or an outbound queue's write_mutex
//-----------------------------------------------------------------------------
class IpcProcessRegistry {
 public:
  bool Init();
  bool Add(IpcProcessId id);
  bool Remove(IpcProcessId id);
  void GetAll(std::vector<IpcProcessId> *list);

 private:
  static const IpcProcessId kInvalidProcessId = 0;
  static const int kMaxProcesses = 100;
  // The structure store in our shared memory
  struct Registry {
    IpcProcessId processes[kMaxProcesses];
  };
  ATL::CMutex mutex_;
  SharedMemory shared_memory_;
};


bool IpcProcessRegistry::Init() {
  CStringW mutex_name;
  CStringW shared_memory_name;
  GetKernelObjectName(kRegistryMutex, 0, &mutex_name);
  GetKernelObjectName(kRegistryMemory, 0, &shared_memory_name);

  if (!mutex_.Create(NULL, FALSE, mutex_name))
    return false;

  ATL::CMutexLock lock(mutex_);
  if (!shared_memory_.Open(shared_memory_name)) {
    if (!shared_memory_.Create(shared_memory_name, sizeof(Registry))) {
      return false;
    }
    SharedMemory::MappedViewOf<Registry> registry;
    if (!registry.OpenView(&shared_memory_, true)) {
      return false;
    }
    for (int i = 0; i < kMaxProcesses; ++i) {
      registry->processes[i] = kInvalidProcessId;
    }
  }
  return true;
}


bool IpcProcessRegistry::Add(IpcProcessId id) {
  assert(id != 0);
  assert(!Remove(id));
  ATL::CMutexLock lock(mutex_);
  SharedMemory::MappedViewOf<Registry> registry;
  if (!registry.OpenView(&shared_memory_, true)) {
    return false;
  }
  for (int i = 0; i < kMaxProcesses; ++i) {
    if (registry->processes[i] == kInvalidProcessId) {
      registry->processes[i] = id;
      return true;
    }
  }  
  // TODO(michaeln): prune dead processes and try again, grow shared memory?
  assert(false);  // apparently we need a bigger array
  return false;
}


bool IpcProcessRegistry::Remove(IpcProcessId id) {
  assert(id != 0);
  ATL::CMutexLock lock(mutex_);
  SharedMemory::MappedViewOf<Registry> registry;
  if (!registry.OpenView(&shared_memory_, true)) {
    return false;
  }
  for (int i = 0; i < kMaxProcesses; ++i) {
    if (registry->processes[i] == id) {
      registry->processes[i] = kInvalidProcessId;
      return true;
    }
  }  
  return false;
}


void IpcProcessRegistry::GetAll(std::vector<IpcProcessId> *out) {
  out->clear();
  ATL::CMutexLock lock(mutex_);
  SharedMemory::MappedViewOf<Registry> registry;
  if (!registry.OpenView(&shared_memory_, false)) {
    return;
  }
  for (int i = 0; i < kMaxProcesses; ++i) {
    if (registry->processes[i] != kInvalidProcessId) {
      out->push_back(registry->processes[i]);
    }
  }  
}


//-----------------------------------------------------------------------------
// IpcThreadMessageEnvelope and ShareableIpcMessage
//-----------------------------------------------------------------------------

class ShareableIpcMessage {
 public:
  ShareableIpcMessage(int ipc_message_type,
                      IpcMessageData *ipc_message_data)
      : ipc_message_type_(ipc_message_type),
        ipc_message_data_(ipc_message_data),
        refcount_(1) {
  }

  void AddReference() {
    AtomicIncrement(&refcount_, 1);
  }

  void RemoveReference() {
    if (AtomicIncrement(&refcount_, -1) == 0)
      delete this;
  }

  int ipc_message_type() { return ipc_message_type_; }

  IpcMessageData *ipc_message_data() { return ipc_message_data_.get(); }

  std::vector<uint8> *serialized_message_data() {
    assert(ipc_message_data_.get());
    if (!serialized_message_data_.get()) {
      serialized_message_data_.reset(new std::vector<uint8>);
      Serializer serializer(serialized_message_data_.get());
      if (!serializer.WriteObject(ipc_message_data_.get())) {
        serialized_message_data_.reset(NULL);
      }
    }
    return serialized_message_data_.get();
  }

 private:
  int ipc_message_type_;
  scoped_ptr<IpcMessageData> ipc_message_data_;
  scoped_ptr< std::vector<uint8> > serialized_message_data_;
  int refcount_;
};


class IpcThreadMessageEnvelope : public MessageData {
 public:
  IpcThreadMessageEnvelope(int ipc_message_type,
                           IpcMessageData *ipc_message_data,
                           bool include_self) 
      : send_to_all_(true),
        include_self_(include_self) {
    shareable_message_ = new ShareableIpcMessage(ipc_message_type,
                                                 ipc_message_data);
  }

  IpcThreadMessageEnvelope(int ipc_message_type,
                           IpcMessageData *ipc_message_data,
                           IpcProcessId dest_process_id) 
      : send_to_all_(false),
        destination_process_id_(dest_process_id) {
    shareable_message_ = new ShareableIpcMessage(ipc_message_type,
                                                 ipc_message_data);
  }

  virtual ~IpcThreadMessageEnvelope() {
    shareable_message_->RemoveReference();  
  }

  bool send_to_all_;
  bool include_self_;
  IpcProcessId destination_process_id_;
  ShareableIpcMessage *shareable_message_;
};


//-----------------------------------------------------------------------------
// InboundQueue and OutboundQueue
//-----------------------------------------------------------------------------

class Win32IpcMessageQueue;

class QueueBase {
 protected:
  QueueBase(Win32IpcMessageQueue *owner) : owner_(owner) {}

  Win32IpcMessageQueue *owner_;
  IpcQueueMutex write_mutex_;
  IpcQueueEvent data_available_event_;
  IpcQueueEvent space_available_event_;
  IpcQueueBuffer io_buffer_;

  // The 'wire format' used to represent messages in our io buffer
  struct MessageHeader {
    IpcProcessId msg_source;
    int msg_type;
    int msg_size;

    MessageHeader() : msg_source(0), msg_type(0), msg_size(0) {}
    MessageHeader(IpcProcessId source, int type, int size)
        : msg_source(source), msg_type(type), msg_size(size) {}
  };
};


class OutboundQueue : public QueueBase {
 public:
  OutboundQueue(Win32IpcMessageQueue *owner)
    : QueueBase(owner),
      process_id_(0),
      has_write_mutex_(false),
      is_waiting_for_write_mutex_(false),
      is_waiting_for_space_available_(false) {}

  ~OutboundQueue();

  bool Open(IpcProcessId process_id);
  void AddMessageToQueue(ShareableIpcMessage *message);
  void WaitComplete(HANDLE handle, bool abandoned);
  bool empty() { return pending_.empty(); }
  IpcProcessId process_id() { return process_id_; }

 private:
  IpcProcessId process_id_;
  ATL::CHandle process_handle_;
  std::deque<ShareableIpcMessage*> pending_;
  bool has_write_mutex_;
  bool is_waiting_for_write_mutex_;
  bool is_waiting_for_space_available_;

  void WritePendingMessages();
  bool WriteOneMessage(ShareableIpcMessage *message);
  void ClearPendingMessages();
  void MaybeWaitForWriteMutex();
  void MaybeReleaseWriteMutex();
  void WaitForSpaceAvailable();
};

class InboundQueue : public QueueBase {
 public:
  InboundQueue(Win32IpcMessageQueue *owner) : QueueBase(owner) {}

  bool Create(IpcProcessId process_id);
  void WaitComplete(HANDLE handle, bool abandoned);

 private:
  void ReadAndDispatchMessages();
  bool ReadOneMessage(IpcProcessId *source, int *message_type,
                      IpcMessageData **message);
};


//-----------------------------------------------------------------------------
// Win32IpcMessageQueue
//-----------------------------------------------------------------------------
class Win32IpcMessageQueue : public IpcMessageQueue,
                             public ThreadMessageQueue::HandlerInterface {
 public:
  Win32IpcMessageQueue()
    : current_process_id_(0), thread_id_(0), thread_message_queue_(NULL) {} 

  ~Win32IpcMessageQueue();

  bool Init();

  // IpcMessageQueue overrides
  virtual IpcProcessId GetCurrentIpcProcessId();
  virtual void Send(IpcProcessId dest_process_id,
                    int message_type,
                    IpcMessageData *message_data);
  virtual void SendToAll(int message_type,
                         IpcMessageData *message_data,
                         bool including_current_process);

  // Our worker thread's entry point and message loop
  static unsigned int __stdcall StaticThreadProc(void *start_data);
  void InstanceThreadProc(struct ThreadStartData *start_data);
  void RunMessageLoop();

  // Wait completion handling
  void WaitComplete(int index, bool abandoned);
  void AddWaitHandleForQueue(HANDLE wait_handle, OutboundQueue *outbound_queue);

  // ThreadMessage handling
  virtual void HandleThreadMessage(int message_type,
                                   MessageData *message_data);
  void HandleSendToAll(IpcThreadMessageEnvelope *message_envelope);
  void HandleSendToOne(IpcThreadMessageEnvelope *message_envelope);

  // OutboundQueue management
  OutboundQueue *GetOutboundQueue(IpcProcessId process_id);
  void RemoveOutboundQueue(OutboundQueue *queue);

  IpcProcessRegistry process_registry_;
  IpcProcessId current_process_id_;
  ThreadMessageQueue *thread_message_queue_;
  ATL::CHandle thread_;
  ThreadId thread_id_;
  std::vector<HANDLE> wait_handles_;
  std::vector<OutboundQueue*> waiting_outbound_queues_;
  scoped_ptr<InboundQueue> inbound_queue_;
  std::map<IpcProcessId, linked_ptr<OutboundQueue> > outbound_queues_;
};



//-----------------------------------------------------------------------------
// OutboundQueue impl
//-----------------------------------------------------------------------------

OutboundQueue::~OutboundQueue() {
  ClearPendingMessages();
  MaybeReleaseWriteMutex();
}


void OutboundQueue::ClearPendingMessages() {
  while (!pending_.empty()) {
    ShareableIpcMessage *message = pending_.front();
    pending_.pop_front();
    message->RemoveReference();
  }  
}


void OutboundQueue::MaybeWaitForWriteMutex() {
  if (!has_write_mutex_ && !is_waiting_for_write_mutex_) {
    is_waiting_for_write_mutex_ = true;
    owner_->AddWaitHandleForQueue(write_mutex_.m_h, this);
  }
}


void OutboundQueue::MaybeReleaseWriteMutex() {
  if (has_write_mutex_) {
    ::ReleaseMutex(write_mutex_.m_h);
    has_write_mutex_ = false;
  }
}


void OutboundQueue::WaitForSpaceAvailable() {
  assert(has_write_mutex_);
  assert(!is_waiting_for_space_available_);
  is_waiting_for_space_available_ = true;
  owner_->AddWaitHandleForQueue(space_available_event_.m_h, this);
}


bool OutboundQueue::Open(IpcProcessId process_id) {
  process_id_ = process_id;
  process_handle_.Attach(::OpenProcess(SYNCHRONIZE, FALSE, process_id));
  if (!process_handle_ ||
      !write_mutex_.Open(process_id) ||
      !io_buffer_.Open(process_id) ||
      !data_available_event_.Open(kQueueDataAvailableEvent, process_id) ||
      !space_available_event_.Open(kQueueSpaceAvailableEvent, process_id)) {
    return false;
  }

  // We wait on the process handle to get notified if it terminates
  owner_->AddWaitHandleForQueue(process_handle_.m_h, this);
  return true;
}


void OutboundQueue::AddMessageToQueue(ShareableIpcMessage *message) {
  message->AddReference();
  pending_.push_back(message);
  MaybeWaitForWriteMutex();
}


void OutboundQueue::WaitComplete(HANDLE wait_handle, bool abandoned) {
  if (wait_handle == write_mutex_.m_h) {
    is_waiting_for_write_mutex_ = false;
    has_write_mutex_ = true;
    WritePendingMessages();
  } else if (wait_handle == space_available_event_.m_h) {
    assert(has_write_mutex_);
    space_available_event_.Reset();
    is_waiting_for_space_available_ = false;
    WritePendingMessages();
  } else if (wait_handle == process_handle_.m_h) {
    ClearPendingMessages();
    MaybeReleaseWriteMutex();
  } else {
    assert(false);
  }
}


void OutboundQueue::WritePendingMessages() {
  assert(has_write_mutex_);

  // write as many pending messages will fit the io buffers available space
  while (!pending_.empty()) {
    ShareableIpcMessage *message = pending_.front();
    if (!WriteOneMessage(message)) {
      break;
    }
    pending_.pop_front();
    message->RemoveReference();
  }

  assert(pending_.empty() || is_waiting_for_space_available_);

  if (pending_.empty()) {
    ::ReleaseMutex(write_mutex_.m_h);
    has_write_mutex_ = false;
  }
}


bool OutboundQueue::WriteOneMessage(ShareableIpcMessage *message) {
  assert(has_write_mutex_);
  assert(message);
  assert(message->serialized_message_data());
  {
    IpcQueueBuffer::WriteTransaction writer;
    if (!writer.Start(&io_buffer_)) {
      ClearPendingMessages();
      return false;
    }

    if (writer.data_available() > 0) {
      // If the buffer is not empty, signal the data_available_event_
      // to recover from a previous process that managed to commit to the
      // buffer, but failed to signal the event (ie. crashed)
      data_available_event_.Set();
    }

    std::vector<uint8> *msg_data = message->serialized_message_data();
    MessageHeader header(owner_->current_process_id_,
                         message->ipc_message_type(),
                         static_cast<int>(msg_data->size()));
    const size_t space_needed = sizeof(header) + header.msg_size;
    if (space_needed > writer.space_available()) {
      if (space_needed > IpcQueueBuffer::kCapacity) {
        // TODO(michaeln): This message is too big to be sent thru this
        // ipc mechanism. We silently drop it on the floor for now. The
        // todo is to support sending messages larger than our io buffer
        // size (at which time we could decrease the size of our io buffer).
        assert(false);
        return true;
      }
      WaitForSpaceAvailable();
      return false;
    }

    writer.Write(&header, sizeof(header));
    if (header.msg_size > 0)
      writer.Write(&((*msg_data)[0]), header.msg_size);
  }
  data_available_event_.Set();
  return true;
}


//-----------------------------------------------------------------------------
// InboundQueue impl
//-----------------------------------------------------------------------------

bool InboundQueue::Create(IpcProcessId process_id) {
  if (!write_mutex_.Create(process_id) ||
      !io_buffer_.Create(process_id) ||
      !data_available_event_.Create(kQueueDataAvailableEvent, process_id) ||
      !space_available_event_.Create(kQueueSpaceAvailableEvent, process_id)) {
    return false;
  }

  // We perpetually wait for new data being dropped into our queue
  owner_->AddWaitHandleForQueue(data_available_event_.m_h, NULL);
  return true;
}


void InboundQueue::WaitComplete(HANDLE wait_handle, bool abandoned) {
  assert(wait_handle == data_available_event_.m_h);
  assert(!abandoned);
  data_available_event_.Reset();
  ReadAndDispatchMessages();
}


void InboundQueue::ReadAndDispatchMessages() {
  IpcProcessId source_process_id;
  int message_type;
  IpcMessageData *message;
  while (ReadOneMessage(&source_process_id, &message_type, &message)) {
    Sleep(1);
    if (message) {
      // TODO(michaeln): For now we echo the message back to the sender
      // for testing purposes. The real impl is the two lines commented
      // out below.
      owner_->Send(source_process_id, message_type, message);
      //CallRegisteredHandler(source_process_id, message_type, message);
      //delete message_data;
    }
  } 
}


bool InboundQueue::ReadOneMessage(IpcProcessId *source_process_id,
                                  int *message_type,
                                  IpcMessageData **message) {
  *source_process_id = 0;
  *message_type = 0;
  *message = NULL;

  MessageHeader header;
  std::vector<uint8> msg_data;
  { 
    IpcQueueBuffer::ReadTransaction reader;
    if (!reader.Start(&io_buffer_)) {
      // TODO(michaeln): Send a crash report, unwind this ipc thread, and
      // start ignore Send and SendToAll method calls.
      assert(false);
      return false;
    }
    if (!reader.data_available()) {
      return false;
    }

    assert(reader.data_available() >= sizeof(header));
    reader.Read(&header, sizeof(header));
    if (header.msg_size > 0) {
      assert(reader.data_available() >= static_cast<size_t>(header.msg_size));
      msg_data.resize(header.msg_size);
      reader.Read(&msg_data[0], header.msg_size);
    }
  }
  space_available_event_.Set();

  if (msg_data.size() > 0) {
    Deserializer deserializer(&msg_data[0], msg_data.size());
    deserializer.CreateAndReadObject(message);
  }
  *source_process_id = header.msg_source;
  *message_type = header.msg_type;
  return true;
}


//-----------------------------------------------------------------------------
// An implementation of the IpcMessageQueue API for Win32
//-----------------------------------------------------------------------------

static Mutex g_instance_lock;
static scoped_ptr<Win32IpcMessageQueue> g_instance(NULL);

// static
IpcMessageQueue *IpcMessageQueue::GetInstance() {
  // TODO(michaeln): temporarily prevent this class from being used until 
  // some more testing is done
  return NULL;
  /*
  if (!g_instance.get()) {
    MutexLock locker(&g_instance_lock);
    if (!g_instance.get()) {
      Win32IpcMessageQueue *instance = new Win32IpcMessageQueue;
      if (!instance->Init())
        delete instance;
      else
        g_instance.reset(instance);
    }
  }
  return g_instance.get();
  */
};


Win32IpcMessageQueue::~Win32IpcMessageQueue() {
  process_registry_.Remove(GetCurrentIpcProcessId());
}


IpcProcessId Win32IpcMessageQueue::GetCurrentIpcProcessId() {
  return current_process_id_;
}


void Win32IpcMessageQueue::SendToAll(int ipc_message_type,
                                     IpcMessageData *ipc_message_data,
                                     bool including_self) {
  IpcThreadMessageEnvelope *envelope = new IpcThreadMessageEnvelope(
                                               ipc_message_type,
                                               ipc_message_data,
                                               including_self);
  thread_message_queue_->Send(thread_id_,
                              kIpcMessageQueue_Send,
                              envelope);
}


void Win32IpcMessageQueue::Send(IpcProcessId dest_process_id,
                                int ipc_message_type,
                                IpcMessageData *ipc_message_data) {
  IpcThreadMessageEnvelope *envelope = new IpcThreadMessageEnvelope(
                                               ipc_message_type,
                                               ipc_message_data,
                                               dest_process_id);
  thread_message_queue_->Send(thread_id_,
                              kIpcMessageQueue_Send,
                              envelope);
}


struct ThreadStartData {
  ThreadStartData(Win32IpcMessageQueue *self) 
    : started_signal_(false),
      started_successfully_(false),
      self(self) {}
  Mutex started_mutex_;
  bool started_signal_;
  bool started_successfully_;
  Win32IpcMessageQueue *self;
};


bool Win32IpcMessageQueue::Init() {
  ThreadStartData start_data(this);
  unsigned int not_used;
  thread_.Attach(reinterpret_cast<HANDLE>(_beginthreadex(
                                              NULL, 0, StaticThreadProc,
                                              &start_data, 0, &not_used)));
  if (!thread_) {
    return false;
  }
  MutexLock locker(&start_data.started_mutex_);
  start_data.started_mutex_.Await(Condition(&start_data.started_signal_));

  // TODO(michaeln): test code, remove me when this class goes live
  if (start_data.started_successfully_) {
    SerializableString16::RegisterSerializableString16();
    SendToAll(1, new SerializableString16(STRING16(L"See spot run")), true);
  }

  return start_data.started_successfully_;
}


// static
unsigned int __stdcall Win32IpcMessageQueue::StaticThreadProc(void *param) {
  if (FAILED(CoInitializeEx(NULL, GEARS_COINIT_THREAD_MODEL))) {
    LOG(("Win32IpcMessageQueue - failed to co-initialize new thread.\n"));
  }
  ThreadStartData *start_data = reinterpret_cast<ThreadStartData*>(param);
  Win32IpcMessageQueue* self = start_data->self;
  self->InstanceThreadProc(start_data);
  CoUninitialize();
  return 0;
}


void Win32IpcMessageQueue::InstanceThreadProc(ThreadStartData *start_data) {
  {
    MutexLock locker(&start_data->started_mutex_);
    current_process_id_ = ::GetCurrentProcessId();
    inbound_queue_.reset(new InboundQueue(this));
    thread_message_queue_ = ThreadMessageQueue::GetInstance();
    if (!thread_message_queue_ ||
        !thread_message_queue_->InitThreadMessageQueue() ||
        !inbound_queue_->Create(current_process_id_) ||
        !process_registry_.Init() ||
        !process_registry_.Add(current_process_id_)) {
      start_data->started_signal_ = true;
      start_data->started_successfully_ = false;
      return;
    }
    thread_message_queue_->RegisterHandler(kIpcMessageQueue_Send, this);
    start_data->started_signal_ = true;
    start_data->started_successfully_ = true;
    thread_id_ = thread_message_queue_->GetCurrentThreadId();
  }

  RunMessageLoop();

  wait_handles_.clear();
  waiting_outbound_queues_.clear();
  outbound_queues_.clear();
  inbound_queue_.reset(NULL);
}


void Win32IpcMessageQueue::RunMessageLoop() {
  // TODO(michaeln): run a timer in this loop and kill stalled OutboundQueues
  bool done = false;
  while (!done) {
    // TODO(michaeln): What is the practical limit to how many handles
    // can be waited on?
    const DWORD kMaxWaitHandles = 64;
    DWORD num_wait_handles = wait_handles_.size();
    if (num_wait_handles > kMaxWaitHandles)
      num_wait_handles = kMaxWaitHandles;

    DWORD rv = MsgWaitForMultipleObjectsEx(
                   num_wait_handles, &wait_handles_[0],
                   INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE | MWMO_ALERTABLE);
    if ((rv >= WAIT_OBJECT_0) && (rv < num_wait_handles)) {
      WaitComplete(rv - WAIT_OBJECT_0, false);
    } else if ((rv >= WAIT_ABANDONED_0) && (rv < num_wait_handles)) {
      WaitComplete(rv - WAIT_ABANDONED_0, true);
    } else if (rv != WAIT_FAILED) {
      // We have message queue input to pump. We pump the queue dry
      // prior to looping and calling MsgWaitForMultipleObjects
      MSG msg;
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    } else {
      done = true;
    }
  }
}


void Win32IpcMessageQueue::AddWaitHandleForQueue(HANDLE wait_handle,
                                                 OutboundQueue *queue) {
  wait_handles_.push_back(wait_handle);
  waiting_outbound_queues_.push_back(queue);
}


void Win32IpcMessageQueue::WaitComplete(int index, bool abandoned) {
  assert(index >= 0 && index < static_cast<int>(wait_handles_.size()));
  HANDLE wait_handle = wait_handles_[index];
  OutboundQueue *queue = waiting_outbound_queues_[index];

  if (!queue) {
    // A null queue indicates this handle is for our InboundQueue.
    inbound_queue_->WaitComplete(wait_handle, abandoned);
  } else {
    wait_handles_.erase(wait_handles_.begin() + index);
    waiting_outbound_queues_.erase(waiting_outbound_queues_.begin() + index);
    queue->WaitComplete(wait_handle, abandoned);
    if (queue->empty()) {
      RemoveOutboundQueue(queue);
    }
  }
}


void Win32IpcMessageQueue::HandleThreadMessage(int message_type,
                                               MessageData *message_data) {
  assert(message_type == kIpcMessageQueue_Send);
  IpcThreadMessageEnvelope *envelope =
      reinterpret_cast<IpcThreadMessageEnvelope*>(message_data);
  if (envelope->shareable_message_->serialized_message_data()) {
    if (envelope->send_to_all_) {
      HandleSendToAll(envelope);
    } else {
      HandleSendToOne(envelope);
    }
  }
}


void Win32IpcMessageQueue::HandleSendToAll(
                               IpcThreadMessageEnvelope *envelope) {
  std::vector<IpcProcessId> processes;
  process_registry_.GetAll(&processes);
  for (std::vector<IpcProcessId>::iterator iter = processes.begin();
       iter != processes.end(); iter++) {
    if (envelope->include_self_ || *iter != current_process_id_) {
      OutboundQueue *outbound_queue = GetOutboundQueue(*iter);
      if (outbound_queue)
        outbound_queue->AddMessageToQueue(envelope->shareable_message_);
      else if (current_process_id_ != *iter)
        process_registry_.Remove(*iter);
    } 
  }  
}


void Win32IpcMessageQueue::HandleSendToOne(
                               IpcThreadMessageEnvelope *envelope) {
  OutboundQueue *outbound_queue =
      GetOutboundQueue(envelope->destination_process_id_);
  if (outbound_queue)
    outbound_queue->AddMessageToQueue(envelope->shareable_message_);
}


OutboundQueue *Win32IpcMessageQueue::GetOutboundQueue(
                                         IpcProcessId process_id) {
  std::map<IpcProcessId, linked_ptr<OutboundQueue> >::iterator iter;
  iter = outbound_queues_.find(process_id);
  if (iter != outbound_queues_.end()) {
    return iter->second.get();
  }
  OutboundQueue *queue = new OutboundQueue(this);
  if (!queue->Open(process_id)) {
    delete queue;
    return NULL;
  }
  outbound_queues_[process_id] = linked_ptr<OutboundQueue>(queue);
  return queue;
}


void Win32IpcMessageQueue::RemoveOutboundQueue(OutboundQueue *queue) {
  // TODO(michaeln): avoid this linear search?
  for (int i = waiting_outbound_queues_.size() - 1; i >= 0; --i) {
    if (waiting_outbound_queues_[i] == queue) {
      waiting_outbound_queues_.erase(waiting_outbound_queues_.begin() + i);
      wait_handles_.erase(wait_handles_.begin() + i);
    }
  }
  outbound_queues_.erase(queue->process_id());
}
