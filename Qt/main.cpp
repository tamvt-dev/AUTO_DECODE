#include <QApplication>
#include "mainwindow.h"

#include "../core/include/core.h"
#include "../core/include/logging.h"
#include "../core/include/crash_handler.h"


int main(int argc, char *argv[])
{
    install_crash_handler();
    log_init(LOG_LEVEL_INFO, NULL, TRUE);
    core_init();         
    QApplication app(argc, argv);
    MainWindow w;
    w.show();

    int ret = app.exec();

    core_cleanup();       
    log_close();
    return ret;
}