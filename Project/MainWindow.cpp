#include "MainWindow.h"

#include "Config.h"

#include <algorithm>
#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <QFont>

namespace {
static constexpr qreal kGroundTopY = 500.0;
static constexpr qreal kGroundHeight = 120.0;
static constexpr qreal kObstacleHeight = 72.0;
static constexpr qreal kDogStartX = 80.0;
static constexpr qreal kDogStartY = 220.0;
static constexpr qreal kCoinMinY = 220.0;
static constexpr qreal kCoinMaxY = 340.0;
static constexpr int kHudHeartSize = 28;
static constexpr int kHudHeartSpacing = 6;
}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      timer_(new QTimer(this)),
      dog_(QPointF(kDogStartX, kDogStartY)),
      rng_(std::random_device{}()) {
  setWindowTitle(QStringLiteral("线条小狗跑酷 - V4"));
  resize(960, 540);

  setFocusPolicy(Qt::StrongFocus);

  frame_timer_.start();

  scene_sprite_ = QPixmap(":/images/scene.png");
  if (scene_sprite_.isNull()) {
    qWarning() << "Failed to load sprite from resource:"
               << ":/images/scene.png"
               << "Please add images/scene.png into resources.qrc.";
    Q_ASSERT_X(false, "MainWindow::MainWindow",
               "Sprite load failed: :/images/scene.png");
  }

  menu_sprite_ = QPixmap(":/images/menu.png");
  if (menu_sprite_.isNull()) {
    qWarning() << "Failed to load sprite from resource:"
               << ":/images/menu.png"
               << "Please add images/menu.png into resources.qrc.";
    Q_ASSERT_X(false, "MainWindow::MainWindow",
               "Sprite load failed: :/images/menu.png");
  }

  heart_sprite_ = QPixmap(":/images/heart.png");
  if (heart_sprite_.isNull()) {
    qWarning() << "Failed to load sprite from resource:"
               << ":/images/heart.png"
               << "Please add images/heart.png into resources.qrc.";
    Q_ASSERT_X(false, "MainWindow::MainWindow",
               "Sprite load failed: :/images/heart.png");
  }

  // V2：先放一个地面平台（世界坐标）
  platforms_.emplace_back(QPointF(0.0, kGroundTopY), QSizeF(2000.0, kGroundHeight));

  timer_->setInterval(config::kTargetFrameMs);
  connect(timer_, &QTimer::timeout, this, &MainWindow::on_tick);
  timer_->start();
}

void MainWindow::paintEvent(QPaintEvent* /*event*/) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  if (state_ == GameState::Menu) {
    painter.fillRect(rect(), QColor(245, 245, 245));
    painter.drawPixmap(rect(), menu_sprite_);

    painter.setPen(Qt::black);
    painter.setFont(QFont(QStringLiteral("Arial"), 18, QFont::Bold));
    painter.drawText(QRect(0, height() - 90, width(), 60), Qt::AlignCenter,
                     QStringLiteral("按 空格 开始"));
    return;
  }

  // 视差远景：相机速度的 0.3 倍
  painter.fillRect(rect(), QColor(245, 245, 245));
  const qreal far_x = camera_x_ * config::kParallaxFarFactor;
  painter.drawTiledPixmap(rect(), scene_sprite_, QPointF(-far_x, 0.0));

  // 世界坐标 -> 屏幕坐标：drawPos = worldPos - cameraX
  painter.save();
  painter.translate(-camera_x_, 0.0);
  for (Platform& platform : platforms_) {
    platform.draw(&painter);
  }
  for (const std::unique_ptr<Entity>& obj : objects_) {
    obj->draw(&painter);
  }
  dog_.draw(&painter);
  painter.restore();

  // HUD（固定屏幕坐标）
  painter.setPen(Qt::black);
  painter.setFont(QFont(QStringLiteral("Arial"), 14, QFont::Bold));
  painter.drawText(QPointF(16.0, 28.0), QStringLiteral("Score: %1").arg(score_));

  for (int i = 0; i < lives_; ++i) {
    const QRect dst(16 + i * (kHudHeartSize + kHudHeartSpacing), 40,
                    kHudHeartSize, kHudHeartSize);
    painter.drawPixmap(dst, heart_sprite_);
  }

  if (state_ == GameState::GameOver) {
    painter.fillRect(rect(), QColor(0, 0, 0, 120));
    painter.setPen(Qt::white);
    painter.setFont(QFont(QStringLiteral("Arial"), 26, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter,
                     QStringLiteral("Game Over\nScore: %1\n按 R 重开").arg(score_));
  }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  if (event == nullptr) {
    return;
  }

  // 避免 Qt 的 auto-repeat 导致状态抖动；按键状态由 pressed_keys_ 维护。
  if (!event->isAutoRepeat()) {
    pressed_keys_.insert(event->key());
  }

  if (!event->isAutoRepeat() && event->key() == Qt::Key_Space) {
    if (state_ == GameState::Menu) {
      reset_game();
      set_state(GameState::Playing);
    } else if (state_ == GameState::Playing) {
      dog_.jump();
    }
  }

  if (!event->isAutoRepeat() && event->key() == Qt::Key_R) {
    if (state_ == GameState::GameOver) {
      reset_game();
      set_state(GameState::Playing);
    }
  }

  QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
  if (event == nullptr) {
    return;
  }

  if (!event->isAutoRepeat()) {
    pressed_keys_.erase(event->key());
  }

  if (!event->isAutoRepeat() && event->key() == Qt::Key_Space) {
    dog_.on_jump_key_released();
  }

  QMainWindow::keyReleaseEvent(event);
}

void MainWindow::on_tick() {
  const qint64 elapsed_ms = frame_timer_.restart();
  const float dt = static_cast<float>(elapsed_ms) / 1000.0f;

  if (state_ == GameState::Playing) {
    spawn_objects_if_needed(static_cast<int>(elapsed_ms));
    update_game(dt);
    recycle_offscreen_objects();
  }
  repaint();
}

void MainWindow::update_game(float dt) {
  const bool left =
      pressed_keys_.count(Qt::Key_A) > 0 || pressed_keys_.count(Qt::Key_Left) > 0;
  const bool right =
      pressed_keys_.count(Qt::Key_D) > 0 || pressed_keys_.count(Qt::Key_Right) > 0;

  qreal direction = 0.0;
  if (left && !right) {
    direction = -1.0;
  } else if (right && !left) {
    direction = 1.0;
  }

  dog_.translate_x(direction * config::kDogMoveSpeed * dt);
  dog_.update(dt);
  check_collisions();
  update_camera(dt);

  // 【修复代码】让第一块地面始终跟随相机，实现“无限长度”
  // 确保 platforms_ 至少有一个元素
  if (!platforms_.empty()) {
    // 让地面的起点始终在相机左侧一点，长度覆盖两个屏幕宽
    platforms_[0].set_position(QPointF(camera_x_ - 100, kGroundTopY));
    platforms_[0].set_size(QSizeF(width() + 200, kGroundHeight));
  }

  // 分数：按相机推进速度累积（简易版本，后续可替换为金币/关卡计分）
  score_ = std::max(score_, static_cast<int>(camera_x_ / 10.0));
}

void MainWindow::reset_game() {
  pressed_keys_.clear();
  objects_.clear();
  spawn_accumulator_ms_ = 0;
  camera_x_ = 0.0;
  score_ = 0;
  lives_ = config::kInitialLives;

  dog_ = DogItem(QPointF(kDogStartX, kDogStartY));

  frame_timer_.restart();
  if (!timer_->isActive()) {
    timer_->start();
  }
}

void MainWindow::set_state(GameState state) {
  state_ = state;
  if (state_ == GameState::GameOver) {
    timer_->stop();  // 需求：GameOver 停止计时器
  } else {
    if (!timer_->isActive()) {
      timer_->start();
    }
  }
}

void MainWindow::check_collisions() {
  /**
   * @brief 处理玩家与平台/动态物体的碰撞与修正。
   *
   * 平台碰撞使用“上一帧矩形 vs 当前帧矩形”推断碰撞进入方向，用于解决侧边挤压。
   * 动态物体（Coin/Obstacle）使用 AABB（QRectF::intersects）检测。
   *
   * @note 时间复杂度：
   *   - 平台：O(P)（P 为平台数量）
   *   - 动态物体：O(N)（N 为 objects_ 数量）
   *   - 总计：O(P + N) 每帧
   */
  // 默认认为不在地面；若检测到从上方落地则置为 grounded。
  dog_.set_grounded(false);

  const QRectF prev_rect(dog_.previous_position(), dog_.size());
  QRectF dog_rect = dog_.collision_rect();

  for (const Platform& platform : platforms_) {
    const QRectF platform_rect = platform.collision_rect();

    if (!dog_rect.intersects(platform_rect)) {
      continue;
    }

    const bool from_above =
        prev_rect.bottom() <= platform_rect.top() && dog_.velocity_y() >= 0.0f;
    const bool from_below = prev_rect.top() >= platform_rect.bottom();
    const bool from_left = prev_rect.right() <= platform_rect.left();
    const bool from_right = prev_rect.left() >= platform_rect.right();

    if (from_above) {
      // 从上方落下：修正 y，停止下落，置为 grounded
      dog_.set_position(QPointF(dog_.position().x(),
                                platform_rect.top() - dog_.size().height()));
      dog_.set_velocity_y(0.0f);
      dog_.set_grounded(true);
    } else if (from_left) {
      // 撞到平台左侧：阻挡横向移动
      dog_.set_position(QPointF(platform_rect.left() - dog_.size().width(),
                                dog_.position().y()));
    } else if (from_right) {
      // 撞到平台右侧：阻挡横向移动
      dog_.set_position(QPointF(platform_rect.right(), dog_.position().y()));
    } else if (from_below) {
      // 从下方顶到平台：修正 y 并停止上冲
      dog_.set_position(QPointF(dog_.position().x(), platform_rect.bottom()));
      if (dog_.velocity_y() < 0.0f) {
        dog_.set_velocity_y(0.0f);
      }
    } else {
      // 退化处理：优先按 y 方向分离（避免卡死）
      const qreal overlap_down = platform_rect.bottom() - dog_rect.top();
      const qreal overlap_up = dog_rect.bottom() - platform_rect.top();
      if (overlap_up < overlap_down) {
        dog_.set_position(QPointF(dog_.position().x(),
                                  platform_rect.top() - dog_.size().height()));
        dog_.set_velocity_y(0.0f);
        dog_.set_grounded(true);
      } else {
        dog_.set_position(QPointF(dog_.position().x(), platform_rect.bottom()));
        if (dog_.velocity_y() < 0.0f) {
          dog_.set_velocity_y(0.0f);
        }
      }
    }

    dog_rect = dog_.collision_rect();
  }

  // 与动态对象碰撞：金币加分并回收；障碍扣命，归零则 GameOver
  for (const std::unique_ptr<Entity>& obj : objects_) {
    if (!obj->is_alive()) {
      continue;
    }
    if (!dog_.collision_rect().intersects(obj->collision_rect())) {
      continue;
    }

    if (dynamic_cast<Coin*>(obj.get()) != nullptr) {
      score_ += 10;
      obj->kill();
      continue;
    }

    if (dynamic_cast<Obstacle*>(obj.get()) != nullptr) {
      obj->kill();
      lives_ -= 1;
      if (lives_ <= 0) {
        set_state(GameState::GameOver);
        return;
      }
    }
  }
}

void MainWindow::update_camera(float /*dt*/) {
  const qreal screen_width = static_cast<qreal>(width());
  const qreal trigger_x = screen_width * config::kCameraTriggerRatio;

  // 当小狗超过屏幕 40% 位置时，相机开始跟随（只向右推进，不回退）
  const qreal desired_camera_x = dog_.position().x() - trigger_x;
  if (desired_camera_x > camera_x_) {
    camera_x_ = desired_camera_x;
  }
}

void MainWindow::spawn_objects_if_needed(int elapsed_ms) {
  spawn_accumulator_ms_ += elapsed_ms;
  if (spawn_accumulator_ms_ < config::kSpawnIntervalMs) {
    return;
  }
  spawn_accumulator_ms_ -= config::kSpawnIntervalMs;

  const qreal spawn_x =
      camera_x_ + static_cast<qreal>(width()) + config::kSpawnAheadPadding;

  // 基于当前地面平台（y=420）做一个简单的随机高度范围
  // coin: 悬浮；obstacle: 落在地面上
  std::uniform_int_distribution<int> type_dist(0, 1);  // 0=coin, 1=obstacle
  std::uniform_real_distribution<qreal> coin_y_dist(kCoinMinY, kCoinMaxY);

  const int type = type_dist(rng_);
  if (type == 0) {
    objects_.push_back(std::make_unique<Coin>(QPointF(spawn_x, coin_y_dist(rng_))));
  } else {
    // 障碍物底部贴地
    objects_.push_back(std::make_unique<Obstacle>(
        QPointF(spawn_x, kGroundTopY - kObstacleHeight)));
  }
}

void MainWindow::recycle_offscreen_objects() {
  // 安全回收：remove_if + erase，不在遍历绘制/更新过程中删除，避免迭代器失效。
  objects_.erase(
      std::remove_if(objects_.begin(), objects_.end(),
                     [this](const std::unique_ptr<Entity>& obj) {
                       if (!obj->is_alive()) {
                         return true;
                       }
                       const qreal right = obj->position().x() + obj->size().width();
                       return right < camera_x_;
                     }),
      objects_.end());
}

