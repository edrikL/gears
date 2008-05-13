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
#if USING_CCTESTS
#include <map>

#include "gears/notifier/balloon_collection.h"

void TestNotificationManager();

class BalloonCollectionMock : public BalloonCollectionInterface {
 public:
  BalloonCollectionMock();
  virtual ~BalloonCollectionMock();

  virtual void Show(const Notification &notification);
  virtual bool Update(const Notification &notification);
  virtual bool Delete(const std::string16 &service,
                      const std::string16 &bare_id);

  virtual bool has_space();
  void set_has_space(bool has_space);

  void set_show_call_count(int show_call_count);
  void set_update_call_count(int update_call_count);
  void set_delete_call_count(int delete_call_count);

 private:
  bool has_space_;
  int show_call_count_;
  int update_call_count_;
  int delete_call_count_;

  typedef std::pair<std::string16, std::string16> NotificationId;

  std::map<NotificationId, int> displayed_;
  DISALLOW_EVIL_CONSTRUCTORS(BalloonCollectionMock);
};
#endif  // USING_CCTESTS
