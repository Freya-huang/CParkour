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

// 平台
class Platform : public GameObject {
public:
    Platform(double x, double y, double w, double h) {
        pos = QPointF(x, y);
        size = QSizeF(w, h);
        image = QPixmap(":/images/scene.png");
    }
    
    void draw(QPainter* painter) override {
        if (painter) {
            painter->drawTiledPixmap(QRectF(pos, size), image);
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
        size = QSizeF(72, 72);
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
        onGround = false;
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
        speedY = -15.0;
        onGround = false;
    }
    
    void stopJump() {
        if (speedY < 0) {
            speedY *= 0.5;
            if (speedY > -3.0) {
                speedY = -3.0;
            }
        }
    }
    
    void update(float dt) override {
        lastPos = pos;
        
        // 重力
        pos.setY(pos.y() + speedY * dt + 0.5 * 9.8 * dt * dt);
        speedY = speedY + 9.8 * dt;
        
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
        
        // 创建地面
        platforms.push_back(new Platform(0, 500, 2000, 120));
        
        // 初始化游戏
        state = 0;
        dog = new Dog(80, 220);
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
        // 移动
        bool left = keys.count(Qt::Key_A) || keys.count(Qt::Key_Left);
        bool right = keys.count(Qt::Key_D) || keys.count(Qt::Key_Right);
        
        double dir = 0;
        if (left && !right) dir = -1;
        else if (right && !left) dir = 1;
        
        dog->pos.setX(dog->pos.x() + dir * 360.0 * dt);
        dog->update(dt);
        
        collision();
        camera();
        
        // 更新地面
        if (!platforms.empty()) {
            platforms[0]->pos = QPointF(cameraX - 100, 500);
            platforms[0]->size = QSizeF(width() + 200, 120);
        }
        
        // 分数
        score = std::max(score, (int)(cameraX / 10));
    }
    
    void collision() {
        dog->onGround = false;
        
        QRectF lastRect(dog->lastPos, dog->size);
        QRectF dogRect = dog->getRect();
        
        // 平台碰撞
        for (auto plat : platforms) {
            QRectF platRect(plat->pos, plat->size);
            
            if (!dogRect.intersects(platRect)) continue;
            
            bool top = lastRect.bottom() <= platRect.top() && dog->speedY >= 0;
            bool bottom = lastRect.top() >= platRect.bottom();
            bool left = lastRect.right() <= platRect.left();
            bool right = lastRect.left() >= platRect.right();
            
            if (top) {
                dog->pos.setY(platRect.top() - dog->size.height());
                dog->speedY = 0;
                dog->onGround = true;
            } else if (left) {
                dog->pos.setX(platRect.left() - dog->size.width());
            } else if (right) {
                dog->pos.setX(platRect.right());
            } else if (bottom) {
                dog->pos.setY(platRect.bottom());
                if (dog->speedY < 0) dog->speedY = 0;
            } else {
                double down = platRect.bottom() - dogRect.top();
                double up = dogRect.bottom() - platRect.top();
                if (up < down) {
                    dog->pos.setY(platRect.top() - dog->size.height());
                    dog->speedY = 0;
                    dog->onGround = true;
                } else {
                    dog->pos.setY(platRect.bottom());
                    if (dog->speedY < 0) dog->speedY = 0;
                }
            }
            
            dogRect = dog->getRect();
        }
        
        // 物品碰撞
        for (auto obj : objects) {
            if (!obj->alive) continue;
            if (!dog->getRect().intersects(obj->getRect())) continue;
            
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
        
        spawnTime = 0;
        cameraX = 0;
        score = 0;
        lives = 3;
        
        delete dog;
        dog = new Dog(80, 220);
        
        frameTime.restart();
        if (!timer->isActive()) {
            timer->start();
        }
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    Game game;
    game.show();
    
    return app.exec();
}
