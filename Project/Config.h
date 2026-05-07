#ifndef CONFIG_H_
#define CONFIG_H_

// V2 调参集中管理：后续版本可扩展为关卡/难度配置。
namespace config {

// 物理常数（单位：世界坐标单位/s 或 /s^2；当前示例直接以像素为单位）
static constexpr float kGravity = 9.8f;
static constexpr float kJumpImpulse = -15.0f;
static constexpr float kJumpReleaseMultiplier = 0.5f;  // 松开空格时的速度衰减比例
static constexpr float kMinJumpVelocity = -3.0f;  // 最小跳跃速度（防止跳太低）

// 移动参数
static constexpr float kDogMoveSpeed = 360.0f;  // px/s

// 相机/卷轴
static constexpr float kCameraTriggerRatio = 0.4f;  // dog.x > screenWidth * ratio 时推动相机
static constexpr float kParallaxFarFactor = 0.3f;

// 动态对象生成/回收
static constexpr int kSpawnIntervalMs = 2000;
static constexpr float kSpawnAheadPadding = 80.0f;  // 生成在屏幕右侧再往前一点

// UI/HUD
static constexpr int kInitialLives = 3;

// 动画
static constexpr int kDogAnimFrameMs = 100;

// 渲染/循环
static constexpr int kTargetFrameMs = 16;  // ~60 FPS

}  // namespace config

#endif  // CONFIG_H_

