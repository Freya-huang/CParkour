#ifndef OBSTACLE_H_
#define OBSTACLE_H_

#include "Entity.h"

class Obstacle final : public Entity {
 public:
  explicit Obstacle(const QPointF& position);

  void update(float dt) override;
  void draw(QPainter* painter) override;
};

#endif  // OBSTACLE_H_

