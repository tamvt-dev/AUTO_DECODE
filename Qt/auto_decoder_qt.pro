TEMPLATE = app
TARGET = auto_decoder_qt
QT += core widgets gui
CONFIG += c++11 console

# 🔥 Quan trọng: ép dùng MinGW (tránh mismatch)
QMAKE_CC = gcc
QMAKE_CXX = g++

# Include paths
INCLUDEPATH += ../core/include
INCLUDEPATH += C:/msys64/ucrt64/include/glib-2.0
INCLUDEPATH += C:/msys64/ucrt64/lib/glib-2.0/include

# Core sources
SOURCES += \
    ../core/src/core.c \
    ../core/src/decoder.c \
    ../core/src/encoder.c \
    ../core/src/plugin.c \
    ../core/src/lru_cache.c \
    ../core/src/logging.c \
    ../core/src/errors.c \
    ../core/src/crash_handler.c \
    ../core/src/score.c \
    ../core/src/buffer.c \
    ../core/src/pipeline.c

# Plugins
SOURCES += \
    ../core/plugins/atbash_plugin.c \
    ../core/plugins/base64_plugin.c \
    ../core/plugins/caesar_plugin.c \
    ../core/plugins/rot13_plugin.c \
    ../core/plugins/scramble_plugin.c \
    ../core/plugins/url_plugin.c \
    ../core/plugins/xor_plugin.c

# Qt sources
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    decoder_engine.cpp \
    history_manager.cpp \
    history_tab.cpp \
    notification_widget.cpp \
    notification_manager.cpp

HEADERS += \
    mainwindow.h \
    history_manager.h \
    history_tab.h \
    notification_widget.h \
    notification_manager.h

RESOURCES += resources.qrc

# 🔥 FIX warning cast GLib
QMAKE_CFLAGS += -Wno-cast-function-type

# 🔥 QUAN TRỌNG: link GLib đúng cách
LIBS += -LC:/msys64/ucrt64/lib -lglib-2.0 -lintl -liconv

# Debug / Release
CONFIG(debug, debug|release) {
    DEFINES += DEBUG
}
CONFIG(release, debug|release) {
    DEFINES += NDEBUG
}