#include "Platform.h"

#include <QDebug>

Platform::Platform(const QPointF& position, const QSizeF& size) : Entity() {
  position_ = position;
  size_ = size;

  // V4：地面与背景共用一张图片。
  sprite_ = QPixmap(":/images/scene.png");
  if (sprite_.isNull()) {
    qWarning() << "Failed to load sprite from resource:"
               << ":/images/scene.png"
               << "Please add images/scene.png into resources.qrc.";
    Q_ASSERT_X(false, "Platform::Platform",
               "Sprite load failed: :/images/scene.png");
  }
}

void Platform::update(float /*dt*/) {
  // 静态平台：V2 不需要更新。
}

void Platform::draw(QPainter* painter) {
  if (painter == nullptr) {
    return;
  }

  // V2：平台使用贴图平铺填充区域。
  painter->drawTiledPixmap(QRectF(position_, size_), sprite_);
}

