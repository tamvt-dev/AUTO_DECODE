#include "console_manager.h"

#include <QCoreApplication>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
bool g_consoleVisible = false;
}

namespace ConsoleManager {

void initialize()
{
#ifdef Q_OS_WIN
    HWND consoleWindow = GetConsoleWindow();
    if (!consoleWindow) {
        return;
    }

    g_consoleVisible = IsWindowVisible(consoleWindow) != FALSE;
    ShowWindow(consoleWindow, SW_HIDE);
    g_consoleVisible = false;
#endif
}

void setVisible(bool visible)
{
#ifdef Q_OS_WIN
    HWND consoleWindow = GetConsoleWindow();
    if (!consoleWindow) {
        return;
    }

    ShowWindow(consoleWindow, visible ? SW_SHOW : SW_HIDE);
    g_consoleVisible = visible;
#else
    Q_UNUSED(visible);
#endif
}

bool isVisible()
{
#ifdef Q_OS_WIN
    HWND consoleWindow = GetConsoleWindow();
    if (!consoleWindow) {
        return false;
    }

    g_consoleVisible = IsWindowVisible(consoleWindow) != FALSE;
#endif
    return g_consoleVisible;
}

QString logFilePath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("lastlog.txt");
}

}
