#ifndef CONSOLE_MANAGER_H
#define CONSOLE_MANAGER_H

#include <QString>

namespace ConsoleManager {
void initialize();
void setVisible(bool visible);
bool isVisible();
QString logFilePath();
}

#endif // CONSOLE_MANAGER_H
