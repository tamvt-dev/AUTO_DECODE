#include <QApplication>
#include <QIcon>
#include "mainwindow.h"
#include "console_manager.h"

#include "../core/include/core.h"
#include "../core/include/logging.h"
#include "../core/include/crash_handler.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QIcon appIcon(":/icons/app.ico");
    app.setWindowIcon(appIcon);
    QByteArray logPath = ConsoleManager::logFilePath().toLocal8Bit();

    install_crash_handler();
    log_init(LOG_LEVEL_INFO, logPath.constData(), FALSE);
    ConsoleManager::initialize();
    core_init();
    MainWindow w;
    w.setWindowIcon(appIcon);
    w.show();

    int ret = app.exec();

    core_cleanup();       
    log_close();
    return ret;
}
