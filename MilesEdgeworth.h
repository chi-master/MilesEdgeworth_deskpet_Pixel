#pragma execution_character_set("utf-8")
#include <QtWidgets/QWidget>
#include "ui_MilesEdgeworth.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMovie>
#include <QMenu>
#include <QActionGroup>
#include <QSoundEffect>
#include <QTimer>
#include <QRandomGenerator>
#include <QPropertyAnimation>
#include <QThread>
#include <QSystemTrayIcon>
#include "ProsecutorBadge.h"
#include "PicViewer.h"
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QString>
#include <windows.h>
#include "FolderOpener.h"
#include <QtGlobal>  // 这个头文件定义了qintptr类型
#include <QSettings>
#include <psapi.h>
#include <uiautomation.h>
#include "ExitAnimationWindow.h"
#include <comutil.h>
#include <iostream>
#include <comdef.h>
#pragma comment(lib, "uiautomationcore.lib")
#define WIN32_LEAN_AND_MEAN

const int ONCE_NUM = 20;
const int WALK_NUM = 8;
const int RUN_NUM = 8;
const double MINI = 1;
const double SMALL = 1.5;
const double MEDIAN = 2;
const double BIG = 3;
const int MAXVIEWER = 30;


class MilesEdgeworth : public QWidget
{
    Q_OBJECT

public:

    enum Type {
        RUN = 0,
        WALK,
        STAND,
        ONCE,
        SLEEP,
        SLEEPING,
        TAKETHAT,
        TAKETHATING,
        BRIEFCASEIN,
        BRIEFCASESTOP,
        CROUCH,
        TIE,
        TIEING,
        WEB,
        WEBING,
        TEA,
        TS,
        TSING,
    };

    explicit MilesEdgeworth(QWidget* parent = nullptr);
    ~MilesEdgeworth();

    virtual void moveEvent(QMoveEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void closeEvent(QCloseEvent* event) override;
    virtual void enterEvent(QEnterEvent* event) override;

    void createContextMenu();
    void createTrayIcon();
    void createClickTimer();
    void createBadgeTimer();
    void createShakeTimer();
    void createPicViewer();
    void showContextMenu();
    // 自动随机切换Gif
    void autoChangeGif()
    {
        Type next_type = MilesEdgeworth::STAND; // 下一动作, 默认为STAND
        int next_direct = 0;    // 下一方向, 默认为向右
        switch (type) {
        case MilesEdgeworth::RUN:
        {
            double randnum = QRandomGenerator::global()->generateDouble();
            if (randnum <= 0.2) { // 0.2概率接同向走路
                next_type = MilesEdgeworth::WALK;
                next_direct = direction;
                petMovie->setFileName(QString(":/gifs/walk/%1.gif").arg(next_direct));
            }
            else if (randnum < 0.4) {  // 0.2概率接随机跑步
                next_type = MilesEdgeworth::RUN;
                next_direct = QRandomGenerator::global()->bounded(RUN_NUM);
                petMovie->setFileName(QString(":/gifs/run/%1.gif").arg(next_direct));
            }
            else {  // 0.6概率接站立
                next_type = MilesEdgeworth::STAND;
                next_direct = direction % 2;
                petMovie->setFileName(QString(":/gifs/stand/%1.gif").arg(next_direct));
            }
            break;
        }
        case MilesEdgeworth::WALK:
        {
            double randnum = QRandomGenerator::global()->generateDouble();
            if (randnum <= 0.2) { // 0.2概率接同向跑步
                next_type = MilesEdgeworth::RUN;
                next_direct = direction;
                petMovie->setFileName(QString(":/gifs/run/%1.gif").arg(next_direct));
            }
            else if (randnum < 0.5) {  // 0.3概率接随机走路
                next_type = MilesEdgeworth::WALK;
                next_direct = QRandomGenerator::global()->bounded(WALK_NUM);
                petMovie->setFileName(QString(":/gifs/walk/%1.gif").arg(next_direct));
            }
            else {  // 0.5概率接站立
                next_type = MilesEdgeworth::STAND;
                next_direct = direction % 2;
                petMovie->setFileName(QString(":/gifs/stand/%1.gif").arg(next_direct));
            }
            break;
        }
        case MilesEdgeworth::TAKETHAT:
        case MilesEdgeworth::ONCE:
        {
            // 若当前动作为单次动作, 则停止后立刻切换为站立, 方向保持不变
            next_type = MilesEdgeworth::STAND;
            next_direct = direction % 2;
            petMovie->setFileName(QString(":/gifs/stand/%1.gif").arg(next_direct));
            break;
        }
        case MilesEdgeworth::STAND:
        {
            double randnum = QRandomGenerator::global()->generateDouble();
            if (randnum < 0.08) {
                // 0.08的概率直接切换为反向站立
                next_type = MilesEdgeworth::STAND;
                next_direct = 1 - direction;
                petMovie->setFileName(QString(":/gifs/stand/%1.gif").arg(next_direct));
            }
            else if (randnum < 0.012) {
                // 0.12的概率切换转身动画
                petMovie->stop();
                specifyChangeGif(QString(":/gifs/special/turn%1.gif").arg(direction),
                    MilesEdgeworth::ONCE, 1 - direction);
            }
            else if (randnum < 0.8) {  //0.8
                // 0.6的概率切换为单次动作, 方向不变
                next_type = MilesEdgeworth::ONCE;
                next_direct = direction;
                int num = QRandomGenerator::global()->bounded(ONCE_NUM);
                num = (direction + 2 * num) % ONCE_NUM;
                petMovie->setFileName(QString(":/gifs/once/%1.gif").arg(num));
            }
            else if (randnum < 0.9) {  //0.9
                // 0.1的概率切换为走路
                next_type = MilesEdgeworth::WALK;
                next_direct = QRandomGenerator::global()->bounded(WALK_NUM);
                petMovie->setFileName(QString(":/gifs/walk/%1.gif").arg(next_direct));
            }
            else {
                // 0.1的概率切换为跑步
                next_type = MilesEdgeworth::RUN;
                next_direct = QRandomGenerator::global()->bounded(RUN_NUM);
                petMovie->setFileName(QString(":/gifs/run/%1.gif").arg(next_direct));
            }
            break;
        }
        }
        type = next_type;
        direction = next_direct;
        petMovie->start();

    }
    // 切换为指定的Gif. 参数为: 下一动作文件名, 下一动作类型, 下一动作方向
    void specifyChangeGif(const QString& filename, Type next_type, int next_direct);

    void runMove();
    void walkMove();

    void showProsBadge();//专注模式网页打开
    void doubleClickEvent();
    void singleClickEvent();
    void setObjectionMode(bool enabled);    // 设置异议模式
    // 语言设置函数
    void saveLanguageSetting(int lang);
    void loadLanguageSetting();
signals:
    void soundPlay();
    void soundStop();
    void exitProgram();

public slots:
    void on_viewerClosing(bool prevIsNull, PicViewer* nextPtr);
    void on_trayActivated(QSystemTrayIcon::ActivationReason reason);
    // 切换异议模式的槽函数
    void toggleObjectionMode(bool enabled);
    // 处理全局双击事件的槽函数
    void handleGlobalDoubleClick(const QPoint& globalPos);
private slots:


protected:
    //异议模式重写鼠标双击
        // 重写鼠标双击事件处理函数
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    // 重写原生事件处理函数，用于处理底层Windows消息
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
private:
    Ui::MilesEdgeworthClass ui;
    QMovie* petMovie;
    QTimer* clickTimer;         // 用于判断单击或双击
    QTimer* badgeTimer;         // 用于让检察官徽章消失
    QTimer* shakeTimer;         // 用于判断是否触发害怕地震动作
    QSoundEffect* soundEffect;  // 用于播放语音
    ProsecutorBadge* prosbadge; // 检察官徽章
    bool m_dragging;	        // 是否正在拖动
    QPoint m_startPosition;     // 拖动开始前的鼠标位置, 是全局坐标
    QPoint m_framePosition;	    // 窗体的原始位置
    QPointF m_pressPos;         // 鼠标按下时的位置, 是widget的坐标 
    Type type;                  // 类型: run,walk,stand,once
    int direction;              // 方向: 左为1,右为0. 在走/跑时更细分为8个方向
    double scale = MEDIAN;         // 缩放倍数
    int language;           // 0为日语,1为英语,2为汉语
    bool doubleClick = false;
    int shakeX = 0;                 // 用于判断是否触发害怕地震动作
    int shakeDirect = 1;            // 用于判断是否触发害怕地震动作
    int shakeCount = 0;             // 用于判断是否触发害怕地震动作, 计数move更换方向的次数

    QScreen* nowScreen = nullptr;

    QMenu* menu;
    QMenu* sizeMenu;     // 设置大小
    QMenu* languageMenu; // 设置语音语言
    QMenu* screenMenu;   // 双屏的有关设置
    QAction* actionWeb;  // 专注模式web打开
    QAction* actionTop;  // 是否置于顶层
    QAction* actionMove; // 是否禁止自动移动
    QAction* actionMute; // 是否静音
    QAction* actionTea;  // 喂茶
    QAction* actionTeaing;  // 喂茶
    QAction* actionSleep;// 睡觉
    QAction* actionPic;  // 显示参考图
    QAction* actionExit; // 退出
    QAction* actionTie;  // 束缚
    QAction* actionTurn;  // 转向
    QAction* actionShowProsBadge;  // 飞徽章
    QAction* actionTs;  // 庭审模式

    // 子菜单的action
    QActionGroup* sizeGroup;
    QAction* miniSize;
    QAction* smallSize;
    QAction* medianSize;
    QAction* bigSize;
    QActionGroup* lanGroup;
    QAction* chLanguage;
    QAction* engLanguage;
    QAction* jpLanguage;
    QActionGroup* screenGroup;
    QAction* leftPrimary;
    QAction* rightPrimary;
    QAction* singleScreen;

    QThread* thread;

    // 参考图查看器多窗口使用链表结构
    PicViewer* firstViewer = nullptr;
    int numViewer = 0;

    // 系统托盘
    QSystemTrayIcon* tray;

    //设置大小
    void applyScale(double scaleValue, QAction* action);
    void saveSettings();
    void loadSettings();
    // 在头文件中添加固定位置的方法
    void setToDefaultPosition() {
        // 获取当前屏幕
        nowScreen = QGuiApplication::primaryScreen();
        QRect desktopRect = nowScreen->availableGeometry();

        // 计算左下角位置（根据当前的缩放比例）
        // 左下角位置：x为左边界加一点偏移，y为底部减去桌宠高度
        int x = desktopRect.x() + 20; // 距离左边界20像素
        int y = desktopRect.y() + desktopRect.height() - 100 * scale - 20; // 距离底部20像素

        // 移动桌宠到计算出的位置
        move(x, y);
    }

    bool objectionModeEnabled;  // 异议模式开关
    QAction* actionObjection; // 异议模式菜单项
    // 注册原始输入设备
    bool RegisterRawInput();

    // 处理原始输入数据
    void ProcessRawInput(HRAWINPUT hRawInput);




    FolderOpener folderOpener;  // 添加 FolderOpener 成员变量
    // 安全释放COM对象
    template<class T> void SafeRelease(T** ppT) {
        if (*ppT) {
            (*ppT)->Release();
            *ppT = NULL;
        }
    }


    void UIA_ToggleBrowser(const wchar_t* url, const wchar_t* titleKeyword) {
        CoInitialize(NULL);

        // 静态变量记录目标浏览器进程ID（关键新增）
        static int s_targetProcessId = 0;

        IUIAutomation* pUIA = NULL;
        HRESULT hr = CoCreateInstance(CLSID_CUIAutomation, NULL,
            CLSCTX_INPROC_SERVER, IID_IUIAutomation,
            (void**)&pUIA);
        if (SUCCEEDED(hr)) {
            // 1. 创建基础条件：浏览器类名（必选）
            VARIANT varClass;
            varClass.vt = VT_BSTR;
            varClass.bstrVal = SysAllocString(L"Chrome_WidgetWin_1");
            IUIAutomationCondition* pClassCondition = NULL;
            pUIA->CreatePropertyCondition(UIA_ClassNamePropertyId, varClass, &pClassCondition);

            // 2. 创建增强条件：进程ID（如果已记录过）
            IUIAutomationCondition* pProcessCondition = NULL;
            VARIANT varProcessId;
            if (s_targetProcessId != 0) {
                varProcessId.vt = VT_I4;
                varProcessId.lVal = s_targetProcessId;
                pUIA->CreatePropertyCondition(UIA_ProcessIdPropertyId, varProcessId, &pProcessCondition);
            }

            // 3. 组合条件：类名必须匹配，进程ID可选匹配
            IUIAutomationCondition* pFinalCondition = pClassCondition;
            if (pProcessCondition) {
                pUIA->CreateAndCondition(pClassCondition, pProcessCondition, &pFinalCondition);
                SafeRelease(&pClassCondition);
                SafeRelease(&pProcessCondition);
            }

            // 4. 查找浏览器窗口
            IUIAutomationElement* pBrowser = NULL;
            IUIAutomationElement* pRoot = NULL;
            pUIA->GetRootElement(&pRoot);
            if (pRoot) {
                pRoot->FindFirst(TreeScope_Children, pFinalCondition, &pBrowser);
                SafeRelease(&pRoot);
                petMovie->stop();
                int next_direct = direction % 2;
                specifyChangeGif(QString(":/gifs/special/book%1.gif").arg(next_direct),//
                    MilesEdgeworth::WEBING, next_direct);
            }

            // 5. 智能回退：如果通过进程ID未找到，尝试仅用类名查找
            if (!pBrowser && s_targetProcessId != 0) {
                s_targetProcessId = 0; // 重置无效的进程ID
                SafeRelease(&pFinalCondition);
                pFinalCondition = pClassCondition; // 回退到仅类名条件

                pUIA->GetRootElement(&pRoot);
                if (pRoot) {
                    pRoot->FindFirst(TreeScope_Children, pFinalCondition, &pBrowser);
                    SafeRelease(&pRoot);
                }
            }

            // 6. 如果找到新窗口，记录其进程ID
            if (pBrowser && s_targetProcessId == 0) {
                pBrowser->get_CurrentProcessId(&s_targetProcessId);
                // DEBUG: 可在此输出 s_targetProcessId 验证
            }

            // 7. 原有窗口操作逻辑（保持最小化/恢复功能）
            if (pBrowser) {
                IUIAutomationWindowPattern* pWindowPattern = NULL;
                hr = pBrowser->GetCurrentPattern(UIA_WindowPatternId, (IUnknown**)&pWindowPattern);

                if (SUCCEEDED(hr) && pWindowPattern) {
                    WindowVisualState currentState;
                    hr = pWindowPattern->get_CurrentWindowVisualState(&currentState);

                    if (SUCCEEDED(hr)) {
                        // 根据窗口状态切换GIF
                        if (currentState == WindowVisualState_Minimized) {
                            // 窗口正常显示时显示的GIF
                            petMovie->stop();
                            int next_direct = direction % 2;
                            specifyChangeGif(QString(":/gifs/special/book%1.gif").arg(next_direct),//
                                MilesEdgeworth::WEBING, next_direct);
                        }
                        else {
                            // 窗口正常显示时显示的GIF
                            petMovie->stop();
                            int next_direct = direction % 2;
                            specifyChangeGif(QString(":/gifs/stand2/%1.gif").arg(next_direct),
                                MilesEdgeworth::WEBING, next_direct);
                        }

                        // 切换窗口状态
                        hr = pWindowPattern->SetWindowVisualState(
                            currentState == WindowVisualState_Minimized ?
                            WindowVisualState_Normal : WindowVisualState_Minimized);
                        if (FAILED(hr)) {
                            _com_error err(hr);
                            std::wcerr << L"SetWindowVisualState failed: "
                                << err.ErrorMessage() << std::endl;
                        }
                    }
                    SafeRelease(&pWindowPattern);
                }
                SafeRelease(&pBrowser);
            }
            else {
                // 8. 未找到窗口则启动浏览器（新增进程ID记录建议）
                SHELLEXECUTEINFOW sei = { sizeof(sei) };
                sei.lpVerb = L"open";
                sei.lpFile = url;
                sei.nShow = SW_SHOWNORMAL;
                if (ShellExecuteExW(&sei)) {
                    // 可选增强：通过sei.hProcess获取新进程ID
                    // WaitForInputIdle + GetProcessId(sei.hProcess)
                }
            }

            // 9. 资源清理（新增进程ID相关清理）
            SafeRelease(&pFinalCondition);
            VariantClear(&varClass);
            if (s_targetProcessId != 0) VariantClear(&varProcessId);
            SafeRelease(&pUIA);
        }
        else {
            _com_error err(hr);
            std::wcerr << L"CoCreateInstance failed: " << err.ErrorMessage() << std::endl;
        }
        CoUninitialize();
    }

    bool isClosing = false;

    void startExitSequence() {
        isClosing = true;
        this->hide();
        ExitAnimationWindow::playExitAnimation(this);
        QTimer::singleShot(1, this, &MilesEdgeworth::delayedClose);
    }

    void delayedClose() {
        this->close();
    }


};