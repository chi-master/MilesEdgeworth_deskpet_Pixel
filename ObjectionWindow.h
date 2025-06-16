// ObjectionWindow.h
#ifndef OBJECTIONWINDOW_H
#define OBJECTIONWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QMovie>
#include <QSoundEffect>
#include <QTimer>

class ObjectionWindow : public QWidget {
    Q_OBJECT

public:
    void setMuted(bool muted);
    static void syncMuteState(bool muted);  // 新增静态方法
    explicit ObjectionWindow(QWidget* parent = nullptr);
    ~ObjectionWindow();
    // 静态成员，跟踪当前活动的窗口实例
    static ObjectionWindow* activeWindow;

    // 设置和播放GIF及音效
    void showObjectionAt(const QPoint& globalPos);

    // 静态方法，用于避免重复创建窗口
    static bool isWindowActive() { return activeWindow != nullptr; }

protected:
    // 添加事件过滤器，用于处理鼠标事件
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QLabel* gifLabel;      // 用于显示GIF的标签
    QMovie* objectionMovie; // GIF动画
    QSoundEffect* soundEffect; // 音效播放器
    static bool globalMuted;  // 新增静态静音状态


    void setupUi();
};

#endif // OBJECTIONWINDOW_H