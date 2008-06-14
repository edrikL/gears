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
#include "third_party/glint/include/color.h"
#include "third_party/glint/include/column.h"
#include "third_party/glint/include/nine_grid.h"
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

  Balloon *new_balloon = new Balloon(notification);
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
                                 true);  // remove from list
  if (!balloon)
    return false;

  RemoveFromUI(balloon);
  delete balloon;

  observer_->OnBalloonSpaceChanged();
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
  root_ui_->set_root_node(container);
}

void BalloonCollection::AddToUI(Balloon *balloon) {
  assert(balloon);
  EnsureRoot();
  glint::Row *container = static_cast<glint::Row*>(
      root_ui_->FindNodeById(kBalloonContainer));
  if (!container)
    return;
  container->AddChild(balloon->ui_root());
}

void BalloonCollection::RemoveFromUI(Balloon *balloon) {
  assert(balloon);
  if (!root_ui_)
    return;
  glint::Row *container = static_cast<glint::Row*>(
      root_ui_->FindNodeById(kBalloonContainer));
  if (!container)
    return;
  container->RemoveNode(balloon->ui_root());
}

Balloon::Balloon(const GearsNotification &from) : ui_root_(NULL) {
  notification_.CopyFrom(from);
}

Balloon::~Balloon() {
}

glint::Node *Balloon::CreateTree() {
  glint::Node *root = new glint::Node();
  root->set_min_width(300);
  root->set_min_height(100);
  glint::Transform offset;
  offset.AddOffset(glint::Vector(250.0f, 250.0f));
  root->set_transform(offset);

  glint::NineGrid *background = new glint::NineGrid();
  background->ReplaceImage("background.png");
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

  return root;
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
