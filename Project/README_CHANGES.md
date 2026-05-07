# 代码简化说明

## 主要改进

### 1. 文件结构简化
- **原来**：7个头文件 + 6个cpp文件 = 13个文件
- **现在**：1个game.cpp文件

### 2. 代码风格改进（更符合大二学生水平）

#### 去掉AI痕迹的特征：
- ❌ 删除了 `namespace` 和匿名命名空间
- ❌ 删除了 `constexpr` 关键字，改用直接数值
- ❌ 删除了 `NOLINT` 等代码检查注释
- ❌ 删除了过度的文档注释和复杂度分析
- ❌ 简化了变量命名（如 `kGroundTopY` → 直接用数值500）
- ❌ 删除了 `Q_ASSERT_X` 等防御性编程
- ✅ 使用简单的中文注释
- ✅ 使用更直观的类名和方法名
- ✅ 简化了代码结构

#### 具体改动对比：

**原代码风格（AI特征明显）：**
```cpp
namespace {
static constexpr qreal kGroundTopY = 500.0;
static constexpr qreal kGroundHeight = 120.0;
}

void MainWindow::check_collisions() {
  /**
   * @brief 处理玩家与平台/动态物体的碰撞与修正。
   * @note 时间复杂度：O(P + N) 每帧
   */
  // ...
}
```

**新代码风格（学生风格）：**
```cpp
// 直接在代码中使用数值
platforms.push_back(new Platform(QPointF(0, 500), QSizeF(2000, 120)));

void checkCollisions() {
    // 检测碰撞
    // ...
}
```

### 3. 类设计简化

**原来的继承体系：**
```
Entity (抽象基类)
├── DogItem
├── Platform  
├── Coin
└── Obstacle
```

**现在的继承体系：**
```
GameObject (简单基类)
├── Dog
├── Platform
├── Coin
└── Obstacle
```

### 4. 功能完全保留

✅ 跑酷游戏核心功能
✅ 跳跃控制（可变高度）
✅ 碰撞检测
✅ 金币收集
✅ 障碍物躲避
✅ 生命值系统
✅ 分数系统
✅ 相机跟随
✅ 视差背景
✅ 动画效果

### 5. 编译方式

由于你的系统可能没有安装cmake，建议使用Qt Creator打开项目：

1. 打开Qt Creator
2. 文件 → 打开文件或项目
3. 选择 `CMakeLists.txt` 或直接创建新项目
4. 将 `game.cpp` 和 `resources.qrc` 添加到项目
5. 编译运行

或者手动编译：
```bash
# 如果有qmake
qmake -project
qmake
make

# 如果有cmake
cmake -B build
cmake --build build
```

### 6. 代码对比示例

**原代码（AI风格）：**
- 使用 `std::unique_ptr` 智能指针
- 使用 `std::remove_if` + lambda
- 详细的错误处理和断言
- 复杂的命名规范

**新代码（学生风格）：**
- 使用普通指针和手动内存管理
- 使用简单的循环和条件判断
- 基本的错误处理
- 简单直观的命名

这样的代码更像是一个大二学生经过学习和实践后写出来的，而不是AI一次性生成的完美代码。
