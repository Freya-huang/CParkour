#include "Obstacle.h"

#include <QDebug>

namespace {
static constexpr qreal kObstacleWidth = 72.0;
static constexpr qreal kObstacleHeight = 72.0;
}  // namespace

Obstacle::Obstacle(const QPointF& position) : Entity() {
  position_ = position;
  size_ = QSizeF(kObstacleWidth, kObstacleHeight);

  sprite_ = QPixmap(":/images/obstacle.png");
  if (sprite_.isNull()) {
    qWarning() << "Failed to load sprite from resource:"
               << ":/images/obstacle.png"
               << "Please add images/obstacle.png into resources.qrc.";
    Q_ASSERT_X(false, "Obstacle::Obstacle",
               "Sprite load failed: :/images/obstacle.png");
  }
}

void Obstacle::update(float /*dt*/) {
  // 静态障碍物。
}

void Obstacle::draw(QPainter* painter) {
  if (painter == nullptr) {
    return;
  }
  painter->drawPixmap(QRectF(position_, size_), sprite_, sprite_.rect());
}

