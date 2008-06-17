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

// Balloon collection is a queue of balloons with notifications currently
// displayed on the main screen. The notifications expire with time and
// disappear, at which moment they are removed from the collection.

#ifdef OFFICIAL_BUILD
  // The notification API has not been finalized for official builds.
#else
#include <string>
#include <assert.h>

#include "gears/notifier/balloons.h"
#include "gears/base/common/string_utils.h"
#include "gears/notifier/system.h"
#include "third_party/glint/include/button.h"
#include "third_party/glint/include/color.h"
#include "third_party/glint/include/column.h"
#include "third_party/glint/include/nine_grid.h"
#include "third_party/glint/include/platform.h"
#include "third_party/glint/include/root_ui.h"
#include "third_party/glint/include/row.h"
#include "third_party/glint/include/simple_text.h"

static const char *kTitleId = "title_text";
static const char *kSubtitleId = "subtitle_text";
static const char *kBalloonContainer = "balloon_container";

BalloonCollection::BalloonCollection(BalloonCollectionObserver *observer)
  : observer_(observer),
    root_ui_(NULL),
    has_space_(true) {
  assert(observer);
  EnsureRoot();
  observer_->OnBalloonSpaceChanged();
}

BalloonCollection::~BalloonCollection() {
  Clear();
}

void BalloonCollection::Clear() {
  for (Balloons::iterator it = balloons_.begin();
       it != balloons_.end();
       ++it) {
    Balloon *removed = *it;
    // TODO(dimich): Do we need this here? it = balloons_->erase(it);
    delete removed;
  }
}

Balloon *BalloonCollection::FindBalloon(const std::string16 &service,
                                        const std::string16 &id,
                                        bool and_remove) {
  for (Balloons::iterator it = balloons_.begin();
       it != balloons_.end();
       ++it) {
    if ((*it)->notification().Matches(service, id)) {
      Balloon *result = *it;
      if (and_remove) {
        balloons_.erase(it);
      }
      return result;
    }
  }
  return NULL;
}

void BalloonCollection::Show(const GearsNotification &notification) {
  Balloon *balloon = FindBalloon(notification.service(),
                                 notification.id(),
                                 false);  // no remove
  assert(!balloon);
  // If someone repeatedly adds the same notification, avoid flood - return.
  if (balloon)
    return;

  Balloon *new_balloon = new Balloon(notification, this);
  balloons_.push_front(new_balloon);
  AddToUI(new_balloon);
  observer_->OnBalloonSpaceChanged();
}

bool BalloonCollection::Update(const GearsNotification &notification) {
  Balloon *balloon = FindBalloon(notification.service(),
                                 notification.id(),
                                 false);  // no remove
  if (!balloon)
    return false;
  balloon->mutable_notification()->CopyFrom(notification);
  balloon->UpdateUI();
  return true;
}

bool BalloonCollection::Delete(const std::string16 &service,
                               const std::string16 &id) {
  Balloon *balloon = FindBalloon(service,
                                 id,
                                 false);  // don't remove from list yet
  if (!balloon)
    return false;

  if (!StartBalloonClose(balloon, false))  // not user-initiated.
    return false;

  return true;
}

void BalloonCollection::EnsureRoot() {
  if (root_ui_)
    return;

  root_ui_ = new glint::RootUI(true);  // topmost window
  glint::Row *container = new glint::Row();
  container->set_direction(glint::Row::BOTTOM_TO_TOP);
  container->ReplaceDistribution("natural");
  container->set_background(glint::Color(0x44444444));
  container->set_id(kBalloonContainer);
  glint::Transform offset;
  offset.AddOffset(glint::Vector(500.0f, 250.0f));
  container->set_transform(offset);
  root_ui_->set_root_node(container);
  root_ui_->Show();
}

bool BalloonCollection::AddToUI(Balloon *balloon) {
  assert(balloon);
  EnsureRoot();
  glint::Row *container = static_cast<glint::Row*>(
      root_ui_->FindNodeById(kBalloonContainer));
  if (!container)
    return false;
  container->AddChild(balloon->ui_root());
  balloon->UpdateUI();
  return true;
}

bool BalloonCollection::RemoveFromUI(Balloon *balloon) {
  assert(balloon);
  Balloon *found = FindBalloon(balloon->notification().service(),
                               balloon->notification().id(),
                               true);  // remove from list
  if (!found || !root_ui_)
    return false;

  glint::Row *container = static_cast<glint::Row*>(
      root_ui_->FindNodeById(kBalloonContainer));
  if (!container)
    return false;

  container->RemoveNode(balloon->ui_root());
  observer_->OnBalloonSpaceChanged();
  return true;
}

bool BalloonCollection::StartBalloonClose(Balloon *balloon,
                                          bool user_initiated) {
  assert(balloon);
  // TODO(dimich): set up alpha transition and do interruptible alpha animation.
  // At the end of animation, signal to the collection.
  // Do this when animation completes:
  if (!RemoveFromUI(balloon))
    return false;
  delete balloon;
  return true;
}

Balloon::Balloon(const GearsNotification &from, BalloonCollection *collection)
  : ui_root_(NULL),
    collection_(collection)  {
  assert(collection);
  notification_.CopyFrom(from);
}

Balloon::~Balloon() {
}

glint::Node *Balloon::CreateTree() {
  glint::Node *root = new glint::Node();
  root->set_min_width(300);
  root->set_min_height(100);

  glint::NineGrid *background = new glint::NineGrid();
  background->ReplaceImage("res://background.png");
  background->set_center_height(10);
  background->set_center_width(10);
  background->set_shadow(true);
  root->AddChild(background);

  glint::Column *column = new glint::Column();
  column->set_background(glint::Color(0xFFFF00));
  root->AddChild(column);

  glint::SimpleText *text = new glint::SimpleText();
  text->set_id(kTitleId);
  text->set_font_size(14);
  glint::Rectangle margin;
  margin.Set(10, 10, 10, 10);
  text->set_margin(margin);
  column->AddChild(text);

  text = new glint::SimpleText();
  text->set_id(kSubtitleId);
  margin.Set(2, 3, 2, 3);
  text->set_margin(margin);
  column->AddChild(text);

  glint::Row *buttons = new glint::Row();
  margin.Set(5, 5, 5, 5);
  buttons->set_margin(margin);
  buttons->set_horizontal_alignment(glint::X_RIGHT);
  column->AddChild(buttons);

  glint::Button *close_button = new glint::Button();
  close_button->ReplaceImage("res://button_strip.png");
  close_button->set_min_height(22);
  close_button->set_min_width(70);
  buttons->AddChild(close_button);
  text = new glint::SimpleText();
  text->set_text("Close");
  margin.Set(3, 3, 3, 3);
  text->set_margin(margin);
  close_button->AddChild(text);
  close_button->SetClickHandler(Balloon::OnCloseButton, this);

  return root;
}

class CloseButtonAction : public glint::WorkItem {
 public:
  CloseButtonAction(BalloonCollection *collection,
                    Balloon *balloon_to_close,
                    bool by_user)
    : collection_(collection),
      balloon_to_close_(balloon_to_close),
      by_user_(by_user) {
    assert(collection && balloon_to_close);
  }
  virtual void Run() {
    collection_->StartBalloonClose(balloon_to_close_, by_user_);
  }
 private:
  BalloonCollection *collection_;
  Balloon *balloon_to_close_;
  bool by_user_;
};

void Balloon::OnCloseButton(const std::string &button_id, void *user_info) {
  Balloon *this_ = reinterpret_cast<Balloon*>(user_info);
  assert(this_);
  assert(this_->collection_);
  CloseButtonAction *async_work_item =
      new CloseButtonAction(this_->collection_,
                            this_,
                            true);  // true == 'user-initiated'
  if (!async_work_item)
    return;
  glint::platform()->PostWorkItem(NULL, async_work_item);
}

bool Balloon::SetTextField(const char *id, const std::string16 &text) {
  assert(id);
  glint::SimpleText *text_node = static_cast<glint::SimpleText*>(
      ui_root_->FindNodeById(id));
  if (!text_node)
    return false;
  std::string text_utf8;
  if (!String16ToUTF8(text.c_str(), &text_utf8))
    return false;
  text_node->set_text(text_utf8);
  return true;
}

void Balloon::UpdateUI() {
  SetTextField(kTitleId, notification_.title());
  SetTextField(kSubtitleId, notification_.subtitle());
}

#endif  // OFFICIAL_BULID
