// ObjectionWindow.cpp
#include "ObjectionWindow.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QCursor>
#include <QDebug>
#include <QTimer>
#include <QSettings>

bool ObjectionWindow::globalMuted = false; //静音模式关闭
int getCurrentLanguage() {
    QSettings settings("MilesEdgeworth", "DesktopPet");
    return settings.value("language", 0).toInt();
}

// 定义静态成员变量
ObjectionWindow* ObjectionWindow::activeWindow = nullptr;

ObjectionWindow::ObjectionWindow(QWidget* parent) : QWidget(parent) {
    // 设置窗口属性：无边框、透明背景、置顶
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动删除
    // 添加：初始化时获取主窗口的静音状态
    QSettings settings("MilesEdgeworth", "DesktopPet");
    bool isMuted = settings.value("muted", false).toBool();
    setMuted(isMuted);

    // 标记为当前活动窗口
    activeWindow = this;

    setupUi();
}

ObjectionWindow::~ObjectionWindow() {
    // 清理资源
    if (objectionMovie) {
        objectionMovie->stop();
        delete objectionMovie;
    }
    if (soundEffect) {
        delete soundEffect;
    }

    // 清除活动状态
    activeWindow = nullptr;
}

void ObjectionWindow::setMuted(bool muted) {
    if (soundEffect) {
        soundEffect->setVolume(muted ? 0 : 0.8f);
    }
}

// 新增静态同步方法
void ObjectionWindow::syncMuteState(bool muted) {
    globalMuted = muted;
    if (activeWindow) {
        activeWindow->setMuted(muted);
    }
}

void ObjectionWindow::setupUi() {
    // 先创建音效对象
    setMuted(globalMuted);  // 确保使用当前静音状态
    // 创建音效对象 - 根据语言选择不同音效
    QString audioPath;
    switch (getCurrentLanguage()) {
    case 0: // 日语
        audioPath = ":/audios/objection0.wav";
        break;
    case 1: // 英语
        audioPath = ":/audios/objection1.wav";
        break;
    case 2: // 中文
        audioPath = ":/audios/objection2.wav";
        break;
    default:
        audioPath = ":/audios/objection0.wav"; // 默认日语
    }
    // ... 音效路径选择代码 ...
    soundEffect = new QSoundEffect(this);
    soundEffect->setSource(QUrl::fromLocalFile(audioPath));

    // 然后设置初始音量
    QSettings settings("MilesEdgeworth", "DesktopPet");
    soundEffect->setVolume(settings.value("muted", false).toBool() ? 0 : 0.8f);
    // 创建布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 创建显示GIF的标签
    gifLabel = new QLabel(this);
    gifLabel->setAlignment(Qt::AlignCenter);
    gifLabel->setScaledContents(false); // 修改点1：禁用自动缩放
    layout->addWidget(gifLabel);

    // 创建GIF动画对象 - 根据语言选择不同GIF
    QString gifPath;
    switch (getCurrentLanguage()) {
    case 0: // 日语
        gifPath = ":/gifs/objection/objection0.gif";
        break;
    case 1: // 英语
        gifPath = ":/gifs/objection/objection1.gif";
        break;
    case 2: // 中文
        gifPath = ":/gifs/objection/objection2.gif";
        break;
    default:
        gifPath = ":/gifs/objection/objection0.gif"; // 默认日语
    }
    objectionMovie = new QMovie(gifPath);

    // 连接动画结束信号
    connect(objectionMovie, &QMovie::finished, this, [this]() {
        close();
        });

    // 修改点2：添加手动缩放处理
    connect(objectionMovie, &QMovie::frameChanged, this, [this](int frame) {
        if (frame >= 0) {
            QImage img = objectionMovie->currentImage();
            // 高质量缩放至标签大小，保持宽高比并平滑
            img = img.scaled(gifLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            gifLabel->setPixmap(QPixmap::fromImage(img));

        }
        });

    // 音效初始化（移到所有设置完成后）
    soundEffect = new QSoundEffect(this);
    soundEffect->setSource(QUrl::fromLocalFile(audioPath));
    soundEffect->setVolume(globalMuted ? 0 : 0.8f);  // 直接使用静态变量
}

void ObjectionWindow::showObjectionAt(const QPoint& globalPos) {
    // 设置GIF显示尺寸
    int gifWidth = 192;
    int gifHeight = 108;
    setFixedSize(gifWidth, gifHeight);

    // 计算显示位置
    QPoint absolutePos = QCursor::pos();
    int posX = absolutePos.x() - (gifWidth / 2);
    int posY = absolutePos.y() - (gifHeight / 2);

    // 确保窗口在屏幕内
    QScreen* screen = QApplication::primaryScreen();
    QRect screenRect = screen->geometry();
    if (posX < screenRect.left()) posX = screenRect.left();
    else if (posX + gifWidth > screenRect.right()) posX = screenRect.right() - gifWidth;
    if (posY < screenRect.top()) posY = screenRect.top();
    else if (posY + gifHeight > screenRect.bottom()) posY = screenRect.bottom() - gifHeight;

    // 设置窗口位置
    setGeometry(posX, posY, gifWidth, gifHeight);

    // 设置并播放GIF
    gifLabel->setMovie(objectionMovie);
    show();
    objectionMovie->start();
    soundEffect->play();

    // 定时关闭
    QTimer::singleShot(1600, this, [this]() {
        if (isVisible()) close();
        });
}

// 其他代码保持不变...

// 实现eventFilter函数
bool ObjectionWindow::eventFilter(QObject* watched, QEvent* event) {
    // 如果需要处理特定事件，在这里添加代码
    // 目前只是简单地传递给父类处理
    return QWidget::eventFilter(watched, event);
}



