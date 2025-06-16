// FolderOpener.cpp

#include "FolderOpener.h"
#include <shellapi.h>
#include <algorithm>
#include <sstream>
#include "MilesEdgeworth.h"
#include <processthreadsapi.h>

// 构造函数，为每个实例创建唯一的窗口属性名称
FolderOpener::FolderOpener() {
    // 使用进程ID和时间戳创建唯一标识符
    DWORD processId = GetCurrentProcessId();
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);

    // 组合进程ID和时间创建唯一字符串
    std::wstringstream ss;
    ss << L"FolderOpener_" << processId << L"_" << fileTime.dwLowDateTime;
    windowPropertyName = ss.str();

    lastFoundWindow = nullptr;
}

// 主功能函数：返回窗口操作状态
FolderOpener::WindowState FolderOpener::toggleFolderState() {
    HWND hwnd = findExplorerWindow();

    if (hwnd && IsWindow(hwnd)) {
        return toggleWindowState(hwnd);  // 返回切换后的状态
    }
    else {
        openSpecialFolder();
        trackNewExplorerWindow();
        return WindowState::WindowOpened;  // 明确返回"新窗口打开"状态
    }
}

// 修改后的窗口状态切换函数
FolderOpener::WindowState FolderOpener::toggleWindowState(HWND hwnd) {
    if (!IsWindow(hwnd)) {
        return WindowState::InvalidWindow;  // 无效窗口状态
    }

    if (IsIconic(hwnd)) {
        // 如果窗口是最小化的，则恢复它
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        return WindowState::WindowRestored;  // 窗口恢复状态
    }
    else {
        // 如果窗口是正常的，则最小化它
        ShowWindow(hwnd, SW_MINIMIZE);
        return WindowState::WindowMinimized;  // 窗口最小化状态
    }
}

// 打开文件夹功能
void FolderOpener::openSpecialFolder() {
    // 总是打开一个新的资源管理器窗口，不再检查lastFoundWindow
    ShellExecuteW(nullptr,
        L"open",
        L"explorer.exe",
        L"/e,::{20D04FE0-3AEA-1069-A2D8-08002B30309D}",
        nullptr,
        SW_SHOWNORMAL);
}

// 检查窗口是否被当前实例追踪
bool FolderOpener::isOurWindow(HWND hwnd) {
    return GetProp(hwnd, windowPropertyName.c_str()) != nullptr;
}

// 查找被当前实例追踪的资源管理器窗口
HWND FolderOpener::findExplorerWindow() {
    const wchar_t* classes[] = { L"CabinetWClass", L"ExploreWClass" };

    // 优先查找被当前实例标记的窗口
    for (auto cls : classes) {
        HWND hwnd = nullptr;
        while ((hwnd = FindWindowEx(nullptr, hwnd, cls, nullptr)) != nullptr) {
            if (isOurWindow(hwnd) && IsWindow(hwnd)) {
                lastFoundWindow = hwnd;
                return hwnd;
            }
        }
    }

    // 如果没有找到被追踪的窗口，返回nullptr以触发创建新窗口
    return nullptr;
}

// 增强的窗口追踪函数
void FolderOpener::trackNewExplorerWindow() {
    const int maxRetries = 50;   // 最大重试次数
    const int retryDelay = 200;  // 重试间隔时间（毫秒）
    const wchar_t* classes[] = { L"CabinetWClass", L"ExploreWClass" };

    // 记录上次查找时已存在的窗口，用于识别新创建的窗口
    std::vector<HWND> existingWindows;
    for (auto cls : classes) {
        HWND hwnd = nullptr;
        while ((hwnd = FindWindowEx(nullptr, hwnd, cls, nullptr)) != nullptr) {
            existingWindows.push_back(hwnd);
        }
    }

    for (int retry = 0; retry < maxRetries; ++retry) {
        for (auto cls : classes) {
            std::vector<HWND> currentWindows;
            HWND hwnd = nullptr;

            // 查找所有当前可见的资源管理器窗口
            while ((hwnd = FindWindowEx(nullptr, hwnd, cls, nullptr)) != nullptr) {
                if (IsWindowVisible(hwnd) && (GetWindowTextLength(hwnd) > 0)) {
                    currentWindows.push_back(hwnd);
                }
            }

            // 查找新创建的窗口（不在之前列表中的窗口）
            for (HWND newWindow : currentWindows) {
                if (std::find(existingWindows.begin(), existingWindows.end(), newWindow) == existingWindows.end() &&
                    !isOurWindow(newWindow)) {
                    // 使用当前实例的窗口属性标记这个窗口
                    SetProp(newWindow, windowPropertyName.c_str(), (HANDLE)1);
                    lastFoundWindow = newWindow;
                    return;
                }
            }
        }
        Sleep(retryDelay);
    }
}