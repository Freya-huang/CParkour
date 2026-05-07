#ifndef DOG_ITEM_H_
#define DOG_ITEM_H_

#include "Entity.h"

class DogItem final : public Entity {
 public:
  // 必须使用初始化列表（position/size/sprite 全在初始化列表完成）
  explicit DogItem(const QPointF& position);

  void update(float dt) override;
  void draw(QPainter* painter) override;

  void translate_x(qreal dx) { position_.setX(position_.x() + dx); }

  void jump();
  void on_jump_key_released();

  [[nodiscard]] bool is_grounded() const { return is_grounded_; }
  void set_grounded(bool grounded) { is_grounded_ = grounded; }

  [[nodiscard]] float velocity_y() const { return velocity_y_; }
  void set_velocity_y(float vy) { velocity_y_ = vy; }

  [[nodiscard]] QPointF previous_position() const { return previous_position_; }

 private:
  QPointF previous_position_{0.0, 0.0};
  float velocity_y_ = 0.0f;
  bool is_grounded_ = false;

  // 跑步帧动画（V4）：每 kDogAnimFrameMs 切换一次
  QPixmap run_sprite1_;
  QPixmap run_sprite2_;
  int anim_accumulator_ms_ = 0;
  bool use_sprite1_ = true;
};

#endif  // DOG_ITEM_H_

