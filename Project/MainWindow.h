#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>

#include <memory>
#include <random>
#include <set>
#include <vector>

#include "Coin.h"
#include "Config.h"
#include "DogItem.h"
#include "Obstacle.h"
#include "Platform.h"

class MainWindow final : public QMainWindow {
  Q_OBJECT

 public:
  enum class GameState { Menu, Playing, GameOver };

  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override = default;

 protected:
  void paintEvent(QPaintEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;

 private slots:
  void on_tick();

 private:
  void update_game(float dt);
  void reset_game();
  void set_state(GameState state);

  // NOLINTNEXTLINE(readability-identifier-naming)
  void check_collisions();
  void update_camera(float dt);
  void spawn_objects_if_needed(int elapsed_ms);
  void recycle_offscreen_objects();

  QTimer* timer_ = nullptr;
  QElapsedTimer frame_timer_;

  std::set<int> pressed_keys_;

  GameState state_ = GameState::Menu;

  DogItem dog_;
  std::vector<Platform> platforms_;

  // 相机（世界坐标）
  qreal camera_x_ = 0.0;

  // 视差背景
  QPixmap scene_sprite_;
  QPixmap menu_sprite_;
  QPixmap heart_sprite_;

  // 动态对象：金币/障碍物（统一管理）
  std::vector<std::unique_ptr<Entity>> objects_;
  int spawn_accumulator_ms_ = 0;
  std::mt19937 rng_;

  int score_ = 0;
  int lives_ = config::kInitialLives;
};

#endif  // MAIN_WINDOW_H_

