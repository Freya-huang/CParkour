#include "DogItem.h"

#include "Config.h"

#include <QDebug>

namespace {
static constexpr qreal kDefaultDogWidth = 96.0;
static constexpr qreal kDefaultDogHeight = 96.0;
}  // namespace

DogItem::DogItem(const QPointF& position)
    : Entity(),
      previous_position_(position),
      velocity_y_(0.0f),
      is_grounded_(false),
      run_sprite1_(),
      run_sprite2_(),
      anim_accumulator_ms_(0),
      use_sprite1_(true) {
  position_ = position;
  size_ = QSizeF(kDefaultDogWidth, kDefaultDogHeight);

  run_sprite1_ = QPixmap(":/images/dog_run1.png");
  run_sprite2_ = QPixmap(":/images/dog_run2.png");
  sprite_ = run_sprite1_;

  // run1 必须存在：否则直接中止（核心素材）
  if (run_sprite1_.isNull()) {
    qWarning() << "Failed to load sprite from resource:"
               << ":/images/dog_run1.png"
               << "Please add images/dog_run1.png into resources.qrc.";
    Q_ASSERT_X(false, "DogItem::DogItem",
               "Sprite load failed: :/images/dog_run1.png");
  }

  // run2 可选：若缺失则回退到 run1，避免运行时崩溃
  if (run_sprite2_.isNull()) {
    qWarning() << "Optional sprite missing:"
               << ":/images/dog_run2.png"
               << "Fallback to dog_run1.png (animation disabled).";
    run_sprite2_ = run_sprite1_;
  }
}

void DogItem::jump() {
  /*
  if (!is_grounded_) {
    return;
  }
  */

  velocity_y_ = config::kJumpImpulse;
  is_grounded_ = false;
}

void DogItem::on_jump_key_released() {
  // 可控跳跃高度：提前松开且仍在上冲（vy < 0），则衰减上冲速度。
  // 但不会低于最小速度，保证最小跳跃高度。
  if (velocity_y_ < 0.0f) {
    velocity_y_ *= config::kJumpReleaseMultiplier;
    // 限制最小跳跃速度，避免跳跃过低
    if (velocity_y_ > config::kMinJumpVelocity) {
      velocity_y_ = config::kMinJumpVelocity;
    }
  }
}

void DogItem::update(float dt) {
  previous_position_ = position_;

  const float g = config::kGravity;
  const float vy0 = velocity_y_;

  // 位移公式：y += v_y * dt + 0.5 * g * dt^2
  position_.setY(position_.y() + vy0 * dt + 0.5f * g * dt * dt);

  // 速度更新：v_y += g * dt
  velocity_y_ = vy0 + g * dt;

  // 帧动画：每 100ms 切换一次（run1 <-> run2）
  anim_accumulator_ms_ += static_cast<int>(dt * 1000.0f);
  while (anim_accumulator_ms_ >= config::kDogAnimFrameMs) {
    anim_accumulator_ms_ -= config::kDogAnimFrameMs;
    use_sprite1_ = !use_sprite1_;
  }
  sprite_ = use_sprite1_ ? run_sprite1_ : run_sprite2_;
}

void DogItem::draw(QPainter* painter) {
  if (painter == nullptr) {
    return;
  }

  // V1：世界坐标=屏幕坐标（相机在 V3 引入）
  const QRectF dst_rect(position_, size_);
  painter->drawPixmap(dst_rect, sprite_, sprite_.rect());
}

