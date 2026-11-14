QT += core gui widgets network

CONFIG += c++17

# 应用名与产物
TARGET = PicRope
TEMPLATE = app
VERSION = 1.1.1

DEFINES += QT_DEPRECATED_WARNINGS

# 使用 /utf-8 处理源码中的中文字符串（MSVC 需要；MinGW 默认 utf-8）
win32-msvc*: QMAKE_CXXFLAGS += /utf-8

# 目录
INCLUDEPATH += $$PWD system core ui

SOURCES += \
    main.cpp \
    app.cpp \
    system/win32.cpp \
    system/clipboard.cpp \
    core/state.cpp \
    core/image.cpp \
    core/gallery.cpp \
    core/texts.cpp \
    core/capture.cpp \
    core/updater.cpp \
    ui/themes.cpp \
    ui/card.cpp \
    ui/shelf.cpp \
    ui/pinned.cpp \
    ui/panels.cpp \
    ui/settings.cpp \
    ui/viewer.cpp \
    ui/tray.cpp

HEADERS += \
    app.h \
    system/win32.h \
    system/clipboard.h \
    core/state.h \
    core/image.h \
    core/gallery.h \
    core/texts.h \
    core/capture.h \
    core/updater.h \
    ui/themes.h \
    ui/card.h \
    ui/shelf.h \
    ui/pinned.h \
    ui/panels.h \
    ui/settings.h \
    ui/viewer.h \
    ui/tray.h

RESOURCES += resources.qrc

# Windows：把 app.ico 编入 exe（资源管理器/快捷方式/安装包显示的图标）
win32: RC_ICONS = app.ico

# Windows 平台额外链接
win32: LIBS += -luser32 -lgdi32 -lshell32 -lshcore -ldwmapi -lole32
