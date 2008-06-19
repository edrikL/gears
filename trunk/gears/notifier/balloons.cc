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
#include "third_party/glint/include/animation_timeline.h"
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

const double kAlphaTransitionDuration = 1.0;
const double kAlphaTransitionDurationShort = 0.3;
const double kAlphaTransitionDurationForRestore = 0.0;

class AnimationCompletedHandler : public glint::MessageHandler {
 public:
  AnimationCompletedHandler(Balloon *balloon) : balloon_(balloon) {
    assert(balloon);
  }

  glint::MessageResultCode HandleMessage(const glint::Message &message) {
    if (message.code == glint::GL_MSG_ANIMATION_COMPLETED) {
      balloon_->OnAnimationCompleted();
    }
    return glint::MESSAGE_CONTINUE;
  }

 private:
  Balloon *balloon_;
  DISALLOW_EVIL_CONSTRUCTORS(AnimationCompletedHandler);
};

class RemoveWorkItem : public glint::WorkItem {
 public:
  RemoveWorkItem(BalloonCollection *collection,
                 Balloon *balloon)
    : collection_(collection), balloon_(balloon) {
    assert(collection_);
    assert(balloon_);
  }
  virtual void Run() {
    collection_->RemoveFromUI(balloon_);
    delete balloon_;
  }
 private:
  BalloonCollection *collection_;
  Balloon *balloon_;
  DISALLOW_EVIL_CONSTRUCTORS(RemoveWorkItem);
};

class MouseWithinDetector : public glint::MessageHandler {
 public:
  MouseWithinDetector(Balloon *balloon, glint::Node *owner)
    : owner_(owner),
      balloon_(balloon),
      mouse_within_(false) {
  }

  ~MouseWithinDetector() {
    // Make sure we reset notification expiration counter.
    if (mouse_within_) {
      balloon_->OnMouseOut();
    }
  }

  glint::MessageResultCode HandleMessage(const glint::Message &message) {
    bool mouse_within_now = mouse_within_;
    if (message.code == glint::GL_MSG_MOUSELEAVE) {
      // Mouse leaves the application's window - signal "mouse out".
      mouse_within_now = false;
    } else if (message.code == glint::GL_MSG_MOUSEMOVE_BROADCAST) {
      // GL_MSG_MOUSEMOVE_BROADCAST comes to all nodes even if the mouse
      // is outside - we use it to detect the mouse in/out condition.
      glint::Rectangle bounds(glint::Point(), owner_->final_size());
      // GL_MSG_MOUSEMOVE_BROADCAST comes with screen coordinate of the
      // mouse, so transform it first.
      glint::Point mouse = message.GetLocalPosition(owner_);
      mouse_within_now = bounds.Contains(mouse);
    }
    // Detect change in mouse "withinness".
    if (mouse_within_now != mouse_within_) {
      mouse_within_ = mouse_within_now;
      if (mouse_within_) {
        balloon_->OnMouseIn();
      } else {
        balloon_->OnMouseOut();
      }
    }
    return glint::MESSAGE_CONTINUE;
  }

 private:
  glint::Node *owner_;
  Balloon *balloon_;
  bool mouse_within_;
  DISALLOW_EVIL_CONSTRUCTORS(MouseWithinDetector);
};

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

  if (!balloon->InitiateClose(false))  // not user-initiated.
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
  return balloon->InitializeUI(container);
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

  container->RemoveNode(balloon->root());
  observer_->OnBalloonSpaceChanged();
  return true;
}

Balloon::Balloon(const GearsNotification &from, BalloonCollection *collection)
  : root_(NULL),
    collection_(collection),
    state_(OPENING_BALLOON)  {
  assert(collection_);
  notification_.CopyFrom(from);
}

Balloon::~Balloon() {
}

glint::Node *Balloon::CreateTree() {
  glint::Node *root = new glint::Node();
  root->set_min_width(300);
  root->set_min_height(100);
  root->set_alpha(glint::colors::kTransparentAlpha);

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

  root->AddHandler(new AnimationCompletedHandler(this));
  root->AddHandler(new MouseWithinDetector(this, root));
  SetAlphaTransition(root, kAlphaTransitionDuration);

  return root;
}

void Balloon::OnCloseButton(const std::string &button_id, void *user_info) {
  Balloon *this_ = reinterpret_cast<Balloon*>(user_info);
  assert(this_);
  this_->InitiateClose(true);  // true == 'user_initiated'
}

bool Balloon::SetTextField(const char *id, const std::string16 &text) {
  assert(id);
  glint::SimpleText *text_node = static_cast<glint::SimpleText*>(
      root_->FindNodeById(id));
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

bool Balloon::SetAlphaTransition(glint::Node *node,
                                 double transition_duration) {
  if (!node)
    return false;

  glint::AnimationTimeline *timeline = NULL;
  // Consider transition with suration <0.1s an abrupt transition.
  if (transition_duration > 0.1) {
    timeline = new glint::AnimationTimeline();

    glint::AlphaAnimationSegment *segment = new glint::AlphaAnimationSegment();
    segment->set_type(glint::RELATIVE_TO_START);
    timeline->AddAlphaSegment(segment);

    segment = new glint::AlphaAnimationSegment();
    segment->set_type(glint::RELATIVE_TO_FINAL);
    segment->set_duration(transition_duration);
    timeline->AddAlphaSegment(segment);

    timeline->RequestCompletionMessage(node);
  }
  node->SetTransition(glint::ALPHA_TRANSITION, timeline);
  return true;
}

bool Balloon::InitiateClose(bool user_initiated) {
  if (state_ != SHOWING_BALLOON)
    return true;

  state_ = user_initiated ? USER_CLOSING_BALLOON : AUTO_CLOSING_BALLOON;

  // If closed by user, set shorter transition.
  if (user_initiated) {
    SetAlphaTransition(root_, kAlphaTransitionDurationShort);
  }

  root_->set_alpha(glint::colors::kTransparentAlpha);
  return true;
}

void Balloon::OnAnimationCompleted() {
  switch (state_) {
    case OPENING_BALLOON:
      state_ = SHOWING_BALLOON;
      break;

    case AUTO_CLOSING_BALLOON:
    case USER_CLOSING_BALLOON: {
      // Need to do this async because 'animation completion' message gets
      // fired from synchronous tree walk and it's bad to remove part of
      // the tree during the walk.
      RemoveWorkItem* remove_work = new RemoveWorkItem(collection_, this);
      glint::platform()->PostWorkItem(NULL, remove_work);
      break;
    }

    case RESTORING_BALLOON:
      state_ = SHOWING_BALLOON;
      SetAlphaTransition(root_, kAlphaTransitionDuration);
      break;

    case SHOWING_BALLOON:
      // We might be doing some other animation while showing the balloon,
      // like the scaling animation when the system font changes.
      break;

    default:
      assert(false);
      break;
  }
}

void Balloon::OnMouseIn() {
  // If closing it automatically, bring it back
  if (state_ == AUTO_CLOSING_BALLOON) {
    // Cancel current animation
    SetAlphaTransition(root_, 0);

    // Restore the alpha to opaque immediately
    SetAlphaTransition(root_, kAlphaTransitionDurationForRestore);
    root_->set_alpha(glint::colors::kOpaqueAlpha);

    state_ = RESTORING_BALLOON;
  }

  // TODO(dimich): collection_->SuspendExpirationTimer();
}

void Balloon::OnMouseOut() {
  // TODO(dimich): collection_->RestoreExpirationTimer();
}

bool Balloon::InitializeUI(glint::Node *container) {
  assert(container);
  assert(!root_);
  // Some operations (triggering animations for example) require node to be
  // already connected to a RootUI. So first create tree, hook it up to
  // container and then complete initialization.
  root_ = CreateTree();
  if (!root_)
    return false;

  container->AddChild(root_);
  UpdateUI();

  // Start revealing animation.
  state_ = OPENING_BALLOON;
  root_->set_alpha(glint::colors::kOpaqueAlpha);
  return true;
}


#endif  // OFFICIAL_BULID
