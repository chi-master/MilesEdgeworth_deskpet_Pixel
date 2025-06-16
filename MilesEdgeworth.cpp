#include "MilesEdgeworth.h"
#include <FolderOpener.h>
#include "ObjectionWindow.h"
#include <Qdebug>
#include <QSettings>


MilesEdgeworth::MilesEdgeworth(QWidget* parent)
    : QWidget(parent)
    , objectionModeEnabled(false) // 默认关闭异议模式
{
    ui.setupUi(this);
    //gif动态标签
    setWindowTitle("咪酱");
    ui.petLabel->setScaledContents(true);
    ui.petLabel->resize(100 * scale, 100 * scale);
    petMovie = new QMovie(":/gifs/special/briefcaseIn0.gif");
    petMovie->setParent(this);
    type = MilesEdgeworth::BRIEFCASEIN;
    direction = 0;
    nowScreen = QGuiApplication::primaryScreen();
    QRect desktopRect = nowScreen->availableGeometry();
    move(desktopRect.x() - 45 * scale, desktopRect.y() + desktopRect.height() - 90 * scale);
    ui.petLabel->setMovie(petMovie);
    // 设置窗口为无边框 透明 置顶
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    // 提示: 如果设置为Qt::Tool, this->close()是不会退出程序的, 只会隐藏当前窗口. 
    // 我的解决方法是close的时候发送信号, 让main里的QApplication接收到信号时调用quit()函数退出程序.
    

    createContextMenu();            // 创建右键菜单
    createTrayIcon();               // 创建托盘图标
    setContextMenuPolicy(Qt::CustomContextMenu);    // 右键唤起菜单
    createClickTimer();             // 创建单双击专用的计时器
    createBadgeTimer();             // 创建徽章专用的计时器
    createShakeTimer();             // 创建判断是否触发害怕地震动画专用的计时器  
    petMovie->start();
    petMovie->setSpeed(110); // 
    CoInitialize(nullptr);      // 初始化COM库（专注模式调用）
    //设置（语音和大小加载）
    loadLanguageSetting();  // 语言设置加载
    loadSettings();//大小设置加载，在构造函数最后加载上次的尺寸设置

    // 创建音效播放器, 并移动到独立线程
    thread = new QThread();
    soundEffect = new QSoundEffect(this);
    soundEffect->setVolume(0.8f);
    soundEffect->moveToThread(thread);
    // 创建检察官徽章
    prosbadge = new ProsecutorBadge();
    // 连接右键菜单
    connect(this, &QWidget::customContextMenuRequested, this, &MilesEdgeworth::showContextMenu);
    // 帧切换: 走路/跑步要进行移动; 播放到最后一帧时判断是否要结束动作
    connect(petMovie, &QMovie::frameChanged, [=](int frame) {

        // 移动
        if ((type == MilesEdgeworth::RUN || type == MilesEdgeworth::BRIEFCASEIN) && !actionMove->isChecked()) {
            runMove();
        }
        else if (type == MilesEdgeworth::WALK && !actionMove->isChecked()) {
            walkMove();
        }
        // 判断是否要结束动作
        if (frame == petMovie->frameCount() - 1) { // 若此时为最后一帧
            switch (type) {
            case BRIEFCASEIN:
            {
                petMovie->setFileName(QString(":gifs/special/briefcaseStop0.gif"));
                type = MilesEdgeworth::BRIEFCASESTOP;
                direction = 0;
                break;
            }
            case BRIEFCASESTOP:
            {
                petMovie->setFileName(QString(":gifs/stand/0.gif"));
                type = MilesEdgeworth::STAND;
                direction = 0;
                break;
            }
            case MilesEdgeworth::RUN:
            case MilesEdgeworth::WALK:
            {
                // 若当前动作为跑/走, 则有二分之一的概率继续重复, 二分之一的概率停止并切换下一动作
                double randnum = QRandomGenerator::global()->generateDouble();
                if (randnum <= 0.5) {
                //petMovie->stop();
                autoChangeGif();
                }
                break;
            }
            case MilesEdgeworth::STAND:
            {
                // 若当前动作为站立, 则有0.3概率继续重复, 0.7的概率停止并切换下一动作
                double randnum = QRandomGenerator::global()->generateDouble();
                if (randnum <= 0.7) { //0.7
                    //petMovie->stop();
                    autoChangeGif();
                }
                break;
            }
            case MilesEdgeworth::TAKETHAT:
            {
                // 接持续睡觉
                petMovie->setFileName(QString(":/gifs/stand/%1.gif").arg(direction));
                type = MilesEdgeworth::TAKETHATING;
                break;
            }
            case MilesEdgeworth::ONCE:
            {
                // 若当前动作为单次动作, 则播放一遍后立即停止, 切换下一动作
                //petMovie->stop();
                autoChangeGif();
                break;
            }
            case MilesEdgeworth::SLEEP:
            {
                // 接持续睡觉
                petMovie->setFileName(QString(":/gifs/special/sleeping%1.gif").arg(direction));
                type = MilesEdgeworth::SLEEPING;
                actionSleep->setText("唤醒");
                actionSleep->setEnabled(true);
                actionTurn->setText("转向");
                actionTurn->setEnabled(true);
                break;
            }
            case MilesEdgeworth::TIE:
            {
                // 接持续被绑(测试)
                petMovie->setFileName(QString(":/gifs/special/tie%1.gif").arg(direction));
                type = MilesEdgeworth::TIEING;
                actionTie->setText("解绑");
                actionTie->setEnabled(true);
                actionTurn->setText("转向");
                actionTurn->setEnabled(true);
                break;
            }
            case MilesEdgeworth::TS:
            {

                petMovie->setFileName(QString(":/gifs/tingshen/remain%1.gif").arg(direction));
                type = MilesEdgeworth::TSING;
                actionTs->setText("降下桌子");
                actionTs->setEnabled(true);
                actionTurn->setText("转向");
                actionTurn->setEnabled(true);
                break;


            }
            case MilesEdgeworth::WEB:
            {

                petMovie->setFileName(QString(":/gifs/stand2/%1.gif").arg(direction));
                type = MilesEdgeworth::WEBING;
                actionWeb->setText("解除专注模式");
                actionWeb->setEnabled(true);
                actionTurn->setText("转向");
                actionTurn->setEnabled(true);
                break;


            }

            case MilesEdgeworth::WEBING:
            {
                // 循环, 啥也不干.
                break;
            }

            case MilesEdgeworth::TAKETHATING:
            {
                // 循环, 啥也不干.
                break;
            }
            case MilesEdgeworth::SLEEPING:
            {
                // 循环, 啥也不干.
                break;
            }

            case MilesEdgeworth::TIEING:
            {
                // 循环, 啥也不干.
                break;
            }
            case MilesEdgeworth::TEA:
            {
                // 循环, 啥也不干.
                break;
            }
            case MilesEdgeworth::TSING:
            {
                // 循环, 啥也不干.
                break;
            }
            case MilesEdgeworth::CROUCH:
            {
                // 固定在最后一帧, 直到鼠标松开才站起
                petMovie->stop();
                if (actionObjection) {
                    // ===== 修改部分：确保关闭异议模式 =====
                    objectionModeEnabled = false;  // 强制关闭异议模式
                    actionObjection->setChecked(false);  // 取消菜单项的选中状态（如果是 QAction）
                }
            }
            }  
        }
        });
    // 检察官徽章被点击时消失, 触发鞠躬动画, 停止计时器
    connect(prosbadge, &ProsecutorBadge::badgeClicked, [=]() {
        prosbadge->close();
        petMovie->stop();
        specifyChangeGif(QString(":/gifs/special/bow%1.gif").arg(direction % 2),
            MilesEdgeworth::ONCE, direction % 2);
        // 停止用于定时close的计时器
        badgeTimer->stop();
        });
    //语言设置
    // 在语言变化时保存设置
    connect(jpLanguage, &QAction::triggered, [=](bool checked) {
        if (checked) {
            language = 0;
            saveLanguageSetting(language);
        }
        });
    connect(engLanguage, &QAction::triggered, [=](bool checked) {
        if (checked) {
            language = 1;
            saveLanguageSetting(language);
        }
        });
    connect(chLanguage, &QAction::triggered, [=](bool checked) {
        if (checked) {
            language = 2;
            saveLanguageSetting(language);
        }
        });
    // 独立线程播放音频
    connect(thread, &QThread::finished, soundEffect, &QObject::deleteLater);
    connect(soundEffect, &QObject::destroyed, thread, &QThread::deleteLater);
    connect(this, &MilesEdgeworth::soundPlay, soundEffect, &QSoundEffect::play);
    connect(this, &MilesEdgeworth::soundStop, soundEffect, &QSoundEffect::stop);
    thread->start();


}



MilesEdgeworth::~MilesEdgeworth()
{
    prosbadge->deleteLater();
    if (thread->isRunning())
    {
        qDebug() << "thread is Running";
        thread->quit();
        thread->wait();
    }
}

//语音设置
void saveLanguageSetting(int lang) {
    QSettings settings("MilesEdgeworth", "DesktopPet");
    settings.setValue("language", lang);
}

// 保存语言设置
void MilesEdgeworth::saveLanguageSetting(int lang)
{
    QSettings settings("MilesEdgeworth", "DesktopPet");
    settings.setValue("language", lang);
}


// 加载语言设置
void MilesEdgeworth::loadLanguageSetting()
{
    QSettings settings("MilesEdgeworth", "DesktopPet");
    language = settings.value("language", 0).toInt(); // 默认日语

    // 根据加载的语言更新UI勾选状态
    switch (language) {
    case 0: jpLanguage->setChecked(true); break;
    case 1: engLanguage->setChecked(true); break;
    case 2: chLanguage->setChecked(true); break;
    }
}

// 大小设置
// 修改applyScale函数，确保尺寸变化后位置也相应调整
// 修改applyScale函数，同步更新当前帧
// 修改点2：同步窗口尺寸逻辑
void MilesEdgeworth::applyScale(double scaleValue, QAction* action)
{
    scale = scaleValue;
    const QSize newSize(100 * scale, 100 * scale);

    // 同步调整（关键修改）
    this->resize(newSize);          // [+1行] 先改窗口
    ui.petLabel->resize(newSize);   // 原有代码



    saveSettings();
}
// 保存大小设置函数
void MilesEdgeworth::saveSettings()
{
    QSettings settings("MilesEdgeworth", "DesktopPet");
    settings.setValue("scale", scale);
}
// 加载设置函数
// 修改loadSettings函数，使用默认位置
void MilesEdgeworth::loadSettings() {
    QSettings settings("MilesEdgeworth", "DesktopPet");
    double savedScale = settings.value("scale", MEDIAN).toDouble();  // 默认为中等尺寸

    // 根据加载的尺寸值选择对应的菜单项
    if (qFuzzyCompare(savedScale, MINI)) {
        miniSize->setChecked(true);
        scale = MINI;
    }
    else if (qFuzzyCompare(savedScale, SMALL)) {
        smallSize->setChecked(true);
        scale = SMALL;
    }
    else if (qFuzzyCompare(savedScale, BIG)) {
        bigSize->setChecked(true);
        scale = BIG;
    }
    else {
        // 默认为中等尺寸或者当保存的值不匹配任何预设值时
        medianSize->setChecked(true);
        scale = MEDIAN;
    }
    // 调整大小
    ui.petLabel->resize(100 * scale, 100 * scale);
    // 使用默认位置
    setToDefaultPosition();
}

// 重写窗口移动事件. 用于限制窗口位置
void MilesEdgeworth::moveEvent(QMoveEvent* event)
{
    if (type == MilesEdgeworth::BRIEFCASEIN) return;
    if (QGuiApplication::screenAt(event->pos() + QPoint(50 * scale, 50 * scale)) != nullptr) {
        // 如果咪酱的中心点没被移出屏幕, 则更新咪酱的所在屏幕
        nowScreen = QGuiApplication::screenAt(event->pos() + QPoint(50 * scale, 50 * scale));
    }
    //QRect nowScreenRect = QGuiApplication::primaryScreen()->availableGeometry();
    QPoint newPos = event->pos();
    // 咪酱此时的四角坐标
    QPoint topLeft = newPos + QPoint(36 * scale, 10 * scale);
    QPoint topRight = newPos + QPoint(63 * scale, 10 * scale);
    QPoint bottomLeft = newPos + QPoint(36 * scale, 90 * scale);
    QPoint bottomRight = newPos + QPoint(63 * scale, 90 * scale);

    if (QGuiApplication::screenAt(topLeft) == nullptr || QGuiApplication::screenAt(topRight) == nullptr
        || QGuiApplication::screenAt(bottomLeft) == nullptr || QGuiApplication::screenAt(bottomRight) == nullptr) {
        // 只要有一个角出了屏幕,那么这次的move就可能要被限制
        // 窗口topLeft点的坐标限制
        int xMin;
        int xMax;
        int yMin;
        int yMax;
        QRect nowScreenRect = nowScreen->geometry();
        // 单屏时是四条边都做限制.
        xMax = nowScreenRect.x() + nowScreenRect.width() - 63 * scale;
        xMin = nowScreenRect.x() - 36 * scale;
        yMax = nowScreenRect.y() + nowScreenRect.height() - 90 * scale;
        yMin = nowScreenRect.y() - 10 * scale;
        if (leftPrimary->isChecked()) {
            // 如果主屏幕在左侧, 则不限制主屏幕右边界和第二屏幕左边界, 而是限制主屏幕的左边界和第二屏幕的右边界
            // 如果是在主屏幕内, 则有xMin但没有xMax
            if (QGuiApplication::screens().indexOf(nowScreen) == 0) {
                // 如果是在主屏幕, 则有xMin但没有xMax
                xMax = 1000000000;
            }
            else if (QGuiApplication::screens().indexOf(nowScreen) == 1) {
                // 如果是在第二屏幕, 则有xMax但没有xMin
                xMin = -1000000000;
            }
        }
        else if (rightPrimary->isChecked()) {
            // 如果主屏幕在右侧, 则不限制主屏幕左边界和第二屏幕的右边界, 而是限制主屏幕的右边界和第二屏幕的左边界
            // 如果是在主屏幕内, 则有xMax但没有xMin
            if (QGuiApplication::screens().indexOf(nowScreen) == 0) {
                // 如果是在主屏幕, 则有xMax但没有xMin
                xMin = -1000000000;
            }
            else if (QGuiApplication::screens().indexOf(nowScreen) == 1) {
                // 如果是在第二屏幕, 则有xMin但没有xMax
                xMax = 1000000000;
            }
        }

        // 限制左上角位置
        newPos.setX(qMax(xMin, newPos.x()));
        newPos.setY(qMax(yMin, newPos.y()));

        // 限制右下角位置
        newPos.setX(qMin(xMax, newPos.x()));
        newPos.setY(qMin(yMax, newPos.y()));

        move(newPos);
    }
}


void MilesEdgeworth::mouseMoveEvent(QMouseEvent* event)
{
    if (type == MilesEdgeworth::BRIEFCASEIN) return;
    if (event->buttons() & Qt::LeftButton)
    {
        //此段用于处理拖拽移动
        if (m_dragging)
        {
            //delta 相对偏移量
            QPoint delta = event->globalPosition().toPoint() - m_startPosition;
            //新位置：窗体原始位置+偏移量
            move(m_framePosition + delta);
        }
        // 此段用于判断是否触发害怕地震动画
        if (type != MilesEdgeworth::SLEEP && type != MilesEdgeworth::SLEEPING && type != MilesEdgeworth::TIE && type != MilesEdgeworth::TIEING
            && (event->globalPosition().x() - shakeX) * shakeDirect < 0) {
            // 如果移动的方向改变, 则计数+1, 记录方向和横坐标
            shakeCount++;
            shakeDirect = -shakeDirect;
            shakeX = event->globalPosition().x();
            if (shakeCount == 5 && shakeTimer->isActive()) {
                // 计数达到五次时, 触发害怕地震动画, 计时器停止, 计数清零
                petMovie->stop();
                specifyChangeGif(QString(":gifs/special/crouch%1.gif").arg(direction % 2),
                    MilesEdgeworth::CROUCH, direction % 2);
                shakeTimer->stop();
                shakeCount = 0;
            }
        }
    }
    QWidget::mouseMoveEvent(event);
}

void MilesEdgeworth::mousePressEvent(QMouseEvent* event)
{

    if (type == MilesEdgeworth::BRIEFCASEIN) return;
    //响应左键
    if (event->button() == Qt::LeftButton)
    {
        // 此段用于处理拖拽移动
        m_dragging = true;
        m_startPosition = event->globalPosition().toPoint();
        m_framePosition = frameGeometry().topLeft();
        // 此段用于处理单击双击判断
        if (clickTimer->isActive()) {
            // 如果计时器正在运作,则说明这次press是第二次
            doubleClick = true;
            clickTimer->stop();         // 第二次press后关闭计时器
        }
        // 此段用于判断是否触发害怕地震动作
        if (type != MilesEdgeworth::SLEEP && type != MilesEdgeworth::SLEEPING) {
            shakeX = event->globalPosition().x();
            shakeCount = 0;
            shakeTimer->start();
        }
    }
    QWidget::mousePressEvent(event);
}



void MilesEdgeworth::mouseReleaseEvent(QMouseEvent* event)
{
    if (type == MilesEdgeworth::BRIEFCASEIN) return;
    // 此段用于处理拖拽移动
    m_dragging = false;
    // 此段用于处理单双击判断
    if (event->button() == Qt::LeftButton && 
        event->globalPosition().toPoint() == m_startPosition) {
        // 如果release时与press时鼠标位置相同, 则不是拖拽, 预备进行单双击
        if (objectionModeEnabled) {
            return;
        }
        if (doubleClick) {
            // 若这是第二次点击的release, 则执行双击事件
            doubleClickEvent();
            doubleClick = false;
        }
        else {
            // 否则这是第一次点击的release, 开启计时器
            m_pressPos = event->position();
            clickTimer->start();
        }
    }
    // 此段用于判断是否触发害怕地震动作+bug重置,将所有按钮恢复
    if (type != MilesEdgeworth::SLEEP && type != MilesEdgeworth::SLEEPING&& type != MilesEdgeworth::SLEEPING) {
        shakeTimer->stop();
        shakeCount = 0;
        if (type == MilesEdgeworth::CROUCH) {
            // 如果处于蹲下状态, 则切换站起动作.
            if (petMovie->state() == QMovie::NotRunning) {
                // 如果已经蹲下, 则切换完整的站起
                specifyChangeGif(QString(":gifs/special/standup%1.gif").arg(direction),
                    MilesEdgeworth::ONCE, direction);
                actionWeb->setEnabled(true);
                actionWeb->setText("专注模式");
                actionTea->setEnabled(true);
                actionTeaing->setEnabled(true);
                actionTie->setEnabled(true);
                actionTie->setText("束缚");
                actionSleep->setEnabled(true);
                actionSleep->setText("睡觉");
                actionTurn->setEnabled(true);
                actionShowProsBadge->setEnabled(true);
                actionTs->setEnabled(true);
                actionTs->setText("升起桌子");
                update();
            }
            else {
                // 如果还没蹲下, 则切换惊吓恢复站立
                petMovie->stop();
                specifyChangeGif(QString(":gifs/special/standup%1.gif").arg(direction + 2),
                    MilesEdgeworth::ONCE, direction);
                actionWeb->setEnabled(true);
                actionWeb->setText("专注模式");
                actionTea->setEnabled(true);
                actionTeaing->setEnabled(true);
                actionTie->setEnabled(true);
                actionTie->setText("束缚");
                actionSleep->setEnabled(true);
                actionSleep->setText("睡觉");
                actionTurn->setEnabled(true);
                actionShowProsBadge->setEnabled(true);
                actionTs->setEnabled(true);
                actionTs->setText("升起桌子");
                update();
            }

        }
    }

    QWidget::mouseReleaseEvent(event);
}
void MilesEdgeworth::closeEvent(QCloseEvent* event)
{
    if (!isClosing) {
        event->ignore();
        startExitSequence();
    }
    else {
        // delete所有picViewer
        if (firstViewer != nullptr) {
            while (firstViewer->next != nullptr) {
                PicViewer* temp = firstViewer;
                firstViewer = temp->next;
                firstViewer->prev = nullptr;
                temp->deleteLater();
            }
            firstViewer->deleteLater();
            firstViewer = nullptr; // 清空指针
        }
        emit exitProgram();
        event->accept(); // 明确接受关闭事件
    }
}

void MilesEdgeworth::enterEvent(QEnterEvent* event)
{
    setCursor(Qt::PointingHandCursor);
}

// 创建右键菜单
void MilesEdgeworth::createContextMenu()
{
    menu = new QMenu(this);
    sizeMenu = menu->addMenu("调整大小");
    languageMenu = menu->addMenu("语音语言");
    screenMenu = menu->addMenu("双屏选项");
    menu->addSeparator();
    actionTop = menu->addAction("置于顶层");
    actionTop->setCheckable(true);  // 设置action可选中
    actionTop->setChecked(true);    // 默认选中
    actionMove = menu->addAction("禁止走动");
    actionMove->setCheckable(true);  // 设置action可选中
    actionMute = menu->addAction("静音");
    actionMute->setCheckable(true);  // 设置action可选中
    actionTurn = menu->addAction("转向");
    actionMute->setCheckable(true);  // 设置action可选中
    menu->addSeparator();
    actionTea = menu->addAction("喂食红茶");
    actionTeaing = menu->addAction("持续喝茶");
    actionShowProsBadge = menu->addAction("飞徽章");
    actionSleep = menu->addAction("睡觉");
    actionTie = menu->addAction("束缚");
    menu->addSeparator();
    actionTs = menu->addAction("升起桌子");
    actionPic = menu->addAction("图片置顶查看器");
    actionWeb = menu->addAction("专注模式");
    actionObjection = menu->addAction("异议模式");
    actionObjection->setCheckable(true);  // 设置action可选中
    actionObjection->setChecked(false);   // 默认异议模式不选中
    connect(actionObjection, &QAction::triggered, this, &MilesEdgeworth::toggleObjectionMode);//异议模式按钮链接
    menu->addSeparator();
    actionExit = menu->addAction("退出");



    // 子菜单sizeMenu选项
    sizeGroup = new QActionGroup(sizeMenu);
    sizeGroup->setExclusive(true);  // 设置为单选

    miniSize = sizeMenu->addAction("迷你");
    miniSize->setCheckable(true);    // 设置action可选中
    sizeGroup->addAction(miniSize);  // 加入action组

    smallSize = sizeMenu->addAction("小");
    smallSize->setCheckable(true);    // 设置action可选中
    sizeGroup->addAction(smallSize);  // 加入action组

    medianSize = sizeMenu->addAction("中");
    medianSize->setCheckable(true);    // 设置action可选中
    medianSize->setChecked(true);      // 默认选中小
    sizeGroup->addAction(medianSize);  // 加入action组

    bigSize = sizeMenu->addAction("大");
    bigSize->setCheckable(true);    // 设置action可选中
    sizeGroup->addAction(bigSize);  // 加入action组

    // 子菜单languageMenu选项
    lanGroup = new QActionGroup(languageMenu);
    lanGroup->setExclusive(true);   // 设置为单选

    jpLanguage = languageMenu->addAction("日语");
    jpLanguage->setCheckable(true);    // 设置action可选中
    jpLanguage->setChecked(true);      // 默认选中日语
    lanGroup->addAction(jpLanguage);  // 加入action组

    engLanguage = languageMenu->addAction("英语");
    engLanguage->setCheckable(true);    // 设置action可选中
    lanGroup->addAction(engLanguage);  // 加入action组

    chLanguage = languageMenu->addAction("汉语");
    chLanguage->setCheckable(true);    // 设置action可选中
    lanGroup->addAction(chLanguage);  // 加入action组

    // 子菜单screenMenu选项
    screenGroup = new QActionGroup(screenMenu);
    screenGroup->setExclusive(true);    // 设置为单选

    singleScreen = screenMenu->addAction("单屏");
    leftPrimary = screenMenu->addAction("主屏幕在左侧");
    rightPrimary = screenMenu->addAction("主屏幕在右侧");
    singleScreen->setCheckable(true);
    singleScreen->setChecked(true);     // 默认选中单屏
    if (QGuiApplication::screens().length() == 1) {
        leftPrimary->setDisabled(true);
        rightPrimary->setDisabled(true);
    }
    else {
        leftPrimary->setCheckable(true);
        rightPrimary->setCheckable(true);
    }
    screenGroup->addAction(singleScreen);
    screenGroup->addAction(leftPrimary);
    screenGroup->addAction(rightPrimary);

    // 大小设置添加保存设置功能
    connect(miniSize, &QAction::triggered, [=](bool checked) {
        if (checked) {
            applyScale(MINI, miniSize);
        }
        });

    connect(smallSize, &QAction::triggered, [=](bool checked) {
        if (checked) {
            applyScale(SMALL, smallSize);
        }
        });

    connect(medianSize, &QAction::triggered, [=](bool checked) {
        if (checked) {
            applyScale(MEDIAN, medianSize);
        }
        });

    connect(bigSize, &QAction::triggered, [=](bool checked) {
        if (checked) {
            applyScale(BIG, bigSize);
        }
        });



    // 置顶
    connect(actionTop, &QAction::triggered, [=](bool checked) {
        if (checked) {
            setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
            this->show();
        }
        else {
            setWindowFlags(this->windowFlags() & ~Qt::WindowStaysOnTopHint);
            this->show();
        }
        });
    //静音
    connect(actionMute, &QAction::triggered, [=](bool checked) {
        // 1. 控制主窗口音效
        soundEffect->setVolume(checked ? 0 : 0.8f);

        // 2. 控制副窗口音效（直接同步状态）
        ObjectionWindow::syncMuteState(checked);
        });
    //转向，各种情况下的转向处理
    connect(actionTurn, &QAction::triggered, [=](bool checked) {
        // 计算新方向
        int newDirection;
        if (direction % 2 == 1) { // 奇数方向(1,3,5,7)减1
            newDirection = (direction - 1 + 8) % 8; // +8确保不出现负数
        }
        else { // 偶数方向(0,2,4,6)加1
            newDirection = (direction + 1) % 8;
        }
        if (type == MilesEdgeworth::WEB) {
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand/%1.gif").arg(direction),
                MilesEdgeworth::STAND, newDirection);
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand2/%1.gif").arg(newDirection % 2),
                MilesEdgeworth::WEBING, newDirection);
        }
        else if (type == MilesEdgeworth::WEBING) {
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand/%1.gif").arg(direction),
                MilesEdgeworth::STAND, newDirection);
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand2/%1.gif").arg(newDirection % 2),
                MilesEdgeworth::WEBING, newDirection);
        }
        else if (type == MilesEdgeworth::TEA) {
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand/%1.gif").arg(direction),
                MilesEdgeworth::STAND, newDirection);
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/special/teaing%1.gif").arg(newDirection % 2),
                MilesEdgeworth::TEA, newDirection);
        }
        else if (type == MilesEdgeworth::TS) {
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand/%1.gif").arg(direction),
                MilesEdgeworth::STAND, newDirection);
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/tingshen/remain%1.gif").arg(newDirection % 2),
                MilesEdgeworth::TSING, newDirection);
        }
        else if (type == MilesEdgeworth::TSING) {
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand/%1.gif").arg(direction),
                MilesEdgeworth::STAND, newDirection);
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/tingshen/remain%1.gif").arg(newDirection % 2),
                MilesEdgeworth::TSING, newDirection);
        }
        else if (type == MilesEdgeworth::TIEING) {
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand/%1.gif").arg(direction),
                MilesEdgeworth::STAND, newDirection);
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/special/tie%1.gif").arg(newDirection % 2),
                MilesEdgeworth::TIEING, newDirection);
        }
        else if (type == MilesEdgeworth::TIE) {
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand/%1.gif").arg(direction),
                MilesEdgeworth::STAND, newDirection);
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/special/tie%1.gif").arg(newDirection % 2),
                MilesEdgeworth::TIEING, newDirection);
        }
        else if (type == MilesEdgeworth::SLEEPING) {
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand/%1.gif").arg(direction),
                MilesEdgeworth::STAND, newDirection);
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/special/sleeping%1.gif").arg(newDirection % 2),
                MilesEdgeworth::SLEEPING, newDirection);
        }
        else if (type == MilesEdgeworth::RUN) {
            petMovie->stop();
            // 使用跑步专用的转向动画
            specifyChangeGif(QString(":/gifs/special/turn%1.gif").arg(direction),
                MilesEdgeworth::ONCE, newDirection);
            // 转向后继续跑步
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/run/%1.gif").arg(newDirection),
                MilesEdgeworth::RUN, newDirection);
        }
        else if (type == MilesEdgeworth::WALK) {
            petMovie->stop();
            // 使用走路专用的转向动画
            specifyChangeGif(QString(":/gifs/special/turn%1.gif").arg(direction),
                MilesEdgeworth::ONCE, newDirection);
            // 转向后继续走路
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/walk/%1.gif").arg(newDirection),
                MilesEdgeworth::WALK, newDirection);
        }
        else {
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/special/turn%1.gif").arg(direction),
                MilesEdgeworth::ONCE, newDirection);
        }
        });

    //庭审模式，升起桌子
    connect(actionTs, &QAction::triggered, [=](bool checked) {
        if (type == MilesEdgeworth::TSING) {
            // 解除
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/tingshen/end%1.gif").arg(direction % 2),
                MilesEdgeworth::ONCE, direction);
            actionTs->setText("升起桌子");
            actionTea->setEnabled(true);
            actionTeaing->setEnabled(true);
            actionTie->setEnabled(true);
            actionSleep->setEnabled(true);
            actionShowProsBadge->setEnabled(true);
        }
        else {
            petMovie->stop();
            int next_direct = direction % 2;
            specifyChangeGif(QString(":/gifs/tingshen/start%1.gif").arg(next_direct),
                MilesEdgeworth::TS, next_direct);
                actionTs->setDisabled(true);  // 唤醒时解除禁用
                actionTurn->setDisabled(true);  // 唤醒时解除禁用
                actionSleep->setDisabled(true);
                actionTie->setDisabled(true);
                actionTea->setDisabled(true);
                actionTeaing->setDisabled(true);
                actionShowProsBadge->setDisabled(true);
        }
        });
    //专注模式
    connect(actionWeb, &QAction::triggered, [=](bool checked) {
        if (type == MilesEdgeworth::WEBING) {
            // 解除
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/special/bowb%1.gif").arg(direction % 2),
                MilesEdgeworth::ONCE, direction);
            actionWeb->setText("专注模式");
            actionTea->setEnabled(true);
            actionTeaing->setEnabled(true);
            actionTie->setEnabled(true);
            actionSleep->setEnabled(true);
            actionShowProsBadge->setEnabled(true);
        }
        else {
            petMovie->stop();
            int next_direct = direction % 2;
            specifyChangeGif(QString(":/gifs/special/think%1.gif").arg(next_direct),
                MilesEdgeworth::WEB, next_direct);
            // 将action更名为"解绑"
            // 在睡觉动画播放完之前, 两个按钮要被禁用
            actionWeb->setDisabled(true); // 解除禁用在frameChanged的connect里, SLEEP最后一帧时解除禁用
            actionTea->setDisabled(true);  // 唤醒时解除禁用
            actionTeaing->setDisabled(true);  // 唤醒时解除禁用
            actionTurn->setDisabled(true);  // 唤醒时解除禁用
            actionSleep->setDisabled(true);
            actionShowProsBadge->setDisabled(true);
        }
        });
    // 喂食红茶
    connect(actionTea, &QAction::triggered, [=](bool checked) {
        petMovie->stop();
        int next_direct = direction % 2;
        specifyChangeGif(QString(":/gifs/special/tea%1.gif").arg(next_direct),
            MilesEdgeworth::ONCE, next_direct);
        });
    connect(actionTeaing, &QAction::triggered, [=](bool checked) {
        petMovie->stop();
        int next_direct = direction % 2;
        specifyChangeGif(QString(":/gifs/special/teaing%1.gif").arg(next_direct),
            MilesEdgeworth::TEA, next_direct);
        });
    //飞徽章
connect(actionShowProsBadge, &QAction::triggered, [=](bool checked) {

    petMovie->stop();
    // 先播放新的gif动画
    QString preGifFilename = QString(":/gifs/special/prepare%1.gif").arg(direction % 2);
    actionWeb->setDisabled(true);//置灰防止在飞徽章期间按其他按钮导致卡死
    actionTurn->setDisabled(true);
    actionSleep->setDisabled(true);
    actionTie->setDisabled(true);
    actionTea->setDisabled(true);
    actionTeaing->setDisabled(true);
    actionShowProsBadge->setDisabled(true);
    Type pre_type = MilesEdgeworth::TAKETHAT;

    specifyChangeGif(preGifFilename, pre_type, direction);
    // 在前置动画完成后执行原来的徽章动画序列
    QTimer::singleShot(3100, this, [=]() { // 前置动画需要3秒
        petMovie->stop();
    // 设置动画文件名和类型
        QString filename;
    if (language == 0) {
        filename = QString(":/gifs/objection/kanzhaoa%1.gif").arg(direction % 2);
    }
    else if (language == 1) {
        filename = QString(":/gifs/objection/kanzhaob%1.gif").arg(direction % 2);
    }
    else { // language == 2
        filename = QString(":/gifs/objection/kanzhaoc%1.gif").arg(direction % 2);
    }
    Type next_type = MilesEdgeworth::TAKETHAT;
    soundEffect->setSource(QUrl::fromLocalFile(QString(":/audios/takethat%1.wav").arg(language)));//音效
    specifyChangeGif(filename, next_type, direction);
    emit soundPlay();    
    // 显示检察官徽章
    this->showProsBadge(); // 
    });
    QTimer::singleShot(7500, this, [=]() { 
        actionWeb->setEnabled(true);
        actionTea->setEnabled(true);//恢复按钮启用
        actionTeaing->setEnabled(true);//恢复按钮启用
        actionTie->setEnabled(true);
        actionSleep->setEnabled(true);
        actionTurn->setEnabled(true);
        actionShowProsBadge->setEnabled(true);
        });
});
    // 睡觉
    connect(actionSleep, &QAction::triggered, [=](bool checked) {
        if (type == MilesEdgeworth::SLEEPING) {
            // 唤醒
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/special/wake%1.gif").arg(direction % 2),
                MilesEdgeworth::ONCE, direction);
            actionSleep->setText("睡觉");
            actionTea->setEnabled(true);
            actionTeaing->setEnabled(true);
            actionTie->setEnabled(true);
            actionShowProsBadge->setEnabled(true);
        }
        else {
            petMovie->stop();
            int next_direct = direction % 2;
            specifyChangeGif(QString(":/gifs/special/sleep%1.gif").arg(next_direct),
                MilesEdgeworth::SLEEP, next_direct);
            // 将action更名为"唤醒"
            // 在睡觉动画播放完之前, 两个按钮要被禁用
            actionSleep->setDisabled(true); // 解除禁用在frameChanged的connect里, SLEEP最后一帧时解除禁用
            actionTea->setDisabled(true);  // 唤醒时解除禁用
            actionTeaing->setDisabled(true);  // 唤醒时解除禁用
            actionTurn->setDisabled(true);  // 唤醒时解除禁用
            actionTie->setDisabled(true);
            actionShowProsBadge->setDisabled(true);
        }
        });
    // 束缚
    connect(actionTie, &QAction::triggered, [=](bool checked) {
        if (type == MilesEdgeworth::TIEING) {
            // 解绑，改为站立动作
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand/%1.gif").arg(direction % 2),
                MilesEdgeworth::STAND, direction % 2);
            actionSleep->setEnabled(true);
            actionTie->setText("束缚");
            actionTea->setEnabled(true);
            actionTeaing->setEnabled(true);
            actionShowProsBadge->setEnabled(true);
        }

        else {
            petMovie->stop();
            int next_direct = direction % 2;
            specifyChangeGif(QString(":/gifs/special/tied%1.gif").arg(next_direct),
                MilesEdgeworth::TIE, next_direct);
            // 将action更名为"解绑"
            // 在睡觉动画播放完之前, 两个按钮要被禁用
            actionTie->setDisabled(true); // 解除禁用在frameChanged的connect里, SLEEP最后一帧时解除禁用
            actionTea->setDisabled(true);  // 唤醒时解除禁用
            actionTeaing->setDisabled(true);  // 唤醒时解除禁用
            actionTurn->setDisabled(true);  // 唤醒时解除禁用
            actionSleep->setDisabled(true);
            actionShowProsBadge->setDisabled(true);
        }

        });
    // 置顶图查看器
    connect(actionPic, &QAction::triggered, this, &MilesEdgeworth::createPicViewer);
    // 退出: 要关掉所有参考图窗口, 彻底退出程序. 
    connect(actionExit, &QAction::triggered, this, &QWidget::close);
}
// 创建右下角托盘图标
void MilesEdgeworth::createTrayIcon()
{
    tray = new QSystemTrayIcon(this);
    tray->setVisible(true);
    tray->setIcon(QIcon(":/icon/favicon_bar.ico"));
    QMenu* menu = new QMenu(this);
    menu->addAction(actionExit);
    tray->setContextMenu(menu);
    tray->setToolTip(windowTitle());
    tray->show();

    connect(tray, &QSystemTrayIcon::activated, this, &MilesEdgeworth::on_trayActivated);
}
// 创建区分单双击用的计时器
void MilesEdgeworth::createClickTimer()
{
    clickTimer = new QTimer(this);
    clickTimer->setSingleShot(true);
    clickTimer->setInterval(300);   // 300ms内算双击
    connect(clickTimer, &QTimer::timeout, [=]() {
        // 执行单击事件
        singleClickEvent();
        });
}
// 创建检察官徽章用的计时器
void MilesEdgeworth::createBadgeTimer()
{
    badgeTimer = new QTimer(this);
    badgeTimer->setSingleShot(true);
    badgeTimer->setInterval(FLYINGTIME);
    connect(badgeTimer, &QTimer::timeout, [=]() {
        // 超时未点击徽章, 徽章消失并且触发捡徽章动画
        prosbadge->close();
        petMovie->stop();
        specifyChangeGif(QString(":/gifs/special/pickup%1.gif").arg(direction),
            MilesEdgeworth::ONCE, direction);
        });
}
void MilesEdgeworth::createShakeTimer()
{
    shakeTimer = new QTimer(this);
    shakeTimer->setInterval(1000);  
    connect(shakeTimer, &QTimer::timeout, [=]() {
        // 1s内移动方向更换不足5次则重新计数
        shakeCount = 0;
        });
}
void MilesEdgeworth::createPicViewer()
{
    if (numViewer == MAXVIEWER) return;
    if (firstViewer == nullptr) {
        // 当前没有窗口
        firstViewer = new PicViewer();
        firstViewer->next = nullptr;
        firstViewer->prev = nullptr;
        connect(firstViewer, &PicViewer::isClosing, this, &MilesEdgeworth::on_viewerClosing);
        firstViewer->show();
    }
    else {
        // 当前已有窗口. 头插法加入新窗口
        PicViewer* temp = new PicViewer();
        temp->next = firstViewer->next;
        temp->prev = firstViewer;
        firstViewer->next = temp;
        if(temp->next != nullptr) temp->next->prev = temp;
        connect(temp, &PicViewer::isClosing, this, &MilesEdgeworth::on_viewerClosing);
        temp->show();
    }
    numViewer++;
    if (numViewer == MAXVIEWER) actionPic->setDisabled(true);   // 按钮置灰
}


// 显示右键菜单
void MilesEdgeworth::showContextMenu()
{
    menu->exec(QCursor::pos());
}


// 切换为指定的Gif. 参数为: 下一动作文件名, 下一动作类型, 下一动作方向
inline void MilesEdgeworth::specifyChangeGif(const QString& filename, Type next_type, int next_direct)
{
    type = next_type;
    direction = next_direct;
    petMovie->setFileName(filename);
    petMovie->start();
}

// 处理跑步时的窗口移动
// 处理跑步时的窗口移动
void MilesEdgeworth::runMove()
{
    int unit = 4;
    QPoint delta = QPoint(0, 0);
    switch (direction) {
    case 0:
    {
        // 向右走
        delta = (QPointF(unit, 0) * scale * 1.4).toPoint();
        break;
    }
    case 1:
    {
        // 向左走
        delta = (QPointF(-unit, 0) * scale * 1.4).toPoint();
        break;
    }
    case 2:
    {
        // 向右上走
        delta = (QPointF(unit, -unit) * scale).toPoint();
        break;
    }
    case 3:
    {
        // 向左上走
        delta = (QPointF(-unit, -unit) * scale).toPoint();
        break;
    }
    case 4:
    {
        // 向右下走
        delta = (QPointF(unit, unit) * scale).toPoint();
        break;
    }
    case 5:
    {
        // 向左下走
        delta = (QPointF(-unit, unit) * scale).toPoint();
        break;
    }
    case 6:
    {
        // 向上走
        delta = (QPointF(0, -unit) * scale).toPoint();
        break;
    }
    case 7:
    {
        // 向下走
        delta = (QPointF(0, unit) * scale).toPoint();
        break;
    }
    }
    if (leftPrimary->isChecked() && QGuiApplication::screens().at(1) != nullptr) {
        int primaryXMax = QGuiApplication::screens().at(0)->geometry().x() + QGuiApplication::screens().at(0)->geometry().width();
        int secondXMin = QGuiApplication::screens().at(1)->geometry().x();
        int newPosX = pos().x() + delta.x();
        if (pos().x() + 50 * scale <= primaryXMax && newPosX + 50 * scale > primaryXMax) {
            // 此时为从主屏幕跨屏到第二屏幕的瞬间
            newPosX = secondXMin - 40 * scale;
        }
        else if (pos().x() + 50 * scale >= secondXMin && newPosX + 50 * scale < secondXMin) {
            // 此时为从第二屏幕跨屏到第一屏幕的瞬间
            newPosX = primaryXMax - 60 * scale;
        }
        int newPosY = pos().y() + delta.y();
        move(newPosX, newPosY);
    }
    else if (rightPrimary->isChecked() && QGuiApplication::screens().at(1) != nullptr) {
        // 主屏幕在右侧时, 跨屏时要注意坐标
        int secondXMax = QGuiApplication::screens().at(1)->geometry().x() + QGuiApplication::screens().at(1)->geometry().width();
        int primaryXMin = QGuiApplication::screens().at(0)->geometry().x();
        int newPosX = pos().x() + delta.x();
        if (pos().x() + 50 * scale >= primaryXMin && newPosX + 50 * scale < primaryXMin) {
            // 此时为从主屏幕跨屏到第二屏幕的瞬间
            newPosX = secondXMax - 60 * scale;
            qDebug() << "此时为从主屏幕跨屏到第二屏幕的瞬间, newPosX: " << newPosX;
        }
        else if (pos().x() + 50 * scale <= secondXMax && newPosX + 50 * scale > secondXMax) {
            // 此时为从第二屏幕跨屏到第一屏幕的瞬间
            newPosX = primaryXMin - 40 * scale;
            qDebug() << "此时为第二屏幕跨屏主屏幕的瞬间, newPosX: " << newPosX;
        }
        int newPosY = pos().y() + delta.y();
        move(newPosX, newPosY);
    }
    else {
        // 单屏或者主屏幕在左侧时, 正常move就行
        move(pos() + delta);
    }
}

// 处理走路时的窗口移动
void MilesEdgeworth::walkMove()
{
    int unit = 3;
    QPoint delta = QPoint(0, 0);
    switch (direction) {
    case 0:
    {
        // 向右走
        delta = (QPointF(unit, 0) * scale * 1.4).toPoint();
        break;
    }
    case 1:
    {
        // 向左走
        delta = (QPointF(-unit, 0) * scale * 1.4).toPoint();
        break;
    }
    case 2:
    {
        // 向右上走
        delta = (QPointF(unit, -unit) * scale).toPoint();
        break;
    }
    case 3:
    {
        // 向左上走
        delta = (QPointF(-unit, -unit) * scale).toPoint();
        break;
    }
    case 4:
    {
        // 向右下走
        delta = (QPointF(unit, unit) * scale).toPoint();
        break;
    }
    case 5:
    {
        // 向左下走
        delta = (QPointF(-unit, unit) * scale).toPoint();
        break;
    }
    case 6:
    {
        // 向上走
        delta = (QPointF(0, -unit) * scale).toPoint();
        break;
    }
    case 7:
    {
        // 向下走
        delta = (QPointF(0, unit) * scale).toPoint();
        break;
    }
    }   
    if (leftPrimary->isChecked() && QGuiApplication::screens().at(1) != nullptr) {
        int primaryXMax = QGuiApplication::primaryScreen()->geometry().x() + QGuiApplication::primaryScreen()->geometry().width();
        int secondXMin = QGuiApplication::screens().at(1)->geometry().x();
        int newPosX = pos().x() + delta.x();
        if (pos().x() + 50 * scale <= primaryXMax && newPosX + 50 * scale > primaryXMax) {
            // 此时为从主屏幕跨屏到第二屏幕的瞬间
            newPosX = secondXMin - 50 * scale;
        }
        else if (pos().x() + 50 * scale >= secondXMin && newPosX + 50 * scale < secondXMin) {
            // 此时为从第二屏幕跨屏到第一屏幕的瞬间
            newPosX = primaryXMax - 50 * scale;
        }
        int newPosY = pos().y() + delta.y();
        move(newPosX, newPosY);
    }
    else if (rightPrimary->isChecked() && QGuiApplication::screens().at(1) != nullptr) {
        // 主屏幕在右侧时, 跨屏时要注意坐标
        int secondXMax = QGuiApplication::screens().at(1)->geometry().x() + QGuiApplication::screens().at(1)->geometry().width();
        int primaryXMin = QGuiApplication::screens().at(0)->geometry().x();
        int newPosX = pos().x() + delta.x();
        if (pos().x() + 50 * scale >= primaryXMin && newPosX + 50 * scale < primaryXMin) {
            // 此时为从主屏幕跨屏到第二屏幕的瞬间
            newPosX = secondXMax - 60 * scale;
            qDebug() << "此时为从主屏幕跨屏到第二屏幕的瞬间, newPosX: " << newPosX;
        }
        else if (pos().x() + 50 * scale <= secondXMax && newPosX + 50 * scale > secondXMax) {
            // 此时为从第二屏幕跨屏到第一屏幕的瞬间
            newPosX = primaryXMin - 40 * scale;
            qDebug() << "此时为第二屏幕跨屏主屏幕的瞬间, newPosX: " << newPosX;
        }
        int newPosY = pos().y() + delta.y();
        move(newPosX, newPosY);
    }
    else {
        // 单屏或者主屏幕在左侧时, 正常move就行
        move(pos() + delta);
    }
}


// 显示检察官徽章
void MilesEdgeworth::showProsBadge()
{
    QTimer::singleShot(700, [=](){ 
        if (type == MilesEdgeworth::TAKETHAT) {
            prosbadge->setScale(scale);
            if (direction == 0) {
                // 向右时, 设置初始位置
                prosbadge->setGeometry(this->geometry().x() + 86 * scale, this->geometry().y() + 16 * scale, 94, 94);
            }
            else {
                // 向左时, 设置初始位置
                prosbadge->setGeometry(this->geometry().x() + 1 * scale, this->geometry().y() + 16 * scale, 94, 94);
            }
            prosbadge->show();
            prosbadge->moveAnimation(direction);
            badgeTimer->start();
        }
        }); 
}


// 处理双击事件
void MilesEdgeworth::doubleClickEvent()
{
    if (type == MilesEdgeworth::WEB || type == MilesEdgeworth::WEB) {
        // 进入专注模式状态下, 单击无反应.
        return;
    }
    petMovie->stop();
    QString filename;

     if (type == MilesEdgeworth::SLEEPING) {
        // 唤醒
        filename = QString(":/gifs/special/wake%1.gif").arg(direction);
        specifyChangeGif(filename, MilesEdgeworth::ONCE, direction);
        actionSleep->setText("睡觉");
        actionTea->setEnabled(true);
        actionTeaing->setEnabled(true);
        actionTie->setEnabled(true);
        actionShowProsBadge->setEnabled(true);
    }
    else if (type == MilesEdgeworth::WEBING) {   //专注模式下打开文件夹处理，需要根据openfoder穿出来的信号进行处理，启用对应动作
        // 使用普通变量而非static，确保每个实例创建独立的FolderOpener对象
        // 注意：这里需要将folderOpener作为类的成员变量，而不是局部变量
        FolderOpener::WindowState state = folderOpener.toggleFolderState();
        switch (state) {
        case FolderOpener::WindowState::WindowOpened:
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/special/thinkb%1.gif").arg(direction % 2),
                MilesEdgeworth::WEBING, direction);
            break;
        case FolderOpener::WindowState::WindowRestored:
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/special/thinkb%1.gif").arg(direction % 2),
                MilesEdgeworth::WEBING, direction);
            break;
        case FolderOpener::WindowState::WindowMinimized:
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/special/thinkb%1.gif").arg(direction % 2),
                MilesEdgeworth::WEBING, direction);//这里必须改成删除break才能达到我需要的效果
        case FolderOpener::WindowState::InvalidWindow:
            petMovie->stop();
            specifyChangeGif(QString(":/gifs/stand2/%1.gif").arg(direction % 2),
                MilesEdgeworth::WEBING, direction);
            break;
        }
    }
    else if (type == MilesEdgeworth::TIEING || type == MilesEdgeworth::TIEING) {
        petMovie->stop();
        int next_direct = direction % 2;
        specifyChangeGif(QString(":/gifs/special/tied%1.gif").arg(next_direct),
            MilesEdgeworth::TIE, next_direct);
    }
    else if (type == MilesEdgeworth::TSING || type == MilesEdgeworth::TSING) {
         petMovie->stop();
         int next_direct = direction % 2;
         specifyChangeGif(QString(":/gifs/tingshen/object%1.gif").arg(next_direct),
             MilesEdgeworth::TS, next_direct);
         soundEffect->setSource(QUrl::fromLocalFile(QString(":/audios/objection%1.wav").arg(language)));
         if (soundEffect->isPlaying()) {
             emit soundStop();
         }
         emit soundPlay();

     }
    else if (type == MilesEdgeworth::TS || type == MilesEdgeworth::TS) {
         petMovie->stop();
         int next_direct = direction % 2;
         specifyChangeGif(QString(":/gifs/tingshen/object%1.gif").arg(next_direct),
             MilesEdgeworth::TS, next_direct);
         soundEffect->setSource(QUrl::fromLocalFile(QString(":/audios/objection%1.wav").arg(language)));
         if (soundEffect->isPlaying()) {
             emit soundStop();
         }
         emit soundPlay();
     }
    else if (type == MilesEdgeworth::TIE || type == MilesEdgeworth::TIE) {
        petMovie->stop();
        int next_direct = direction % 2;
        specifyChangeGif(QString(":/gifs/special/tied%1.gif").arg(next_direct),
            MilesEdgeworth::TIE, next_direct);
    }
    else {
        Type next_type = MilesEdgeworth::ONCE;
        double randnum = QRandomGenerator::global()->generateDouble();
        if (randnum < 0.3) {       // 等等，根据语言选择对应气泡gif
            if (language == 0) {   //日语
                filename = QString(":/gifs/objection/dengdenga%1.gif").arg(direction % 2);
            }
            else if (language == 1) {
                filename = QString(":/gifs/objection/dengdengb%1.gif").arg(direction % 2);
            }
            else { // language == 2
                filename = QString(":/gifs/objection/dengdengc%1.gif").arg(direction % 2);
            }
            soundEffect->setSource(QUrl::fromLocalFile(QString(":/audios/holdit%1.wav").arg(language)));
        }
        else if (randnum < 0.7) {  //异议
            if (language == 0) {
                filename = QString(":/gifs/objection/objectiona%1.gif").arg(direction % 2);
            }
            else if (language == 1) {
                filename = QString(":/gifs/objection/objectionb%1.gif").arg(direction % 2);
            }
            else { // language == 2
                filename = QString(":/gifs/objection/objectionc%1.gif").arg(direction % 2);
            }
            soundEffect->setSource(QUrl::fromLocalFile(QString(":/audios/objection%1.wav").arg(language)));
        }
        else {                      //看招
            if (language == 0) {
                filename = QString(":/gifs/objection/kanzhaoa%1.gif").arg(direction % 2);
                soundEffect->setSource(QUrl::fromLocalFile(QString(":/audios/takethat%1.wav").arg(language)));
            }
            else if (language == 1) {
                filename = QString(":/gifs/objection/kanzhaob%1.gif").arg(direction % 2);
                soundEffect->setSource(QUrl::fromLocalFile(QString(":/audios/takethat%1.wav").arg(language)));
            }
            else { // language == 2
                filename = QString(":/gifs/objection/kanzhaoc%1.gif").arg(direction % 2);
                soundEffect->setSource(QUrl::fromLocalFile(QString(":/audios/takethat%1.wav").arg(language)));
            }

        }

        // 更改动画
        specifyChangeGif(filename, next_type, direction % 2);
        // 播放音频
        //soundEffect->play();

        if (soundEffect->isPlaying()) {
            emit soundStop();
        }
        emit soundPlay();
    }

}

// 处理单击事件
void MilesEdgeworth::singleClickEvent()
{
    if (type == MilesEdgeworth::SLEEPING || type == MilesEdgeworth::SLEEP) {
        // 睡觉状态下, 单击无反应.
        return;
    }
    else if (type == MilesEdgeworth::TSING || type == MilesEdgeworth::TS) {
        double randnum = QRandomGenerator::global()->generateDouble();
        QString filename;
        petMovie->stop();
        if (randnum < 0.25) {       // 等等
            int next_direct = direction % 2;
            specifyChangeGif(QString(":/gifs/tingshen/think%1.gif").arg(next_direct),
                MilesEdgeworth::TS, next_direct);
            soundEffect->setSource(QUrl::fromLocalFile(QString(":/audios/holdit%1.wav").arg(language)));
            if (soundEffect->isPlaying()) {
                emit soundStop();
            }
            emit soundPlay();
        }
        else if (randnum < 0.5) {  // 看招
            int next_direct = direction % 2;
            specifyChangeGif(QString(":/gifs/tingshen/tanshou%1.gif").arg(next_direct),
                MilesEdgeworth::TS, next_direct);
        }
        else if (randnum < 0.75 ) {   // 异议
            int next_direct = direction % 2;
            specifyChangeGif(QString(":/gifs/tingshen/zhichu%1.gif").arg(next_direct),
                MilesEdgeworth::TS, next_direct);

        }
        else {                      // koreda
            int next_direct = direction % 2;
            specifyChangeGif(QString(":/gifs/tingshen/damage%1.gif").arg(next_direct),
                MilesEdgeworth::TS, next_direct);
            soundEffect->setSource(QUrl::fromLocalFile(QString(":/audios/damage.wav")));
            if (soundEffect->isPlaying()) {
                emit soundStop();
            }
            emit soundPlay();

        }
    }
    else if (type == MilesEdgeworth::TIEING || type == MilesEdgeworth::TIEING) {
        petMovie->stop();
        int next_direct = direction % 2;
        specifyChangeGif(QString(":/gifs/special/tiedown%1.gif").arg(next_direct),
            MilesEdgeworth::TIE, next_direct);
    }
    else if (type == MilesEdgeworth::TIE || type == MilesEdgeworth::TIEING) {
        petMovie->stop();
        int next_direct = direction % 2;
        specifyChangeGif(QString(":/gifs/special/tiedown%1.gif").arg(next_direct),
            MilesEdgeworth::TIE, next_direct);
    }
    else if (type == MilesEdgeworth::WEB || type == MilesEdgeworth::WEB) {
        // 进入专注模式状态下, 单击无反应.
        return;
    }
    //专注模式打开网页处理
    else if (type == MilesEdgeworth::WEBING || type == MilesEdgeworth::WEBING) {
        UIA_ToggleBrowser(L"https://chat.deepseek.com/", L"DeepSeek");

    }
    else if (type != MilesEdgeworth::STAND) {
        // 非站立也非睡觉时, 转为站立动画
        petMovie->stop();
        specifyChangeGif(QString(":/gifs/stand/%1.gif").arg(direction % 2),
            MilesEdgeworth::STAND, direction % 2);
    }
    else {
        // 站立时, 分区域有不同的反应
        double x = m_pressPos.x();
        double y = m_pressPos.y();
        // 判断点击区域
        switch (direction) {
        case 0:
        {
            if (y < 23 * scale && y + 1.2 * x > 74 * scale) {
                // 点击脸部, 触发scared动画
                petMovie->stop();
                specifyChangeGif(QString(":/gifs/special/scared%1.gif").arg(direction),
                    MilesEdgeworth::ONCE, direction);
            }
            else if (y < 23 * scale) {
                // 点击头部, 触发指头动画(once/2.gif once/3.gif)或抬头看动画(once/18.gif once/19.gif)
                double randnum = QRandomGenerator::global()->generateDouble();
                petMovie->stop();
                if (randnum < 0.5) {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 2),
                        MilesEdgeworth::ONCE, direction);
                }
                else {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 18),
                        MilesEdgeworth::ONCE, direction);
                }
            }
            else if (x < 43 * scale && y < 40 * scale) {
                // 点击大臂, 触发转向正面小动画
                petMovie->stop();
                specifyChangeGif(QString(":/gifs/special/turn%1.gif").arg(direction),
                    MilesEdgeworth::ONCE, 1 - direction);

            }
            else if (x < 43 * scale && y < 57 * scale) {
                // 点击小臂, 触发看表动画(once/8.gif once/9.gif)或摊手动画(once/0.gif once/1.gif)
                double randnum = QRandomGenerator::global()->generateDouble();
                petMovie->stop();
                if (randnum < 0.5) {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 8),
                        MilesEdgeworth::ONCE, direction);
                }
                else {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction),
                        MilesEdgeworth::ONCE, direction);
                }
            }
            else if (y < 38 * scale) {
                // 点击胸部, 触发抱胸手指动画(once/4.gif once/5.gif)
                petMovie->stop();
                specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 4),
                    MilesEdgeworth::ONCE, direction);
            }
            else if (y < 53 * scale) {
                // 点击肚子, 触发指指点点动画(once/10.gif once/11.gif)或鞠躬
                petMovie->stop();
                if (y > 46 * scale) {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 10),
                        MilesEdgeworth::ONCE, direction);
                }
                else {
                    specifyChangeGif(QString(":/gifs/special/bow%1.gif").arg(direction),
                        MilesEdgeworth::ONCE, direction);
                }
            }
            else {
                // 点击腿部, 触发后退或向下看动画
                petMovie->stop();
                if (x < 50 * scale) {
                    specifyChangeGif(QString(":/gifs/special/back%1.gif").arg(direction),
                        MilesEdgeworth::ONCE, direction);
                }
                else {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 16),
                        MilesEdgeworth::ONCE, direction);
                }
            }
            break;
        }
        case 1:
        {
            if (y < 23 * scale && y - 1.2 * x > -43.4 * scale) {
                // 点击脸部, 触发scared动画
                petMovie->stop();
                specifyChangeGif(QString(":/gifs/special/scared%1.gif").arg(direction),
                    MilesEdgeworth::ONCE, direction);
            }
            else if (y < 23 * scale) {
                double randnum = QRandomGenerator::global()->generateDouble();
                petMovie->stop();
                if (randnum < 0.5) {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 2),
                        MilesEdgeworth::ONCE, direction);
                }
                else {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 18),
                        MilesEdgeworth::ONCE, direction);
                }
            }
            else if (x > 57 * scale && y < 40 * scale) {
                // 点击大臂, 触发转向正面小动画
                petMovie->stop();
                specifyChangeGif(QString(":/gifs/special/turn%1.gif").arg(direction),
                    MilesEdgeworth::ONCE, 1 - direction);

            }
            else if (x > 57 * scale && y < 57 * scale) {
                // 点击小臂, 触发看表动画(once/8.gif once/9.gif)或摊手动画(once/0.gif once/1.gif)
                double randnum = QRandomGenerator::global()->generateDouble();
                petMovie->stop();
                if (randnum < 0.5) {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 8),
                        MilesEdgeworth::ONCE, direction);
                }
                else {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction),
                        MilesEdgeworth::ONCE, direction);
                }
            }
            else if (y < 38 * scale) {
                // 点击胸部, 触发抱胸手指动画(once/4.gif once/5.gif)
                petMovie->stop();
                specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 4),
                    MilesEdgeworth::ONCE, direction);
            }
            else if (y < 53 * scale) {
                // 点击肚子, 触发指指点点动画(once/10.gif once/11.gif)或鞠躬
                petMovie->stop();
                if (y > 46 * scale) {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 10),
                        MilesEdgeworth::ONCE, direction);
                }
                else {
                    specifyChangeGif(QString(":/gifs/special/bow%1.gif").arg(direction),
                        MilesEdgeworth::ONCE, direction);
                }
            }
            else {
                // 点击腿部, 触发后退或向下看动画
                petMovie->stop();
                if (x > 49 * scale) {
                    specifyChangeGif(QString(":/gifs/special/back%1.gif").arg(direction),
                        MilesEdgeworth::ONCE, direction);
                }
                else {
                    specifyChangeGif(QString(":/gifs/once/%1.gif").arg(direction + 16),
                        MilesEdgeworth::ONCE, direction);
                }
            }
            break;
        }
        }
    }
}



//图片顶置查看
void MilesEdgeworth::on_viewerClosing(bool prevIsNull, PicViewer* nextPtr) 
{
    numViewer--;
    if (numViewer == MAXVIEWER - 1) actionPic->setEnabled(true);
    if (prevIsNull) firstViewer = nextPtr;
}

void MilesEdgeworth::on_trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        this->activateWindow();
    }
}

void MilesEdgeworth::mouseDoubleClickEvent(QMouseEvent* event) {
    if (objectionModeEnabled && !ObjectionWindow::isWindowActive()) {

        // 创建窗口对象但不立即显示
        ObjectionWindow* objectionWindow = new ObjectionWindow();

        // 在QTimer中延迟显示，这样可以避免事件循环中的问题
        QTimer::singleShot(10, [objectionWindow, event]() {


            // 使用最新的鼠标位置而非事件位置
            objectionWindow->showObjectionAt(QCursor::pos());
            });
        event->accept();
    }
    else {
        QWidget::mouseDoubleClickEvent(event);
    }
}
//取消异议模式，按钮恢复使用
void MilesEdgeworth::toggleObjectionMode(bool enabled) {
    actionWeb->setEnabled(true);
    actionWeb->setText("专注模式");
    actionTea->setEnabled(true);
    actionTea->setText("喂食红茶");
    actionTeaing->setEnabled(true);
    actionTie->setEnabled(true);
    actionTie->setText("束缚");
    actionSleep->setEnabled(true);
    actionSleep->setText("睡觉");
    actionShowProsBadge->setEnabled(true);
    actionTurn->setEnabled(true);
    actionTs->setEnabled(true);
    setObjectionMode(enabled);

}

void MilesEdgeworth::setObjectionMode(bool enabled) {
    // 如果状态没变，直接返回
    if (objectionModeEnabled == enabled)

        return;

    objectionModeEnabled = enabled;

    // 根据状态注册或取消注册原始输入
    if (objectionModeEnabled) {
        // 注册原始输入设备
        RegisterRawInput();
    }
    // 注意：Raw Input API不需要显式取消注册，当窗口销毁时会自动清理
    //启动异议模式，按钮置灰
    if (objectionModeEnabled) {
        actionTie->setDisabled(true);
        actionTea->setDisabled(true);
        actionTeaing->setDisabled(true);
        actionWeb->setDisabled(true);
        actionSleep->setDisabled(true);
        actionTurn->setEnabled(true);
        actionShowProsBadge->setDisabled(true);
        actionTs->setDisabled(true);

    }
}

// 原始输入实现
bool MilesEdgeworth::RegisterRawInput() {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;      // HID使用页面
    rid.usUsage = 0x02;          // 鼠标使用ID
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = (HWND)this->winId();  // 将消息发送到此窗口

    if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE))) {
        qWarning() << "注册原始输入设备失败，错误码:" << GetLastError();
        return false;
    }
    return true;
}

// 检测双击的变量
static DWORD lastClickTime = 0;
static LONG lastClickX = 0;
static LONG lastClickY = 0;

void MilesEdgeworth::ProcessRawInput(HRAWINPUT hRawInput) {
    UINT dataSize;
    GetRawInputData(hRawInput, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

    std::vector<BYTE> buffer(dataSize);
    if (GetRawInputData(hRawInput, RID_INPUT, buffer.data(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize) {
        return;
    }

    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
    if (raw->header.dwType == RIM_TYPEMOUSE && (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)) {
        DWORD currentTime = GetTickCount();
        POINT cursorPos;
        GetCursorPos(&cursorPos);

        // 检测是否是双击（同一位置，时间间隔短）
        if (currentTime - lastClickTime <= GetDoubleClickTime() &&
            abs(cursorPos.x - lastClickX) <= GetSystemMetrics(SM_CXDOUBLECLK) &&
            abs(cursorPos.y - lastClickY) <= GetSystemMetrics(SM_CYDOUBLECLK)) {

            // 处理双击
            QPoint globalPos(cursorPos.x, cursorPos.y);
            QMetaObject::invokeMethod(this, "handleGlobalDoubleClick",
                Qt::QueuedConnection,
                Q_ARG(QPoint, globalPos));

            // 重置计时器以避免三击被识别为另一次双击
            lastClickTime = 0;
        }
        else {
            // 记录第一次点击
            lastClickTime = currentTime;
            lastClickX = cursorPos.x;
            lastClickY = cursorPos.y;
        }
    }
}

bool MilesEdgeworth::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (!objectionModeEnabled) {
        return QWidget::nativeEvent(eventType, message, result);

    }

    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_INPUT) {
        ProcessRawInput((HRAWINPUT)msg->lParam);
        *result = 0;
        return true;
    }

    return QWidget::nativeEvent(eventType, message, result);
}

void MilesEdgeworth::handleGlobalDoubleClick(const QPoint& globalPos) {
    // 确保只在异议模式启用时处理
    if (objectionModeEnabled) {
        ObjectionWindow* objectionWindow = new ObjectionWindow();
        objectionWindow->showObjectionAt(globalPos);
        petMovie->stop();
        specifyChangeGif(QString(":/gifs/special/object%1.gif").arg(direction % 2),
            MilesEdgeworth::ONCE, direction);

    }  
}

