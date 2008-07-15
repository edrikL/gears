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

#import <Cocoa/Cocoa.h>
#import <unistd.h>
#import <vector>

#import "gears/base/common/common.h"
#import "gears/base/common/ipc_message_queue.h"
#import "third_party/scoped_ptr/scoped_ptr.h"

static const int kGearsIpcVersion = 1;
static const char *kGearsIpcMessageName = "GearsIpc";
static const NSString *kSrcProcessIdKey = @"SrcProcessId";
static const NSString *kMessageTypeKey = @"MessageType";
static const NSString *kMessageDataKey = @"MessageData";

class NotificationMessageHandler {
 public:
  virtual void HandleMessage(IpcProcessId src_process_id,
                             int message_type,
                             std::vector<uint8> *message_data) = 0;
};

@interface NotificationMessageCenter : NSObject {
 @private
  NotificationMessageHandler *handler_;
  IpcProcessId current_process_id_;
}

#pragma mark Public Class methods

// Converts a process ID into an IPC Channel name string.
+ (NSString *)ipcChannelName:(IpcProcessId)process_id;

#pragma mark Public Instance methods

// Initializes the IPC message center for a process.
- (id)initWithArguments:(NotificationMessageHandler *)handler
       currentProcessId:(IpcProcessId)current_process_id;

// Terminates the IPC message center.
- (void)terminate;

// Sends an IPC message.
- (void)sendMessage:(IpcProcessId)dest_process_id
        messageType:(int)message_type
        messageData:(const std::vector<uint8> *)message_data;

#pragma mark NSDistributedNotificationCenter callbacks

// Handles an IPC message. This callback is registered with
// NSDistributedNotificationCenter.
- (void)handleMessage:(NSNotification *)notification;

@end

@implementation NotificationMessageCenter

#pragma mark Public Class methods

+ (NSString *)ipcChannelName:(IpcProcessId)process_id {
  return [NSString stringWithFormat:@"%s:%d:%d", kGearsIpcMessageName,
            process_id, kGearsIpcVersion];
}

#pragma mark Public Instance methods

- (id)initWithArguments:(NotificationMessageHandler *)handler
       currentProcessId:(IpcProcessId)current_process_id {
  assert(handler);

  self = [super init];
  if (self) {
    NSString *ipc_name =
        [NotificationMessageCenter ipcChannelName:current_process_id];

    [[NSDistributedNotificationCenter defaultCenter]
                 addObserver:self
                   selector:@selector(handleMessage:)
                       name:ipc_name
                     object:nil
         suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];

    handler_ = handler;
    current_process_id_ = current_process_id;
  }
  return self;
}

- (void)terminate {
  [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
}

- (void)sendMessage:(IpcProcessId)dest_process_id
        messageType:(int)message_type
        messageData:(const std::vector<uint8> *)message_data {
  NSString *sender = 
      [NotificationMessageCenter ipcChannelName:current_process_id_];

  NSNumber *ns_src_process_id =
      [NSNumber numberWithUnsignedInt:current_process_id_];
  NSNumber *ns_message_type =
      [NSNumber numberWithInt:message_type];
  NSData *ns_message_data =
      [NSData dataWithBytes:&message_data->at(0) length:message_data->size()];

  NSDictionary *info = [NSDictionary dictionaryWithObjectsAndKeys:
      ns_src_process_id, kSrcProcessIdKey,
      ns_message_type, kMessageTypeKey,
      ns_message_data, kMessageDataKey,
      nil];

  NSString *ipc_name =
      [NotificationMessageCenter ipcChannelName:dest_process_id];

  [[NSDistributedNotificationCenter defaultCenter]
      postNotificationName:ipc_name
                    object:sender
                  userInfo:info
        deliverImmediately:YES];
}

#pragma mark NSDistributedNotificationCenter callbacks

- (void)handleMessage:(NSNotification *)notification {
  NSString *sender = [notification object];
  assert(sender);
  if (!sender) {
    return;
  }

  NSDictionary *info = [notification userInfo];
  assert(info);
  if (!info) {
    return;
  }
  IpcProcessId src_process_id =
      [[info objectForKey:kSrcProcessIdKey] unsignedIntValue];
  assert(src_process_id);
  if (!src_process_id) {
    return;
  }
  int message_type = [[info objectForKey:kMessageTypeKey] intValue];
  assert(message_type);
  if (!message_type) {
    return;
  }
  NSData *data = [info objectForKey:kMessageDataKey];
  assert(data && [data length]);
  if (!data || ![data length]) {
    return;
  }

  std::vector<uint8> message_data;
  message_data.resize([data length]);
  memcpy(&message_data.at(0), [data bytes], [data length]);
  
  LOG(("Received a message with type %d from process %d to %d\n",
       message_type, src_process_id, current_process_id_));

  handler_->HandleMessage(src_process_id, message_type, &message_data);
}

@end


// TODO(jianli): use a worker thread to deserialize and dispatch messages.
class MacIpcMessageQueue : public IpcMessageQueue,
                           public NotificationMessageHandler {
 public:
  MacIpcMessageQueue();
  ~MacIpcMessageQueue();
  bool Init();
  void Terminate();

  // NotificationMessageHandler interface
  virtual void HandleMessage(IpcProcessId src_process_id,
                             int message_type,
                             std::vector<uint8> *message_data);

  // IpcMessageQueue overrides
  virtual IpcProcessId GetCurrentIpcProcessId();

  // Does not support sending a message to itself.
  virtual void Send(IpcProcessId dest_process_id,
                    int message_type,
                    IpcMessageData *message_data);

  // Not supported.
  virtual void SendToAll(int message_type,
                         IpcMessageData *message_data,
                         bool including_current_process);
 private:
  bool die_;
  IpcProcessId current_process_id_;
  NotificationMessageCenter *notification_center_;                 
};

MacIpcMessageQueue::MacIpcMessageQueue()
    : die_(true),
      current_process_id_(0),
      notification_center_(nil) {
  current_process_id_ = getpid();    
}

MacIpcMessageQueue::~MacIpcMessageQueue() {
  Terminate();
}

bool MacIpcMessageQueue::Init() {
  notification_center_ = [[NotificationMessageCenter alloc]
                              initWithArguments:this
                               currentProcessId:current_process_id_];
  if (!notification_center_) {
    return false;
  }

  die_ = false;

  return true;
}

void MacIpcMessageQueue::Terminate() {
  die_ = true;

  [notification_center_ terminate];
  [notification_center_ release];
  notification_center_ = nil;
}

void MacIpcMessageQueue::HandleMessage(IpcProcessId src_process_id,
                                       int message_type,
                                       std::vector<uint8> *message_data) {
  assert(!message_data->empty());
                                       
  // Deserialize the message.                                       
  Deserializer deserializer(&message_data->at(0), message_data->size());
  IpcMessageData *message = NULL;
  if (deserializer.CreateAndReadObject(&message) && message != NULL) {
    // Call the handler.
    CallRegisteredHandler(src_process_id, message_type, message);
    delete message;
  } else {
    LOG(("Failed to deserialize the message data received from %u\n",
         src_process_id));
  }
}

IpcProcessId MacIpcMessageQueue::GetCurrentIpcProcessId() {
  return current_process_id_;
}

void MacIpcMessageQueue::Send(IpcProcessId dest_process_id,
                              int message_type,
                              IpcMessageData *message_data) {
  // Does not support sending a message to itself.
  assert(dest_process_id != static_cast<IpcProcessId>(getpid()));

  if (die_) {
    delete message_data;
    return;
  }

  // Make sure that the message data will be deleted when the function returns.
  scoped_ptr<IpcMessageData> message_data_guard(message_data);

  // Serialize the message.
  std::vector<uint8> serialized_message_data;
  Serializer serializer(&serialized_message_data);
  if (!serializer.WriteObject(message_data)) {
    LOG(("Failed to serialize the message data sent to %u\n", dest_process_id));
    return;
  }                  

  // Send the message.
  [notification_center_ sendMessage:dest_process_id
                        messageType:message_type
                        messageData:&serialized_message_data];

  LOG(("Sent a message with type %d from process %d to %d\n",
       message_type, current_process_id_, dest_process_id));                                   
}

void MacIpcMessageQueue::SendToAll(int message_type,
                                   IpcMessageData *message_data,
                                   bool including_current_process) {
  // Not supported.
  assert(false);
}

namespace {
Mutex g_system_queue_instance_lock;
MacIpcMessageQueue * volatile g_system_queue_instance = NULL;
}

IpcMessageQueue *IpcMessageQueue::GetSystemQueue() {
  if (!g_system_queue_instance) {
    MutexLock locker(&g_system_queue_instance_lock);
    if (!g_system_queue_instance) {
      MacIpcMessageQueue *instance = new MacIpcMessageQueue();
      if (!instance->Init()) {
        LOG(("IpcMessageQueue initialization failed.\n"));
      }
      g_system_queue_instance = instance;
    }
  }
  return g_system_queue_instance;
}

IpcMessageQueue *IpcMessageQueue::GetPeerQueue() {
  // Not supported.
  return NULL;
}

#ifdef USING_CCTESTS
void TestingIpcMessageQueue_GetCounters(IpcMessageQueueCounters *counters,
                                        bool reset) {
  // Not supported. Always set to zero.
  if (counters)
    memset(&counters, 0, sizeof(IpcMessageQueueCounters));
}
#endif  // USING_CCTESTS
