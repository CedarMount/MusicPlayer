QT += quick multimedia widgets sql

# MinGW：强制使用 Qt 安装目录下的配套工具链。若 PATH 里另有较新的 g++（如独立安装的 GCC 12+），
# 会编译出对 std::__glibcxx_assert_fail 等 libstdc++ 新符号的引用，而 Qt 5.15 的库仍按旧 ABI 链接，导致 undefined reference。
win32-g++ {
    QT_MINGW_BUNDLED = $$[QT_INSTALL_PREFIX]/../../Tools/mingw810_64
    exists($$QT_MINGW_BUNDLED/bin/g++.exe) {
        message("MusicPlayer: using Qt-bundled MinGW ($$QT_MINGW_BUNDLED)")
        QMAKE_CC = $$QT_MINGW_BUNDLED/bin/gcc.exe
        QMAKE_CXX = $$QT_MINGW_BUNDLED/bin/g++.exe
        QMAKE_LINK = $$QT_MINGW_BUNDLED/bin/g++.exe
        QMAKE_AR = $$QT_MINGW_BUNDLED/bin/ar.exe
        QMAKE_NM = $$QT_MINGW_BUNDLED/bin/nm.exe
        QMAKE_OBJCOPY = $$QT_MINGW_BUNDLED/bin/objcopy.exe
        QMAKE_STRIP = $$QT_MINGW_BUNDLED/bin/strip.exe
        QMAKE_RC = $$QT_MINGW_BUNDLED/bin/windres.exe
    } else {
        warning("未找到 Qt 自带 MinGW: $$QT_MINGW_BUNDLED — 请从「Qt 5.15.2 MinGW 64-bit」环境的命令行编译，或把该 Tools\\mingw810_64\\bin 置于 PATH 最前，避免混入其它 g++。")
    }
}

# MSVC 默认按系统代码页解析源码，易导致 tr()/QStringLiteral 中文乱码；统一按 UTF-8 编译
win32-msvc* {
    QMAKE_CXXFLAGS += /utf-8
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        musiccontroller.cpp \
        trackfiltermodel.cpp \
        tracklistmodel.cpp \
        usersession.cpp \
        usertracksmodels.cpp

HEADERS += \
        musiccontroller.h \
        trackfiltermodel.h \
        tracklistmodel.h \
        usersession.h \
        usertracksmodels.h

RESOURCES += qml.qrc \
    res.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
