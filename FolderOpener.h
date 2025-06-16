// FolderOpener.h

#pragma once
#include <Windows.h>
#include <string>
#include <vector>

class FolderOpener {
public:
    enum class WindowState {
        InvalidWindow,   // 无效窗口句柄
        WindowOpened,    // 新窗口被打开
        WindowRestored,  // 窗口已恢复
        WindowMinimized  // 窗口已最小化
    };

    FolderOpener();  // 构造函数，创建唯一的实例ID
    WindowState toggleFolderState();

private:
    HWND findExplorerWindow();
    bool isOurWindow(HWND hwnd);
    void trackNewExplorerWindow();
    WindowState toggleWindowState(HWND hwnd);
    void openSpecialFolder();

    HWND lastFoundWindow;
    std::wstring windowPropertyName;  // 当前实例的唯一窗口属性名称
};