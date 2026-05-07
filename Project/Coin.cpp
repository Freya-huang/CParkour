#include "Coin.h"

#include <QDebug>

namespace {
static constexpr qreal kCoinSize = 48.0;
}  // namespace

Coin::Coin(const QPointF& position) : Entity() {
  position_ = position;
  size_ = QSizeF(kCoinSize, kCoinSize);

  sprite_ = QPixmap(":/images/coin.png");
  if (sprite_.isNull()) {
    qWarning() << "Failed to load sprite from resource:"
               << ":/images/coin.png"
               << "Please add images/coin.png into resources.qrc.";
    Q_ASSERT_X(false, "Coin::Coin", "Sprite load failed: :/images/coin.png");
  }
}

void Coin::update(float /*dt*/) {
  // V3：金币暂不做动画；后续可加旋转/浮动。
}

void Coin::draw(QPainter* painter) {
  if (painter == nullptr) {
    return;
  }
  painter->drawPixmap(QRectF(position_, size_), sprite_, sprite_.rect());
}

