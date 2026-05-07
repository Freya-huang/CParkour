#ifndef COIN_H_
#define COIN_H_

#include "Entity.h"

class Coin final : public Entity {
 public:
  explicit Coin(const QPointF& position);

  void update(float dt) override;
  void draw(QPainter* painter) override;
};

#endif  // COIN_H_

