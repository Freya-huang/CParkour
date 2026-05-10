#include <QApplication>
#include <QMainWindow>
#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include <QElapsedTimer>
#include <QFont>
#include <QDebug>
#include <vector>
#include <set>
#include <random>
#include <algorithm>

// 游戏对象基类
class GameObject {
public:
    QPointF pos;
    QSizeF size;
    QPixmap image;
    bool alive;
    
    GameObject() {
        pos = QPointF(0, 0);
        size = QSizeF(0, 0);
        alive = true;
    }
    
    virtual ~GameObject() {}
    
    QRectF getRect() {
        return QRectF(pos, size);
    }
    
    QRectF getShrinkRect() {
        QRectF rect(pos, size);
        double shrink = 0.1;
        return rect.adjusted(rect.width() * shrink / 2, rect.height() * shrink / 2,
                           -rect.width() * shrink / 2, -rect.height() * shrink / 2);
    }
    
    virtual void update(float dt) {}
    
    virtual void draw(QPainter* painter) {
        if (painter) {
            painter->drawPixmap(QRectF(pos, size), image, image.rect());
        }
    }
};

// 平台类型
enum PlatformType {
    GROUND,      // 地面
    BLOCK,       // 普通方块（可以站立）
    QUESTION     // 问号方块（可以顶）
};

// 平台
class Platform : public GameObject {
public:
    PlatformType type;
    bool hasItem;  
    int bounceTimer; 
    QPixmap questionBlockImg; 
    QPixmap questionBlockUsedImg;  
    static QPixmap* groundImage;  
    
    Platform(double x, double y, double w, double h, PlatformType t = GROUND) {
        pos = QPointF(x, y);
        size = QSizeF(w, h);
        type = t;
        hasItem = true;
        bounceTimer = 0;
        
        if (type == GROUND) {
            if (groundImage && !groundImage->isNull()) {
                image = *groundImage;
            } else {
                image = QPixmap(":/images/scene.png");
            }
        } else if (type == QUESTION) {
            questionBlockImg = QPixmap(":/images/question_block.png");
            questionBlockUsedImg = QPixmap(":/images/question_block_used.png");
            
            image = questionBlockImg;
        } else if (type == BLOCK) {
            image = QPixmap(":/images/block.png");

        }
    }
    
    void update(float dt) override {
        if (bounceTimer > 0) {
            bounceTimer -= (int)(dt * 1000);
            if (bounceTimer < 0) bounceTimer = 0;
        }
    }
    
    void hit() {
        // 被从下方顶到
        if (type == QUESTION && hasItem) {
            hasItem = false;
            bounceTimer = 200;  // 弹跳200毫秒
            // 切换到使用后的图片
            image = questionBlockUsedImg;
        } else if (type == BLOCK) {
            bounceTimer = 100;
        }
    }
    
    void draw(QPainter* painter) override {
        if (painter) {
            QRectF rect(pos, size);
            // 弹跳效果
            if (bounceTimer > 0) {
                rect.translate(0, -10);
            }
            
            if (type == GROUND) {
                painter->drawTiledPixmap(rect, image);
            } else {
                painter->drawPixmap(rect, image, image.rect());
                
            }
        }
    }
};

// 初始化静态成员
QPixmap* Platform::groundImage = nullptr;

// 金币
class Coin : public GameObject {
public:
    bool isFromBlock;  
    float velocityY;   
    float gravity;     
    int lifetime;      
    int maxLifetime;   
    
    Coin(double x, double y, bool fromBlock = false) {
        pos = QPointF(x, y);
        size = QSizeF(48, 48);
        image = QPixmap(":/images/coin.png");
        isFromBlock = fromBlock;
        
        if (isFromBlock) {
            // 从方块弹出的金币
            velocityY = -600.0f;  // 向上弹出的初速度
            gravity = 1800.0f;    // 重力加速度
            lifetime = 0;
            maxLifetime = 800;    // 800毫秒后消失
        } else {
            // 场景中的静态金币
            velocityY = 0;
            gravity = 0;
            lifetime = -1;  // 永久存在
            maxLifetime = -1;
        }
    }
    
    void update(float dt) override {
        if (isFromBlock) {
            // 应用重力和速度
            velocityY += gravity * dt;
            pos.setY(pos.y() + velocityY * dt);
            
            // 更新生命周期
            lifetime += (int)(dt * 1000);
            if (lifetime >= maxLifetime) {
                alive = false;
            }
        }
    }
    
    void draw(QPainter* painter) override {
        if (painter) {
            // 弹出的金币有淡出效果
            if (isFromBlock && lifetime > maxLifetime * 0.6f) {
                float alpha = 1.0f - (lifetime - maxLifetime * 0.6f) / (maxLifetime * 0.4f);
                painter->setOpacity(alpha);
            }
            
            painter->drawPixmap(QRectF(pos, size), image, image.rect());
            
            // 恢复透明度
            if (isFromBlock) {
                painter->setOpacity(1.0);
            }
        }
    }
};

// 障碍物
class Obstacle : public GameObject {
public:
    Obstacle(double x, double y) {
        pos = QPointF(x, y);
        size = QSizeF(64, 48);  
        image = QPixmap(":/images/obstacle.png");
    }
};

// 小狗
class Dog : public GameObject {
public:
    QPointF lastPos;
    float speedY;
    float speedX;  
    bool onGround;
    bool wasOnGround;  
    int coyoteTime;  
    bool jumpPressed;  
    QPixmap img1, img2;
    int animTimer;
    bool showImg1;
    
    // 马里奥式跳跃参数
    const float JUMP_IMPULSE = -850.0f;  // 跳跃初速度
    const float JUMP_CUT_MULTIPLIER = 0.4f;  // 松开跳跃键时的速度衰减
    const float MIN_JUMP_VELOCITY = -200.0f;  // 最小跳跃速度（保证最低高度）
    const float GRAVITY_RISING = 1800.0f;  // 上升时的重力
    const float GRAVITY_FALLING = 2800.0f;  // 下落时的重力
    const float MAX_FALL_SPEED = 1200.0f;  // 最大下落速度
    const float GROUND_ACCEL = 2000.0f;  // 地面加速度
    const float AIR_ACCEL = 1200.0f;  // 空中加速度
    const float MAX_SPEED_WALK = 300.0f;  // 行走最大速度
    const float MAX_SPEED_RUN = 500.0f;  // 奔跑最大速度
    const float FRICTION = 0.85f;  // 摩擦力
    const int COYOTE_TIME_MS = 120;  // 土狼时间（毫秒）
    const float STOMP_BOUNCE_SMALL = -400.0f;  // 踩踏小反弹
    const float STOMP_BOUNCE_BIG = -700.0f;  // 踩踏大反弹（按住跳跃键）
    
    Dog(double x, double y) {
        pos = QPointF(x, y);
        lastPos = pos;
        size = QSizeF(96, 96);
        speedY = 0;
        speedX = 0;
        onGround = true;
        wasOnGround = true;
        coyoteTime = 0;
        jumpPressed = false;
        animTimer = 0;
        showImg1 = true;
        
        img1 = QPixmap(":/images/dog_run1.png");
        img2 = QPixmap(":/images/dog_run2.png");
        image = img1;
        
        if (img2.isNull()) {
            img2 = img1;
        }
    }
    
    void jump() {
        // 土狼时间：刚离开地面一小段时间内仍可跳跃
        if (onGround || coyoteTime > 0) {
            speedY = JUMP_IMPULSE;
            onGround = false;
            coyoteTime = 0;
            jumpPressed = true;
        }
    }
    
    void stopJump() {
        jumpPressed = false;
        // 松开跳跃键时，如果还在上升，立即减速（实现变长跳跃）
        if (speedY < MIN_JUMP_VELOCITY) {
            speedY *= JUMP_CUT_MULTIPLIER;
            // 确保不低于最小跳跃速度
            if (speedY > MIN_JUMP_VELOCITY) {
                speedY = MIN_JUMP_VELOCITY;
            }
        }
    }
    
    void stompBounce() {
        // 踩踏敌人后的反弹
        if (jumpPressed) {
            // 如果按住跳跃键，大反弹
            speedY = STOMP_BOUNCE_BIG;
        } else {
            // 否则小反弹
            speedY = STOMP_BOUNCE_SMALL;
        }
        onGround = false;
    }
    
    void move(float dt, float dirX, bool running) {
        // 助跑系统：根据是否按住奔跑键决定最大速度
        float maxSpeed = running ? MAX_SPEED_RUN : MAX_SPEED_WALK;
        float accel = onGround ? GROUND_ACCEL : AIR_ACCEL;
        
        // 应用加速度
        if (dirX != 0) {
            speedX += dirX * accel * dt;
            // 限制最大速度
            if (speedX > maxSpeed) speedX = maxSpeed;
            if (speedX < -maxSpeed) speedX = -maxSpeed;
        } else {
            // 没有输入时应用摩擦力
            if (onGround) {
                speedX *= FRICTION;
                if (std::abs(speedX) < 10.0f) speedX = 0;
            }
        }
        
        // 应用水平移动
        pos.setX(pos.x() + speedX * dt);
    }
    
    void update(float dt) override {
        lastPos = pos;
        wasOnGround = onGround;
        
        // 非对称重力：上升时轻，下落时重
        float gravity = (speedY < 0) ? GRAVITY_RISING : GRAVITY_FALLING;
        
        // 应用重力
        speedY += gravity * dt;
        
        // 限制最大下落速度
        if (speedY > MAX_FALL_SPEED) {
            speedY = MAX_FALL_SPEED;
        }
        
        // 应用垂直移动
        pos.setY(pos.y() + speedY * dt);
        
        // 土狼时间：刚离开地面后的短暂缓冲
        if (wasOnGround && !onGround) {
            coyoteTime = COYOTE_TIME_MS;
        }
        if (coyoteTime > 0) {
            coyoteTime -= (int)(dt * 1000);
            if (coyoteTime < 0) coyoteTime = 0;
        }
        
        // 动画（只在有水平移动时播放）
        if (std::abs(speedX) > 10.0f) {  // 只有当速度大于10时才播放动画
            int animSpeed = 100;
            if (std::abs(speedX) > MAX_SPEED_WALK) {
                animSpeed = 60;  // 奔跑时动画更快
            }
            animTimer += (int)(dt * 1000);
            if (animTimer >= animSpeed) {
                animTimer = 0;
                showImg1 = !showImg1;
            }
        } else {
            // 静止时保持第一帧
            showImg1 = true;
        }
        image = showImg1 ? img1 : img2;
    }
};

// 毛线筐（触发推箱子关卡的道具）
class Basket : public GameObject {
public:
    bool triggered;  // 是否已触发
    int animTimer;   // 动画计时器
    
    Basket(double x, double y) {
        pos = QPointF(x, y);
        size = QSizeF(80, 80);
        image = QPixmap(":/images/basket.png");
        triggered = false;
        animTimer = 0;
        
    }
    
    void draw(QPainter* painter) override {
        if (painter && !triggered) {
            // 轻微浮动动画
            QRectF rect(pos, size);
            double offset = std::sin(animTimer / 200.0) * 5;
            rect.translate(0, offset);
            painter->drawPixmap(rect, image, image.rect());
            
            // 绘制提示文字
            painter->setPen(QColor(255, 200, 0));
            painter->setFont(QFont("Arial", 12, QFont::Bold));
            painter->drawText(QRectF(pos.x() - 20, pos.y() - 30, 120, 20), 
                            Qt::AlignCenter, "解谜关卡");
        }
    }
    
    void update(float dt) override {
        animTimer += (int)(dt * 1000);
    }
};

// 推箱子管理器
class SokobanManager {
public:
    // 关卡数据
    std::vector<std::string> levels;
    int currentLevel;
    
    // 网格数据
    std::vector<std::vector<char>> grid;
    int gridWidth, gridHeight;
    int cellSize;  // 每个格子的像素大小
    
    // 玩家位置（网格坐标）
    int playerX, playerY;
    
    // 目标点和箱子位置
    std::vector<QPoint> targets;
    std::vector<QPoint> boxes;
    
    // 动画相关
    bool isMoving;
    QPointF playerAnimPos;  // 玩家动画位置（像素坐标）
    QPointF movingBoxStart;  // 移动中的箱子起始位置
    QPointF movingBoxEnd;    // 移动中的箱子结束位置
    int movingBoxIndex;      // 正在移动的箱子索引
    float moveProgress;      // 移动进度 0-1
    
    // 图片资源
    QPixmap wallImg, targetImg, boxImg, boxOnTargetImg, dogSokobanImg;
    
    SokobanManager() {
        currentLevel = -1;
        isMoving = false;
        moveProgress = 0;
        movingBoxIndex = -1;
        cellSize = 64;
        
        // 初始化关卡数据
        levels = {
            // Level 1 (1000分触发 - 双球入门)
            "###########\n"
            "#         #\n"
            "#      .B #\n"
            "# P    .B #\n"
            "#      #  #\n"
            "###########",
            
            // Level 2 (2000分触发 - 垂直长廊挑战)
            "#######\n"
            "#  .  #\n"
            "#     #\n"
            "#   B #\n"
            "###   #\n"
            "#P    #\n"
            "#   B #\n"
            "#   B #\n"
            "#    .#\n"
            "#     #\n"
            "# .   #\n"
            "#######",
            
            // Level 3 (3000分触发 - 复杂迷宫)
            "##########\n"
            "#    .   #\n"
            "#  B   B #\n"
            "#  # #   #\n"
            "#  . P . #\n"
            "#  # #   #\n"
            "#  B   B #\n"
            "#    .   #\n"
            "##########"
        };
        
        // 加载图片资源
        loadImages();
    }
    
    void loadImages() {
        // 墙壁
        wallImg = QPixmap(":/images/wall.png");
        if (wallImg.isNull()) {
            wallImg = QPixmap(cellSize, cellSize);
            wallImg.fill(QColor(80, 80, 80));
        }
        
        // 目标点
        targetImg = QPixmap(":/images/target.png");
        if (targetImg.isNull()) {
            targetImg = QPixmap(cellSize, cellSize);
            targetImg.fill(Qt::transparent);
        }
        
        // 毛线球（箱子）
        boxImg = QPixmap(":/images/yarn_ball.png");
        if (boxImg.isNull()) {
            boxImg = QPixmap(cellSize, cellSize);
            boxImg.fill(QColor(255, 100, 150));  // 粉色毛线球
        }
        
        // 毛线球在目标上
        boxOnTargetImg = QPixmap(":/images/yarn_ball_on_target.png");
        if (boxOnTargetImg.isNull()) {
            boxOnTargetImg = QPixmap(cellSize, cellSize);
            boxOnTargetImg.fill(QColor(100, 255, 100));  // 绿色表示正确
        }
        
        // 小狗（推箱子模式）
        dogSokobanImg = QPixmap(":/images/dog_run1.png");
    }
    
    void loadLevel(int levelIndex) {
        if (levelIndex < 0 || levelIndex >= (int)levels.size()) return;
        
        currentLevel = levelIndex;
        grid.clear();
        targets.clear();
        boxes.clear();
        
        // 解析关卡字符串
        std::string levelData = levels[levelIndex];
        std::vector<char> row;
        gridHeight = 0;
        gridWidth = 0;
        
        int x = 0, y = 0;
        for (char c : levelData) {
            if (c == '\n') {
                if (!row.empty()) {
                    grid.push_back(row);
                    gridWidth = std::max(gridWidth, (int)row.size());
                    row.clear();
                    y++;
                    x = 0;
                }
            } else {
                // 处理特殊符号
                if (c == 'P') {
                    playerX = x;
                    playerY = y;
                    row.push_back(' ');  // 玩家位置变为空地
                } else if (c == 'B') {
                    boxes.push_back(QPoint(x, y));
                    row.push_back(' ');  // 箱子位置变为空地
                } else if (c == '.') {
                    targets.push_back(QPoint(x, y));
                    row.push_back('.');  // 保留目标点标记
                } else if (c == '*') {
                    // 箱子已经在目标上
                    boxes.push_back(QPoint(x, y));
                    targets.push_back(QPoint(x, y));
                    row.push_back('.');
                } else {
                    row.push_back(c);
                }
                x++;
            }
        }
        
        if (!row.empty()) {
            grid.push_back(row);
            gridWidth = std::max(gridWidth, (int)row.size());
            gridHeight = (int)grid.size();
        } else {
            gridHeight = (int)grid.size();
        }
        
        // 补齐每行长度
        for (auto& r : grid) {
            while ((int)r.size() < gridWidth) {
                r.push_back(' ');
            }
        }
        
        // 初始化动画位置
        playerAnimPos = QPointF(playerX * cellSize, playerY * cellSize);
        isMoving = false;
    }
    
    bool tryMove(int dx, int dy) {
        if (isMoving) return false;  // 动画中不能移动
        
        int newX = playerX + dx;
        int newY = playerY + dy;
        
        // 边界检查
        if (newX < 0 || newX >= gridWidth || newY < 0 || newY >= gridHeight) {
            return false;
        }
        
        // 墙壁检查
        if (grid[newY][newX] == '#') {
            return false;
        }
        
        // 检查是否有箱子
        int boxIndex = -1;
        for (int i = 0; i < (int)boxes.size(); i++) {
            if (boxes[i].x() == newX && boxes[i].y() == newY) {
                boxIndex = i;
                break;
            }
        }
        
        if (boxIndex != -1) {
            // 尝试推箱子
            int boxNewX = newX + dx;
            int boxNewY = newY + dy;
            
            // 箱子新位置边界检查
            if (boxNewX < 0 || boxNewX >= gridWidth || boxNewY < 0 || boxNewY >= gridHeight) {
                return false;
            }
            
            // 箱子新位置墙壁检查
            if (grid[boxNewY][boxNewX] == '#') {
                return false;
            }
            
            // 箱子新位置是否有其他箱子
            for (int i = 0; i < (int)boxes.size(); i++) {
                if (i != boxIndex && boxes[i].x() == boxNewX && boxes[i].y() == boxNewY) {
                    return false;
                }
            }
            
            // 可以推箱子
            movingBoxStart = QPointF(boxes[boxIndex].x() * cellSize, boxes[boxIndex].y() * cellSize);
            boxes[boxIndex] = QPoint(boxNewX, boxNewY);
            movingBoxEnd = QPointF(boxNewX * cellSize, boxNewY * cellSize);
            movingBoxIndex = boxIndex;
        }
        
        // 移动玩家
        playerX = newX;
        playerY = newY;
        isMoving = true;
        moveProgress = 0;
        
        return true;
    }
    
    void update(float dt) {
        if (isMoving) {
            moveProgress += dt * 8.0f;  // 移动速度
            if (moveProgress >= 1.0f) {
                moveProgress = 1.0f;
                isMoving = false;
                movingBoxIndex = -1;
            }
            
            // 更新玩家动画位置（平滑插值）
            float t = moveProgress;
            playerAnimPos = QPointF(
                playerX * cellSize * t + playerAnimPos.x() * (1 - t),
                playerY * cellSize * t + playerAnimPos.y() * (1 - t)
            );
        } else {
            playerAnimPos = QPointF(playerX * cellSize, playerY * cellSize);
        }
    }
    
    bool isCompleted() {
        // 检查所有箱子是否都在目标点上
        for (const QPoint& box : boxes) {
            bool onTarget = false;
            for (const QPoint& target : targets) {
                if (box == target) {
                    onTarget = true;
                    break;
                }
            }
            if (!onTarget) return false;
        }
        return true;
    }
    
    void draw(QPainter* painter, int screenWidth, int screenHeight) {
        if (currentLevel < 0) return;
        
        // 计算地图总尺寸
        int totalWidth = gridWidth * cellSize;
        int totalHeight = gridHeight * cellSize;
        
        // 计算缩放比例（如果地图超出屏幕）
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        
        // 留出上下各50像素的空间给UI
        int availableWidth = screenWidth - 40;  // 左右各留20像素
        int availableHeight = screenHeight - 100;  // 上下各留50像素
        
        if (totalWidth > availableWidth) {
            scaleX = (float)availableWidth / totalWidth;
        }
        if (totalHeight > availableHeight) {
            scaleY = (float)availableHeight / totalHeight;
        }
        
        // 使用较小的缩放比例，保持宽高比
        float scale = std::min(scaleX, scaleY);
        
        // 应用缩放后的尺寸
        int scaledWidth = (int)(totalWidth * scale);
        int scaledHeight = (int)(totalHeight * scale);
        
        // 计算居中偏移
        int offsetX = (screenWidth - scaledWidth) / 2;
        int offsetY = (screenHeight - scaledHeight) / 2;
        
        painter->save();
        painter->translate(offsetX, offsetY);
        
        // 应用缩放
        if (scale < 1.0f) {
            painter->scale(scale, scale);
        }
        
        // 绘制背景
        painter->fillRect(0, 0, totalWidth, totalHeight, QColor(240, 230, 220));
        
        // 绘制网格
        for (int y = 0; y < gridHeight; y++) {
            for (int x = 0; x < gridWidth; x++) {
                QRectF cellRect(x * cellSize, y * cellSize, cellSize, cellSize);
                
                char cell = grid[y][x];
                if (cell == '#') {
                    // 墙壁
                    painter->drawPixmap(cellRect, wallImg, wallImg.rect());
                } else if (cell == '.') {
                    // 目标点
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(QColor(255, 200, 100, 100));
                    painter->drawEllipse(cellRect.adjusted(cellSize * 0.2, cellSize * 0.2, 
                                                          -cellSize * 0.2, -cellSize * 0.2));
                }
            }
        }
        
        // 绘制箱子
        for (int i = 0; i < (int)boxes.size(); i++) {
            QPointF boxPos;
            
            // 如果是正在移动的箱子，使用插值位置
            if (isMoving && i == movingBoxIndex) {
                float t = moveProgress;
                boxPos = QPointF(
                    movingBoxEnd.x() * t + movingBoxStart.x() * (1 - t),
                    movingBoxEnd.y() * t + movingBoxStart.y() * (1 - t)
                );
            } else {
                boxPos = QPointF(boxes[i].x() * cellSize, boxes[i].y() * cellSize);
            }
            
            QRectF boxRect(boxPos, QSizeF(cellSize, cellSize));
            
            // 检查箱子是否在目标上
            bool onTarget = false;
            for (const QPoint& target : targets) {
                if (boxes[i] == target) {
                    onTarget = true;
                    break;
                }
            }
            
            if (onTarget) {
                painter->drawPixmap(boxRect, boxOnTargetImg, boxOnTargetImg.rect());
            } else {
                painter->drawPixmap(boxRect, boxImg, boxImg.rect());
            }
        }
        
        // 绘制玩家
        QRectF playerRect(playerAnimPos, QSizeF(cellSize, cellSize));
        painter->drawPixmap(playerRect, dogSokobanImg, dogSokobanImg.rect());
        
        painter->restore();
        
        // 绘制UI提示
        painter->setPen(Qt::black);
        painter->setFont(QFont("Arial", 14, QFont::Bold));
        painter->drawText(QRect(0, 10, screenWidth, 30), Qt::AlignCenter,
                         QString("推箱子关卡 %1/3 - 方向键移动 - ESC退出").arg(currentLevel + 1));
        
        // 绘制进度
        int boxesOnTarget = 0;
        for (const QPoint& box : boxes) {
            for (const QPoint& target : targets) {
                if (box == target) {
                    boxesOnTarget++;
                    break;
                }
            }
        }
        painter->drawText(QRect(0, screenHeight - 40, screenWidth, 30), Qt::AlignCenter,
                         QString("进度: %1/%2").arg(boxesOnTarget).arg(boxes.size()));
    }
};

// 游戏主窗口
class Game : public QMainWindow {
public:
    // 游戏状态
    int state; // 0=菜单 1=游戏中 2=游戏结束 3=推箱子模式 4=推箱子完成动画
    
    // 定时器
    QTimer* timer;
    QElapsedTimer frameTime;
    
    // 按键
    std::set<int> keys;
    
    // 游戏对象
    Dog* dog;
    std::vector<Platform*> platforms;
    std::vector<GameObject*> objects;
    
    // 推箱子系统
    SokobanManager* sokobanManager;
    Basket* currentBasket;  // 当前的毛线筐
    int nextSokobanScore;   // 下一个触发推箱子的分数
    int sokobanLevel;       // 当前推箱子关卡索引
    
    // 转场动画
    bool isTransitioning;   // 是否在转场中
    float transitionProgress;  // 转场进度 0-1
    int transitionType;     // 0=进入推箱子 1=退出推箱子
    QPointF dogTransitionPos;  // 小狗转场动画位置
    float dogTransitionScale;  // 小狗转场缩放
    int completionTimer;    // 完成关卡后的延迟计时器
    bool sokobanCompleted;  // 是否完成了推箱子关卡（而非ESC退出）
    
    // 相机
    double cameraX;
    
    // 图片
    QPixmap bgImg, groundImg, menuImg, heartImg;
    
    // 区块生成
    int nextChunkX;  // 下一个区块的X坐标
    int chunkWidth;  // 每个区块的宽度
    
    // 生成计时
    int spawnTime;
    std::mt19937 rng;
    
    // 分数和生命
    int score;
    int lives;
    
    Game() {
        setWindowTitle("小狗跑酷");
        resize(960, 540);
        setFocusPolicy(Qt::StrongFocus);
        
        // 初始化定时器
        timer = new QTimer(this);
        timer->setInterval(16);
        connect(timer, &QTimer::timeout, this, &Game::tick);
        
        // 加载图片
        menuImg = QPixmap(":/images/menu.png");
        heartImg = QPixmap(":/images/heart.png");
        
        // 背景使用scene.png
        bgImg = QPixmap(":/images/scene.png");
        
        // 尝试加载地面图片，如果不存在则使用scene.png
        groundImg = QPixmap(":/images/ground.png");
        if (groundImg.isNull()) {
            groundImg = QPixmap(":/images/scene.png");
        }
        
        // 确保地面图片加载成功
        if (groundImg.isNull()) {
            qDebug() << "警告：地面图片加载失败！";
        } else {
            qDebug() << "地面图片加载成功，尺寸：" << groundImg.size();
        }
        
        // 设置Platform类的静态地面图片
        Platform::groundImage = &groundImg;
        
        // 初始化随机数
        rng.seed(std::random_device{}());
        
        // 初始化区块系统
        chunkWidth = 1000;
        nextChunkX = 0;
        
        // 创建初始关卡
        createLevel();
        
        // 初始化推箱子系统
        sokobanManager = new SokobanManager();
        currentBasket = nullptr;
        nextSokobanScore = 1000;
        sokobanLevel = 0;
        isTransitioning = false;
        transitionProgress = 0;
        transitionType = 0;
        completionTimer = 0;
        sokobanCompleted = false;
        
        // 初始化游戏
        state = 0;
        dog = new Dog(80, 404);  // 地面高度500 - 小狗高度96 = 404
        cameraX = 0;
        score = 0;
        lives = 3;
        spawnTime = 0;
        
        frameTime.start();
        timer->start();
    }
    
    ~Game() {
        delete dog;
        delete sokobanManager;
        if (currentBasket) delete currentBasket;
        for (auto p : platforms) delete p;
        for (auto obj : objects) delete obj;
    }
    
    void paintEvent(QPaintEvent* e) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        // 推箱子模式
        if (state == 3) {
            p.fillRect(rect(), QColor(200, 200, 200));
            sokobanManager->draw(&p, width(), height());
            
            // 淡入效果
            if (isTransitioning && transitionType == 0) {
                float alpha = 1.0f - transitionProgress;
                p.fillRect(rect(), QColor(0, 0, 0, (int)(alpha * 255)));
            }
            return;
        }
        
        // 推箱子完成动画
        if (state == 4) {
            p.fillRect(rect(), QColor(200, 200, 200));
            sokobanManager->draw(&p, width(), height());
            
            // 胜利提示
            p.fillRect(rect(), QColor(0, 0, 0, 150));
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 24, QFont::Bold));
            p.drawText(rect(), Qt::AlignCenter, "关卡完成！\n\n返回跑酷模式...");
            
            // 淡出效果
            if (isTransitioning && transitionType == 1) {
                p.fillRect(rect(), QColor(0, 0, 0, (int)(transitionProgress * 255)));
            }
            return;
        }
        
        // 菜单
        if (state == 0) {
            p.setRenderHint(QPainter::Antialiasing); 
            p.fillRect(rect(), QColor(245, 245, 245));
            p.drawPixmap(rect(), menuImg);

            auto drawStyledButton = [&](const QString &text, int x, int y, int h, QColor color, QFont font) {
        
                p.setFont(font);
                QFontMetrics fm = p.fontMetrics();
                int w = fm.horizontalAdvance(text) + (h == 60 ? 40 : 20); 

                p.setPen(Qt::NoPen);
                p.setBrush(color.darker(150));
                p.drawRoundedRect(x, y + 4, w, h, 10, 10);

                p.setBrush(color);
                p.setPen(QPen(QColor("#5D4037"), 2)); 
                QRect btnRect(x, y, w, h);
                p.drawRoundedRect(btnRect, 10, 10);

                p.setPen(QColor("#3E2723")); 
                p.drawText(btnRect, Qt::AlignCenter, text);

                return w; 
            };

            QString mainText = "SPACE 开始游戏";
            QFont mainFont("Arial", 22, QFont::Bold);
            QFontMetrics mainFm(mainFont);
    
            int mainW = mainFm.horizontalAdvance(mainText) + 40;
            int mainX = (width() - mainW) / 2; 
            drawStyledButton(mainText, mainX, height() - 150, 60, QColor("#F3C367"), mainFont); // 使用你喜欢的暖黄色

            int centerY = height() - 80;
            int spacing = 15;
            p.setFont(QFont("Arial", 11, QFont::Bold));

            auto drawKeyRect = [&](const QString &key, const QString &desc, int &currentX) {
                QFontMetrics fm = p.fontMetrics();
                int keyWidth = fm.horizontalAdvance(key) + 20;
                int descWidth = fm.horizontalAdvance(desc) + 5;
        
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(200, 200, 200)); 
                p.drawRoundedRect(currentX, centerY + 3, keyWidth, 30, 8, 8);

                p.setBrush(QColor("#FDF5E6")); 
                p.setPen(QPen(QColor("#8B4513"), 1.5)); 
                QRect keyRect(currentX, centerY, keyWidth, 30);
                p.drawRoundedRect(keyRect, 8, 8);

                p.setPen(Qt::black);
                p.drawText(keyRect, Qt::AlignCenter, key);

                currentX += keyWidth + 8;
                p.setPen(QColor(60, 60, 60));
                p.drawText(currentX, centerY + 20, desc);
        
                currentX += descWidth + spacing;
            };

            int currentX = width() / 2 - 120; 
            drawKeyRect("Space", "跳跃", currentX);
            drawKeyRect("Shift", "加速", currentX);
            drawKeyRect("A/D", "移动", currentX);

            return;
        }
        
        // 游戏画面
        p.fillRect(rect(), QColor(245, 245, 245));
        
        // 背景
        double bgX = cameraX * 0.3;
        p.drawTiledPixmap(rect(), bgImg, QPointF(-bgX, 0));
        
        // 游戏对象
        p.save();
        p.translate(-cameraX, 0);
        
        for (auto plat : platforms) {
            plat->draw(&p);
        }
        for (auto obj : objects) {
            obj->draw(&p);
        }
        
        // 绘制毛线筐
        if (currentBasket && currentBasket->alive) {
            currentBasket->draw(&p);
        }
        
        dog->draw(&p);
        
        // 转场动画：小狗跳入筐中
        if (isTransitioning && transitionType == 0 && state == 1) {
            // 绘制小狗缩小并下落的动画
            p.save();
            QPointF center = dogTransitionPos + QPointF(dog->size.width() / 2, dog->size.height() / 2);
            p.translate(center);
            p.scale(dogTransitionScale, dogTransitionScale);
            p.translate(-center);
            p.setOpacity(1.0f - transitionProgress * 0.5f);
            dog->draw(&p);
            p.restore();
        }
        
        p.restore();
        
        // UI
        p.setPen(Qt::black);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(QPointF(16, 28), QString("分数: %1").arg(score));
        
        // 生命值
        for (int i = 0; i < lives; i++) {
            p.drawPixmap(QRect(16 + i * 34, 40, 28, 28), heartImg);
        }
        
        // 显示速度指示器（助跑状态）
        if (state == 1) {
            float speedPercent = std::abs(dog->speedX) / dog->MAX_SPEED_RUN;
            if (speedPercent > 0.1f) {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(100, 200, 255, 180));
                int barWidth = (int)(speedPercent * 150);
                p.drawRect(16, 75, barWidth, 8);
                p.setBrush(QColor(255, 255, 255, 100));
                p.drawRect(16 + barWidth, 75, 150 - barWidth, 8);
                
                p.setPen(Qt::black);
                p.setFont(QFont("Arial", 10));
                p.drawText(QPointF(16, 95), speedPercent >= 0.95f ? "全速冲刺！" : "速度");
            }
        }
        
        // 游戏结束
        if (state == 2) {
            p.fillRect(rect(), QColor(0, 0, 0, 120));
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 26, QFont::Bold));
            p.drawText(rect(), Qt::AlignCenter, 
                      QString("游戏结束\n分数: %1\n\n按R重新开始").arg(score));
            
            // 显示操作提示
            p.setFont(QFont("Arial", 12));
            p.drawText(QRect(0, height() - 100, width(), 80), Qt::AlignCenter,
                      "操作提示：\n空格=跳跃（长按跳更高） | Shift=奔跑加速 | A/D或方向键=移动");
        }
    }
    
    void keyPressEvent(QKeyEvent* e) override {
        if (!e->isAutoRepeat()) {
            keys.insert(e->key());
        }
        
        // 推箱子模式的按键处理
        if (state == 3) {
            if (e->key() == Qt::Key_Escape) {
                // ESC退出推箱子模式
                sokobanCompleted = false;  // 标记为未完成（ESC退出）
                state = 4;  // 先切换到完成动画状态，避免竞态条件
                completionTimer = 2000;  // 立即触发返回（跳过2秒等待）
                startTransitionToRunner();
                return;
            }
            
            if (!e->isAutoRepeat()) {
                if (e->key() == Qt::Key_Up || e->key() == Qt::Key_W) {
                    sokobanManager->tryMove(0, -1);
                } else if (e->key() == Qt::Key_Down || e->key() == Qt::Key_S) {
                    sokobanManager->tryMove(0, 1);
                } else if (e->key() == Qt::Key_Left || e->key() == Qt::Key_A) {
                    sokobanManager->tryMove(-1, 0);
                } else if (e->key() == Qt::Key_Right || e->key() == Qt::Key_D) {
                    sokobanManager->tryMove(1, 0);
                }
            }
            return;
        }
        
        if (!e->isAutoRepeat() && e->key() == Qt::Key_Space) {
            if (state == 0) {
                restart();
                state = 1;
            } else if (state == 1) {
                dog->jump();
            }
        }
        
        if (!e->isAutoRepeat() && e->key() == Qt::Key_R) {
            if (state == 2) {
                restart();
                state = 1;
            }
        }
    }
    
    void keyReleaseEvent(QKeyEvent* e) override {
        if (!e->isAutoRepeat()) {
            keys.erase(e->key());
        }
        
        if (!e->isAutoRepeat() && e->key() == Qt::Key_Space) {
            dog->stopJump();
        }
    }
    
    void tick() {
        int elapsed = frameTime.restart();
        float dt = elapsed / 1000.0f;
        
        if (state == 1) {
            spawn(elapsed);
            gameUpdate(dt);
            cleanup();
        } else if (state == 3) {
            // 推箱子模式更新
            sokobanManager->update(dt);
            
            // 检查是否完成
            if (sokobanManager->isCompleted() && !isTransitioning) {
                state = 4;
                completionTimer = 0;
                sokobanCompleted = true;  // 标记为完成关卡
            }
        } else if (state == 4) {
            // 完成动画
            completionTimer += elapsed;
            if (completionTimer >= 2000 && !isTransitioning) {  // 2秒后返回，且未在转场中
                startTransitionToRunner();
            }
        }
        
        // 转场动画更新（在所有状态下都执行）
        if (isTransitioning) {
            updateTransition(dt);
        }
        
        repaint();
    }
    
    void gameUpdate(float dt) {
        // 马里奥风格的移动控制
        bool left = keys.count(Qt::Key_A) || keys.count(Qt::Key_Left);
        bool right = keys.count(Qt::Key_D) || keys.count(Qt::Key_Right);
        bool running = keys.count(Qt::Key_Shift);  // Shift键奔跑
        
        float dirX = 0;
        if (left && !right) dirX = -1;
        else if (right && !left) dirX = 1;
        
        // 使用新的移动系统（包含助跑）
        dog->move(dt, dirX, running);
        dog->update(dt);
        
        // 更新平台
        for (auto plat : platforms) {
            plat->update(dt);
        }
        
        // 更新所有物品（包括弹出的金币动画）
        for (auto obj : objects) {
            obj->update(dt);
        }
        
        // 更新毛线筐
        if (currentBasket && currentBasket->alive) {
            currentBasket->update(dt);
        }
        
        collision();
        camera();
        
        // 检查是否需要生成新区块
        if (cameraX + width() > nextChunkX - 500) {
            generateChunk();
        }
        
        // 清理远离相机的区块
        cleanupOldChunks();
        
        // 分数
        score = std::max(score, (int)(cameraX / 10));
        
        // 检查是否触发推箱子关卡
        checkSokobanTrigger();
    }
    
    void collision() {
        dog->onGround = false;
        
        QRectF lastRect(dog->lastPos, dog->size);
        QRectF dogRect = dog->getRect();  // 使用完整碰撞盒
        
        // 平台碰撞
        for (auto plat : platforms) {
            QRectF platRect(plat->pos, plat->size);
            
            if (!dogRect.intersects(platRect)) continue;
            
            bool top = lastRect.bottom() <= platRect.top() + 5 && dog->speedY >= 0;  // 增加5像素容差
            bool bottom = lastRect.top() >= platRect.bottom() - 5;
            bool left = lastRect.right() <= platRect.left() + 5;
            bool right = lastRect.left() >= platRect.right() - 5;
            
            if (top) {
                // 从上方落到平台上
                dog->pos.setY(platRect.top() - dog->size.height());
                dog->speedY = 0;
                dog->onGround = true;
            } else if (left) {
                dog->pos.setX(platRect.left() - dog->size.width());
            } else if (right) {
                dog->pos.setX(platRect.right());
            } else if (bottom) {
                // 从下方顶到平台
                dog->pos.setY(platRect.bottom());
                if (dog->speedY < 0) {
                    dog->speedY = 0;
                    
                    // 如果是问号方块且有物品，生成弹出的金币（在hit()之前检查）
                    if (plat->type == QUESTION && plat->hasItem) {
                        // 金币从方块中心顶部弹出
                        double coinX = plat->pos.x() + (plat->size.width() - 48) / 2;
                        double coinY = plat->pos.y() - 48;  // 从方块顶部开始
                        objects.push_back(new Coin(coinX, coinY, true));  // true表示从方块弹出
                        score += 100;  // 顶出金币获得100分
                    }
                    
                    // 顶方块（这会将hasItem设为false）
                    plat->hit();
                }
            } else {
                double down = platRect.bottom() - dogRect.top();
                double up = dogRect.bottom() - platRect.top();
                if (up < down) {
                    dog->pos.setY(platRect.top() - dog->size.height());
                    dog->speedY = 0;
                    dog->onGround = true;
                } else {
                    dog->pos.setY(platRect.bottom());
                    if (dog->speedY < 0) {
                        dog->speedY = 0;
                        
                        // 如果是问号方块且有物品，生成弹出的金币（在hit()之前检查）
                        if (plat->type == QUESTION && plat->hasItem) {
                            // 金币从方块中心顶部弹出
                            double coinX = plat->pos.x() + (plat->size.width() - 48) / 2;
                            double coinY = plat->pos.y() - 48;
                            objects.push_back(new Coin(coinX, coinY, true));
                            score += 100;
                        }
                        
                        // 顶方块（这会将hasItem设为false）
                        plat->hit();
                    }
                }
            }
            
            dogRect = dog->getRect();
        }
        
        // 物品碰撞（使用缩减碰撞盒，手感更好）
        for (auto obj : objects) {
            if (!obj->alive) continue;
            
            // 金币
            Coin* coin = dynamic_cast<Coin*>(obj);
            if (coin) {
                // 从方块弹出的金币不参与碰撞检测（只是视觉效果）
                if (coin->isFromBlock) {
                    continue;
                }
                
                // 普通金币需要碰撞检测（使用缩减碰撞盒，更宽容）
                if (dog->getShrinkRect().intersects(obj->getShrinkRect())) {
                    score += 10;
                    obj->alive = false;
                }
                continue;
            }
            
            // 毛线筐碰撞检测
            Basket* basket = dynamic_cast<Basket*>(obj);
            if (basket && !basket->triggered) {
                if (dog->getRect().intersects(basket->getRect())) {
                    // 触发推箱子关卡
                    basket->triggered = true;
                    startTransitionToSokoban();
                }
                continue;
            }
            
            // 障碍物碰撞检测（使用完整碰撞盒，更准确）
            Obstacle* obstacle = dynamic_cast<Obstacle*>(obj);
            if (obstacle) {
                QRectF dogRect = dog->getRect();  // 使用完整碰撞盒
                QRectF obstacleRect = obj->getRect();  // 使用完整碰撞盒
                
                // 检查是否碰撞
                if (!dogRect.intersects(obstacleRect)) continue;
                bool stompedFromAbove = (dog->speedY > 0) && 
                                       (dog->lastPos.y() + dog->size.height() <= obstacleRect.top() + 10);
                
                if (stompedFromAbove) {
                    // 踩踏成功！消灭敌人并反弹
                    obj->alive = false;
                    dog->stompBounce();
                    score += 50;  // 踩踏敌人额外加分
                } else {
                    // 侧面碰撞，受伤
                    obj->alive = false;
                    lives--;
                    if (lives <= 0) {
                        state = 2;
                        timer->stop();
                    }
                }
                
            }
        }
        
        // 掉入坑洞检测 - 直接游戏结束
        if (dog->pos.y() > 620) {  // 超出屏幕底部
            state = 2;  // 直接游戏结束
            timer->stop();
        }
    }
    
    void camera() {
        double trigger = width() * 0.4;
        double target = dog->pos.x() - trigger;
        if (target > cameraX) {
            cameraX = target;
        }
    }
    
    void spawn(int elapsed) {
        spawnTime += elapsed;
        if (spawnTime < 2000) return;
        spawnTime -= 2000;
        
        double x = cameraX + width() + 80;
        
        // 检查1：是否靠近问号方块
        bool nearQuestionBlock = false;
        for (auto plat : platforms) {
            if (plat->type == QUESTION) {
                // 问号方块的X范围
                double blockLeft = plat->pos.x() - 150;  // 左侧安全距离
                double blockRight = plat->pos.x() + plat->size.width() + 150;  // 右侧安全距离
                
                // 如果生成位置在问号方块附近，标记为不安全
                if (x >= blockLeft && x <= blockRight) {
                    nearQuestionBlock = true;
                    break;
                }
            }
        }
        
        // 检查2：是否与已有砖块重叠
        bool overlapsPlatform = false;
        for (auto plat : platforms) {
            // 跳过地面（地面很长，会导致无法生成）
            if (plat->type == GROUND) continue;
            
            // 检查水平重叠
            double platLeft = plat->pos.x() - 100;  // 左侧缓冲
            double platRight = plat->pos.x() + plat->size.width() + 100;  // 右侧缓冲
            
            if (x >= platLeft && x <= platRight) {
                // 检查垂直重叠（障碍物在Y=452，金币在Y=220-340）
                double platTop = plat->pos.y();
                double platBottom = plat->pos.y() + plat->size.height();
                
                // 障碍物高度范围：452-500
                // 金币高度范围：220-340
                // 如果砖块在这些范围内，就会重叠
                if ((platBottom >= 220 && platTop <= 500)) {
                    overlapsPlatform = true;
                    break;
                }
            }
        }
        
        // 如果与砖块重叠，跳过本次生成
        if (overlapsPlatform) {
            return;
        }
        
        // 生成元素
        std::uniform_int_distribution<int> type(0, 1);
        std::uniform_real_distribution<double> coinY(220, 340);
        
        if (type(rng) == 0) {
            // 总是可以生成金币（空中不会干扰）
            objects.push_back(new Coin(x, coinY(rng)));
        } else {
            // 只在远离问号方块时生成障碍物
            if (!nearQuestionBlock) {
                objects.push_back(new Obstacle(x, 452));
            } else {
                // 如果靠近问号方块，生成金币代替
                objects.push_back(new Coin(x, coinY(rng)));
            }
        }
    }
    
    void cleanup() {
        auto it = objects.begin();
        while (it != objects.end()) {
            GameObject* obj = *it;
            double right = obj->pos.x() + obj->size.width();
            if (!obj->alive || right < cameraX) {
                // 如果要删除的对象是 currentBasket，先清空指针
                if (obj == currentBasket) {
                    currentBasket = nullptr;
                }
                delete obj;
                it = objects.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void restart() {
        keys.clear();
        
        for (auto obj : objects) delete obj;
        objects.clear();
        
        for (auto plat : platforms) delete plat;
        platforms.clear();
        
        if (currentBasket) {
            delete currentBasket;
            currentBasket = nullptr;
        }
        
        spawnTime = 0;
        cameraX = 0;
        score = 0;
        lives = 3;
        
        nextSokobanScore = 1000;
        sokobanLevel = 0;
        isTransitioning = false;
        transitionProgress = 0;
        sokobanCompleted = false;
        
        nextChunkX = 0;
        
        delete dog;
        dog = new Dog(80, 404);  
        dog->speedX = 0;  
        dog->speedY = 0;
        dog->onGround = true;
        dog->coyoteTime = 0;
        dog->jumpPressed = false;
        
        createLevel();
        
        frameTime.restart();
        if (!timer->isActive()) {
            timer->start();
        }
    }
    
    void createLevel() {
        generateChunk();
        generateChunk();
        generateChunk();
    }
    
    // 生成一个新区块
    void generateChunk() {
        int startX = nextChunkX;
        
        // 随机选择区块类型（0-4）
        std::uniform_int_distribution<int> chunkType(0, 4);
        int type = chunkType(rng);
        
        switch(type) {
            case 0:
                generatePlainChunk(startX);
                break;
            case 1:
                generatePitChunk(startX);
                break;
            case 2:
                generatePlatformChunk(startX);
                break;
            case 3:
                generateStairChunk(startX);
                break;
            case 4:
                generateQuestionChunk(startX);
                break;
        }
        
        nextChunkX += chunkWidth;
    }
    
    // 区块类型1：平原区（简单地面）
    void generatePlainChunk(int startX) {
        
        platforms.push_back(new Platform(startX, 500, chunkWidth, 120, GROUND));
        
        // 随机添加问号方块
        std::uniform_int_distribution<int> questionChance(0, 1);
        if (questionChance(rng) == 0) {
            std::uniform_real_distribution<double> questionX(startX + 300, startX + chunkWidth - 300);
            double qx = questionX(rng);
            platforms.push_back(new Platform(qx, 308, 96, 96, QUESTION));
        }
        
        // 随机添加一些金币
        std::uniform_int_distribution<int> coinCount(2, 5);
        std::uniform_real_distribution<double> coinX(startX + 100, startX + chunkWidth - 100);
        std::uniform_real_distribution<double> coinY(300, 400);
        
        int coins = coinCount(rng);
        for (int i = 0; i < coins; i++) {
            objects.push_back(new Coin(coinX(rng), coinY(rng)));
        }
        
        // 随机添加障碍物
        std::uniform_int_distribution<int> obstacleChance(0, 2);
        if (obstacleChance(rng) == 0) {
            std::uniform_real_distribution<double> obstacleX(startX + 200, startX + chunkWidth - 200);
            objects.push_back(new Obstacle(obstacleX(rng), 452));
        }
    }
    
    // 区块类型2：坑洞区（需要跳跃）
    void generatePitChunk(int startX) {
        platforms.push_back(new Platform(startX, 500, 400, 120, GROUND));
        platforms.push_back(new Platform(startX + 700, 500, chunkWidth - 700, 120, GROUND));

        platforms.push_back(new Platform(startX + 450, 320, 64, 64, BLOCK));  
        platforms.push_back(new Platform(startX + 550, 320, 64, 64, BLOCK));  

        objects.push_back(new Coin(startX + 500, 230));
    }
    
    // 区块类型3：平台区（多层平台）
    void generatePlatformChunk(int startX) {
        // 地面
        platforms.push_back(new Platform(startX, 500, chunkWidth, 120, GROUND));
    
        platforms.push_back(new Platform(startX + 200, 350, 128, 64, BLOCK));
        platforms.push_back(new Platform(startX + 500, 350, 128, 64, BLOCK));
        
        // 中层平台
        platforms.push_back(new Platform(startX + 350, 300, 128, 64, BLOCK));
        platforms.push_back(new Platform(startX + 650, 300, 128, 64, BLOCK));
        
        // 问号方块
        platforms.push_back(new Platform(startX + 400, 100, 96, 96, QUESTION));
        
        // 金币
        objects.push_back(new Coin(startX + 250, 302)); 
        objects.push_back(new Coin(startX + 550, 302)); 
        objects.push_back(new Coin(startX + 700, 198)); 
    }
    
    // 区块类型4：阶梯区（爬升）
    void generateStairChunk(int startX) {
        // 地面
        platforms.push_back(new Platform(startX, 500, 300, 120, GROUND));
        
        // 阶梯
        platforms.push_back(new Platform(startX + 300, 436, 64, 64, BLOCK));
        platforms.push_back(new Platform(startX + 380, 372, 64, 64, BLOCK));
        platforms.push_back(new Platform(startX + 460, 308, 64, 64, BLOCK));
        platforms.push_back(new Platform(startX + 540, 244, 64, 64, BLOCK));
        
        // 高台
        platforms.push_back(new Platform(startX + 620, 244, 200, 64, BLOCK));
        
        // 下降阶梯
        platforms.push_back(new Platform(startX + 820, 308, 64, 64, BLOCK));
        
        // 地面继续
        platforms.push_back(new Platform(startX + 900, 500, chunkWidth - 900, 120, GROUND));
        
        // 金币
        objects.push_back(new Coin(startX + 350, 300));
        objects.push_back(new Coin(startX + 430, 240));
        objects.push_back(new Coin(startX + 510, 180));
        objects.push_back(new Coin(startX + 720, 180));
    }
    
    // 区块类型5：问号方块区
    void generateQuestionChunk(int startX) {
        // 地面
        platforms.push_back(new Platform(startX, 500, chunkWidth, 120, GROUND));
        
        // 一排问号方块
        platforms.push_back(new Platform(startX + 150, 268, 96, 96, QUESTION));
        platforms.push_back(new Platform(startX + 350, 128, 96, 96, QUESTION));
        platforms.push_back(new Platform(startX + 600, 88, 96, 96, QUESTION));
        
        // 普通方块
        platforms.push_back(new Platform(startX + 350, 350, 128, 64, BLOCK));
        platforms.push_back(new Platform(startX + 560, 300, 128, 64, BLOCK));
        
        // 障碍物（确保在问号方块区域之外，避免在方块下方）
        std::uniform_int_distribution<int> obstacleChance(0, 2);
        if (obstacleChance(rng) == 0) {
            std::uniform_real_distribution<double> obstacleX(startX + 50, startX + 150);
            objects.push_back(new Obstacle(obstacleX(rng), 452));
        } else if (obstacleChance(rng) == 1) {
            std::uniform_real_distribution<double> obstacleX(startX + 900, startX + 1100);
            objects.push_back(new Obstacle(obstacleX(rng), 452));
        }
        // else: 不生成障碍物
    }
    
    // 清理远离相机的区块
    void cleanupOldChunks() {
        // 删除相机左侧1000像素外的平台和物品
        double cleanupX = cameraX - 1000;
        
        // 清理平台
        auto platIt = platforms.begin();
        while (platIt != platforms.end()) {
            Platform* plat = *platIt;
            if (plat->pos.x() + plat->size.width() < cleanupX) {
                delete plat;
                platIt = platforms.erase(platIt);
            } else {
                ++platIt;
            }
        }
        
        // 清理物品
        auto objIt = objects.begin();
        while (objIt != objects.end()) {
            GameObject* obj = *objIt;
            if (obj->pos.x() + obj->size.width() < cleanupX || !obj->alive) {
                if (obj == currentBasket) {
                    currentBasket = nullptr;
                }
                delete obj;
                objIt = objects.erase(objIt);
            } else {
                ++objIt;
            }
        }
    }
    
    // 检查是否触发推箱子关卡
    void checkSokobanTrigger() {
        if (score >= nextSokobanScore && !currentBasket && sokobanLevel < 3) {
            // 在前方生成毛线筐
            double basketX = cameraX + width() + 200;
            double basketY = 420;  // 地面上方
            currentBasket = new Basket(basketX, basketY);
            objects.push_back(currentBasket);
        }
    }
    
    // 开始转场到推箱子模式
    void startTransitionToSokoban() {
        isTransitioning = true;
        transitionType = 0;  // 进入推箱子
        transitionProgress = 0;
        dogTransitionPos = dog->pos;
        dogTransitionScale = 1.0f;
    }
    
    // 开始转场回跑酷模式
    void startTransitionToRunner() {
        isTransitioning = true;
        transitionType = 1;  // 退出推箱子
        transitionProgress = 0;
    }
    
    // 更新转场动画
    void updateTransition(float dt) {
        transitionProgress += dt * 2.0f;  // 0.5秒转场
        
        if (transitionProgress >= 1.0f) {
            transitionProgress = 1.0f;
            isTransitioning = false;
            
            if (transitionType == 0) {
                // 完成进入推箱子的转场
                state = 3;
                sokobanManager->loadLevel(sokobanLevel);
            } else if (transitionType == 1) {
                // 完成退出推箱子的转场
                state = 1;
                
                // 如果是完成关卡（而非ESC退出），恢复一条生命
                if (sokobanCompleted && lives < 3) {
                    lives++;
                    sokobanCompleted = false;  // 重置标志
                }
                
                // 更新下一个触发分数（无论是否完成所有关卡）
                sokobanLevel++;
                if (sokobanLevel < 3) {
                    nextSokobanScore = 1000 * (sokobanLevel + 1);
                } else {
                    // 已完成所有关卡，不再生成毛线筐
                    nextSokobanScore = 999999;  // 设置一个很大的值
                }
                
                currentBasket = nullptr;  // 清除毛线筐引用
            }
        }
        
        // 进入推箱子的动画：小狗缩小并下落
        if (transitionType == 0 && state == 1) {
            dogTransitionScale = 1.0f - transitionProgress * 0.8f;  // 缩小到20%
            dogTransitionPos.setY(dogTransitionPos.y() + transitionProgress * 100);  // 下落
        }
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    Game game;
    game.show();
    
    return app.exec();
}
