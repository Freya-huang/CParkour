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
    
    Platform(double x, double y, double w, double h, PlatformType t = GROUND) {
        pos = QPointF(x, y);
        size = QSizeF(w, h);
        type = t;
        hasItem = true;
        bounceTimer = 0;
        
        if (type == GROUND) {
            image = QPixmap(":/images/scene.png");
        } else {
            // 方块用纯色代替（如果没有专门的图片）
            image = QPixmap(64, 64);
            if (type == QUESTION) {
                image.fill(QColor(255, 200, 0));  // 金色问号方块
            } else {
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
            // 变成灰色
            image.fill(QColor(150, 150, 150));
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
                
                // 画问号
                if (type == QUESTION && hasItem) {
                    painter->setPen(Qt::white);
                    painter->setFont(QFont("Arial", 32, QFont::Bold));
                    painter->drawText(rect, Qt::AlignCenter, "?");
                }
            }
        }
    }
};

// 金币
class Coin : public GameObject {
public:
    Coin(double x, double y) {
        pos = QPointF(x, y);
        size = QSizeF(48, 48);
        image = QPixmap(":/images/coin.png");
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
    bool onGround;
    QPixmap img1, img2;
    int animTimer;
    bool showImg1;
    
    Dog(double x, double y) {
        pos = QPointF(x, y);
        lastPos = pos;
        size = QSizeF(96, 96);
        speedY = 0;
        onGround = true;  // 初始在地面上
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
        // 只能在地面上跳跃
        if (!onGround) return;
        
        speedY = -100.0;  // 增加跳跃力度，跳得更高
        onGround = false;
    }
    
    void stopJump() {
        // 松开跳跃键时，如果还在上升，立即减速
        // 这样可以实现短跳和长跳
        if (speedY < -12.0) {
            speedY = -12.0;
        }
    }
    
    void update(float dt) override {
        lastPos = pos;
        
        // 马里奥风格的重力系统
        float gravity = 22.0;  // 调整重力，配合新的跳跃力度
        
        // 上升时重力较轻，下落时重力较重（马里奥的特色）
        if (speedY < 0) {
            gravity = 10.0;  // 上升时重力小一些
        } else {
            gravity = 150.0;  // 增加下落重力，掉坑更快
        }
        
        // 应用重力
        pos.setY(pos.y() + speedY * dt + 0.5 * gravity * dt * dt);
        speedY = speedY + gravity * dt;
        
        // 限制最大下落速度（增加上限）
        if (speedY > 40.0) {
            speedY = 40.0;
        }
        
        // 动画
        animTimer += (int)(dt * 1000);
        if (animTimer >= 100) {
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
            p.drawText(QRect(0, height() - 90, width(), 60), 
                      Qt::AlignCenter, "按空格开始");
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
        
        // 游戏结束
        if (state == 2) {
            p.fillRect(rect(), QColor(0, 0, 0, 120));
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 26, QFont::Bold));
            p.drawText(rect(), Qt::AlignCenter, 
                      QString("游戏结束\n分数: %1\n按R重新开始").arg(score));
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
        // 马里奥风格的移动速度
        bool left = keys.count(Qt::Key_A) || keys.count(Qt::Key_Left);
        bool right = keys.count(Qt::Key_D) || keys.count(Qt::Key_Right);
        
        double dir = 0;
        if (left && !right) dir = -1;
        else if (right && !left) dir = 1;
        
        // 地面和空中移动速度不同
        double moveSpeed = 250.0;  // 地面速度
        if (!dog->onGround) {
            moveSpeed = 200.0;  // 空中控制力稍弱
        }
        
        dog->pos.setX(dog->pos.x() + dir * moveSpeed * dt);
        dog->update(dt);
        
        // 更新平台
        for (auto plat : platforms) {
            plat->update(dt);
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
                    // 顶方块
                    plat->hit();
                    // 如果是问号方块且有物品，生成金币
                    if (plat->type == QUESTION && plat->hasItem) {
                        objects.push_back(new Coin(plat->pos.x() + 8, plat->pos.y() - 60));
                        score += 10;
                    }
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
                        plat->hit();
                        if (plat->type == QUESTION && plat->hasItem) {
                            objects.push_back(new Coin(plat->pos.x() + 8, plat->pos.y() - 60));
                            score += 10;
                        }
                    }
                }
            }
            
            dogRect = dog->getRect();
        }
        
        // 物品碰撞（使用缩减碰撞盒，手感更好）
        for (auto obj : objects) {
            if (!obj->alive) continue;
            if (!dog->getShrinkRect().intersects(obj->getShrinkRect())) continue;
            
            // 金币
            if (dynamic_cast<Coin*>(obj)) {
                score += 10;
                obj->alive = false;
            }
            
            // 障碍物
            if (dynamic_cast<Obstacle*>(obj)) {
                obj->alive = false;
                lives--;
                if (lives <= 0) {
                    state = 2;
                    timer->stop();
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
        
        std::uniform_int_distribution<int> type(0, 1);
        std::uniform_real_distribution<double> coinY(220, 340);
        
        if (type(rng) == 0) {
            objects.push_back(new Coin(x, coinY(rng)));
        } else {
            objects.push_back(new Obstacle(x, 428));
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
        
        // 低层平台
        platforms.push_back(new Platform(startX + 200, 400, 128, 64, BLOCK));
        platforms.push_back(new Platform(startX + 500, 400, 128, 64, BLOCK));
        
        // 中层平台
        platforms.push_back(new Platform(startX + 350, 300, 128, 64, BLOCK));
        platforms.push_back(new Platform(startX + 650, 300, 128, 64, BLOCK));
        
        // 问号方块
        platforms.push_back(new Platform(startX + 400, 250, 64, 64, QUESTION));
        
        // 金币
        objects.push_back(new Coin(startX + 250, 330));
        objects.push_back(new Coin(startX + 550, 330));
        objects.push_back(new Coin(startX + 700, 230));
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
        
        // 一排问号方块
        platforms.push_back(new Platform(startX + 200, 300, 64, 64, QUESTION));
        platforms.push_back(new Platform(startX + 300, 300, 64, 64, QUESTION));
        platforms.push_back(new Platform(startX + 400, 300, 64, 64, QUESTION));
        platforms.push_back(new Platform(startX + 500, 300, 64, 64, QUESTION));
        
        // 普通方块
        platforms.push_back(new Platform(startX + 600, 350, 64, 64, BLOCK));
        platforms.push_back(new Platform(startX + 700, 350, 64, 64, BLOCK));
        
        // 障碍物
        objects.push_back(new Obstacle(startX + 800, 452));
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
