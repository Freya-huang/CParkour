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
    
    // 获取碰撞区域
    QRectF getRect() {
        // 直接返回完整矩形，不缩减
        return QRectF(pos, size);
    }
    
    // 获取用于显示碰撞的缩减矩形（仅用于金币等物品）
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
    bool hasItem;  // 问号方块是否还有物品
    int bounceTimer;  // 被顶时的弹跳动画
    QPixmap questionBlockImg;  // 问号方块图片
    QPixmap questionBlockUsedImg;  // 使用后的问号方块图片
    
    Platform(double x, double y, double w, double h, PlatformType t = GROUND) {
        pos = QPointF(x, y);
        size = QSizeF(w, h);
        type = t;
        hasItem = true;
        bounceTimer = 0;
        
        if (type == GROUND) {
            image = QPixmap(":/images/scene.png");
        } else if (type == QUESTION) {
            // 加载问号方块图片
            questionBlockImg = QPixmap(":/images/question_block.png");
            questionBlockUsedImg = QPixmap(":/images/question_block_used.png");
            
            // 如果图片加载失败，使用纯色作为后备
            if (questionBlockImg.isNull()) {
                questionBlockImg = QPixmap(64, 64);
                questionBlockImg.fill(QColor(255, 200, 0));  // 金色
            }
            if (questionBlockUsedImg.isNull()) {
                questionBlockUsedImg = QPixmap(64, 64);
                questionBlockUsedImg.fill(QColor(150, 150, 150));  // 灰色
            }
            
            image = questionBlockImg;
        } else if (type == BLOCK) {
            // 加载砖块图片
            image = QPixmap(":/images/block.png");
            
            // 如果图片加载失败，使用纯色作为后备
            if (image.isNull()) {
                image = QPixmap(64, 64);
                image.fill(QColor(139, 69, 19));  // 棕色砖块
            }
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
                
                // 如果问号方块图片加载失败（使用纯色后备），则画问号文字
                if (type == QUESTION && hasItem && questionBlockImg.width() == 64 && questionBlockImg.height() == 64) {
                    // 检查是否是纯色填充（后备方案）
                    QImage img = questionBlockImg.toImage();
                    if (img.pixel(0, 0) == img.pixel(32, 32)) {
                        // 可能是纯色，画问号
                        painter->setPen(Qt::white);
                        painter->setFont(QFont("Arial", 32, QFont::Bold));
                        painter->drawText(rect, Qt::AlignCenter, "?");
                    }
                }
            }
        }
    }
};

// 金币
class Coin : public GameObject {
public:
    bool isFromBlock;  // 是否从方块弹出
    float velocityY;   // 垂直速度（用于弹出动画）
    float gravity;     // 重力
    int lifetime;      // 生命周期（毫秒）
    int maxLifetime;   // 最大生命周期
    
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
        size = QSizeF(64, 48);  // 改成64宽x48高，更矮，可以跳过
        image = QPixmap(":/images/obstacle.png");
    }
};

// 小狗
class Dog : public GameObject {
public:
    QPointF lastPos;
    float speedY;
    float speedX;  // 水平速度（用于助跑）
    bool onGround;
    bool wasOnGround;  // 上一帧是否在地面
    int coyoteTime;  // 土狼时间（毫秒）
    bool jumpPressed;  // 跳跃键是否按下
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
        
        // 动画（速度越快，动画越快）
        int animSpeed = 100;
        if (std::abs(speedX) > MAX_SPEED_WALK) {
            animSpeed = 60;  // 奔跑时动画更快
        }
        animTimer += (int)(dt * 1000);
        if (animTimer >= animSpeed) {
            animTimer = 0;
            showImg1 = !showImg1;
        }
        image = showImg1 ? img1 : img2;
    }
};

// 游戏主窗口
class Game : public QMainWindow {
public:
    // 游戏状态
    int state; // 0=菜单 1=游戏中 2=游戏结束
    
    // 定时器
    QTimer* timer;
    QElapsedTimer frameTime;
    
    // 按键
    std::set<int> keys;
    
    // 游戏对象
    Dog* dog;
    std::vector<Platform*> platforms;
    std::vector<GameObject*> objects;
    
    // 相机
    double cameraX;
    
    // 图片
    QPixmap bgImg, menuImg, heartImg;
    
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
        bgImg = QPixmap(":/images/scene.png");
        menuImg = QPixmap(":/images/menu.png");
        heartImg = QPixmap(":/images/heart.png");
        
        // 初始化随机数
        rng.seed(std::random_device{}());
        
        // 初始化区块系统
        chunkWidth = 1000;
        nextChunkX = 0;
        
        // 创建初始关卡
        createLevel();
        
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
        for (auto p : platforms) delete p;
        for (auto obj : objects) delete obj;
    }
    
    void paintEvent(QPaintEvent* e) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        // 菜单
        if (state == 0) {
            p.fillRect(rect(), QColor(245, 245, 245));
            p.drawPixmap(rect(), menuImg);
            p.setPen(Qt::black);
            p.setFont(QFont("Arial", 18, QFont::Bold));
            p.drawText(QRect(0, height() - 150, width(), 60), 
                      Qt::AlignCenter, "按空格开始");
            
            // 操作说明
            p.setFont(QFont("Arial", 14));
            p.drawText(QRect(0, height() - 90, width(), 80), Qt::AlignCenter,
                      "马里奥式跳跃系统\n空格=跳跃（长按跳更高） | Shift=奔跑加速\n踩踏敌人可消灭 | 按住跳跃键踩踏弹得更高");
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
        dog->draw(&p);
        
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
            
            // 障碍物碰撞检测（使用完整碰撞盒，更准确）
            Obstacle* obstacle = dynamic_cast<Obstacle*>(obj);
            if (obstacle) {
                QRectF dogRect = dog->getRect();  // 使用完整碰撞盒
                QRectF obstacleRect = obj->getRect();  // 使用完整碰撞盒
                
                // 检查是否碰撞
                if (!dogRect.intersects(obstacleRect)) continue;
                
                // 判断是否从上方踩踏（小狗在下落且脚部碰到障碍物顶部）
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
        
        spawnTime = 0;
        cameraX = 0;
        score = 0;
        lives = 3;
        
        // 重置区块系统
        nextChunkX = 0;
        
        delete dog;
        dog = new Dog(80, 404);  // 地面高度500 - 小狗高度96 = 404
        dog->speedX = 0;  // 确保重置水平速度
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
        // 创建起始区块（前3个区块）
        generateChunk();  // 第一个区块
        generateChunk();  // 第二个区块
        generateChunk();  // 第三个区块
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
        // 地面
        platforms.push_back(new Platform(startX, 500, chunkWidth, 120, GROUND));
        
        // 随机添加问号方块（50%概率）
        std::uniform_int_distribution<int> questionChance(0, 1);
        if (questionChance(rng) == 0) {
            std::uniform_real_distribution<double> questionX(startX + 300, startX + chunkWidth - 300);
            double qx = questionX(rng);
            // 问号方块在空中，从地面起跳可以顶到
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
        // 坑洞前的地面
        platforms.push_back(new Platform(startX, 500, 400, 120, GROUND));
        
        // 坑洞（400-700之间没有地面）
        
        // 坑洞后的地面
        platforms.push_back(new Platform(startX + 700, 500, chunkWidth - 700, 120, GROUND));
        
        // 坑洞上方的悬浮方块（提高高度，更容易跳上去）
        platforms.push_back(new Platform(startX + 450, 300, 64, 64, BLOCK));  // 从350提高到300
        platforms.push_back(new Platform(startX + 550, 300, 64, 64, BLOCK));  // 从350提高到300
        
        // 空中金币
        objects.push_back(new Coin(startX + 500, 230));  // 金币也相应提高
    }
    
    // 区块类型3：平台区（多层平台）
    void generatePlatformChunk(int startX) {
        // 地面
        platforms.push_back(new Platform(startX, 500, chunkWidth, 120, GROUND));
        
        // 低层平台（修复：从Y=400改为Y=372，避免低于地面）
        // 底部位置：372 + 64 = 436（高于地面500）
        platforms.push_back(new Platform(startX + 200, 372, 128, 64, BLOCK));
        platforms.push_back(new Platform(startX + 500, 372, 128, 64, BLOCK));
        
        // 中层平台
        platforms.push_back(new Platform(startX + 350, 300, 128, 64, BLOCK));
        platforms.push_back(new Platform(startX + 650, 300, 128, 64, BLOCK));
        
        // 问号方块（增大到96x96，更显眼）
        // 位置在中层平台上方，需要在中层平台上起跳才能顶到
        platforms.push_back(new Platform(startX + 400, 100, 96, 96, QUESTION));
        
        // 金币
        objects.push_back(new Coin(startX + 250, 302));  // 低层平台上方
        objects.push_back(new Coin(startX + 550, 302));  // 低层平台上方
        objects.push_back(new Coin(startX + 700, 198));  // 中层平台上方
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
        
        // 金币（移除问号方块，只保留金币）
        objects.push_back(new Coin(startX + 350, 300));
        objects.push_back(new Coin(startX + 430, 240));
        objects.push_back(new Coin(startX + 510, 180));
        objects.push_back(new Coin(startX + 720, 180));
    }
    
    // 区块类型5：问号方块区
    void generateQuestionChunk(int startX) {
        // 地面
        platforms.push_back(new Platform(startX, 500, chunkWidth, 120, GROUND));
        
        // 一排问号方块（增大到96x96）
        // 问号方块范围：X = 200 到 656 (560 + 96)
        platforms.push_back(new Platform(startX + 200, 268, 96, 96, QUESTION));
        platforms.push_back(new Platform(startX + 320, 268, 96, 96, QUESTION));
        platforms.push_back(new Platform(startX + 440, 268, 96, 96, QUESTION));
        platforms.push_back(new Platform(startX + 560, 268, 96, 96, QUESTION));
        
        // 普通方块（在问号方块之后）
        platforms.push_back(new Platform(startX + 700, 350, 64, 64, BLOCK));
        platforms.push_back(new Platform(startX + 800, 350, 64, 64, BLOCK));
        
        // 障碍物（确保在问号方块区域之外，避免在方块下方）
        // 问号方块结束位置：656，安全距离：至少100像素
        std::uniform_int_distribution<int> obstacleChance(0, 2);
        if (obstacleChance(rng) == 0) {
            // 障碍物在问号方块之前（区块开始处）
            std::uniform_real_distribution<double> obstacleX(startX + 50, startX + 150);
            objects.push_back(new Obstacle(obstacleX(rng), 452));
        } else if (obstacleChance(rng) == 1) {
            // 障碍物在问号方块之后（区块结束处）
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
                delete obj;
                objIt = objects.erase(objIt);
            } else {
                ++objIt;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    Game game;
    game.show();
    
    return app.exec();
}
