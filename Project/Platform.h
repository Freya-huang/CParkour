#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "Entity.h"

class Platform final : public Entity {
 public:
  Platform(const QPointF& position, const QSizeF& size);

  void update(float dt) override;
  void draw(QPainter* painter) override;
};

#endif  // PLATFORM_H_

