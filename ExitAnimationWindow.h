// ExitAnimationWindow.h
#pragma once
#include <QWidget>
#include <QLabel>
#include <QMovie>
#include <QTimer>
class ExitAnimationWindow : public QWidget
{
    Q_OBJECT
public:
    static void playExitAnimation(QWidget* parent = nullptr);

private:
    explicit ExitAnimationWindow(QWidget* parent = nullptr);
    ~ExitAnimationWindow();

    void setupAnimation();
    QLabel* gifLabel;
    QMovie* exitMovie;
};