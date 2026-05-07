#ifndef ENTITY_H_
#define ENTITY_H_

#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QSizeF>

class Entity {
 public:
  Entity() = default;
  Entity(const Entity&) = default;
  Entity& operator=(const Entity&) = default;
  Entity(Entity&&) = default;
  Entity& operator=(Entity&&) = default;
  virtual ~Entity() = default;

  virtual void update(float dt) = 0;
  virtual void draw(QPainter* painter) = 0;

  [[nodiscard]] QPointF position() const { return position_; }
  [[nodiscard]] QSizeF size() const { return size_; }
  [[nodiscard]] const QPixmap& sprite() const { return sprite_; }

  void set_position(const QPointF& position) { position_ = position; }
  void set_size(const QSizeF& size) { size_ = size; }
  void set_sprite(const QPixmap& sprite) { sprite_ = sprite; }

  [[nodiscard]] bool is_alive() const { return is_alive_; }
  void kill() { is_alive_ = false; }

  // 视觉矩形（世界坐标）
  [[nodiscard]] QRectF visual_rect() const { return QRectF(position_, size_); }

  // 碰撞箱：在视觉矩形基础上缩小 10%（提升手感）
  [[nodiscard]] QRectF collision_rect() const {
    const QRectF rect = visual_rect();
    const qreal shrink_x = rect.width() * 0.10;
    const qreal shrink_y = rect.height() * 0.10;
    return rect.adjusted(shrink_x * 0.5, shrink_y * 0.5, -shrink_x * 0.5,
                         -shrink_y * 0.5);
  }

 protected:
  QPointF position_{0.0, 0.0};
  QSizeF size_{0.0, 0.0};
  QPixmap sprite_;
  bool is_alive_ = true;
};

#endif  // ENTITY_H_

