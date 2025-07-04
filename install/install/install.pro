#-------------------------------------------------
#
# Project created by QtCreator 2018-01-15T17:07:40
#
#-------------------------------------------------

QT       += core gui network
QT       += core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TRANSLATIONS += zh_cn.ts\
                en_us.ts

TARGET = install
TEMPLATE = app

INCLUDEPATH += "."

QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"
QMAKE_PROJECT_DEPTH = 0

SOURCES += main.cpp\
    ../baselineedit.cpp \
    ../threadpool.cpp \
    installprocess.cpp \
        widget.cpp \
    quazip/JlCompress.cpp \
    quazip/qioapi.cpp \
    quazip/quazip.cpp \
    quazip/quazipfile.cpp \
    quazip/quazipfileinfo.cpp \
    quazip/quazipnewinfo.cpp \
    quazip/unzip.c \
    quazip/zip.c \
    zipthread.cpp \
    ../basewidget.cpp \
    ../msgbox.cpp \
    ../singleapp.cpp \
    ../xfunc.cpp

HEADERS  += widget.h \
    ../baselineedit.h \
    ../threadpool.h \
    installprocess.h \
    quazip/crypt.h \
    quazip/ioapi.h \
    quazip/JlCompress.h \
    quazip/quazip.h \
    quazip/quazip_global.h \
    quazip/quazipfile.h \
    quazip/quazipfileinfo.h \
    quazip/quazipnewinfo.h \
    quazip/unzip.h \
    quazip/zconf.h \
    quazip/zip.h \
    quazip/zlib.h \
    zipthread.h \
    ../basewidget.h \
    ../msgbox.h \
    ../singleapp.h \
    ../xfunc.h

FORMS    += widget.ui \
    ../msgbox.ui

DEFINES += INSTALL
RC_FILE += ../../subject/exe.rc

LIBS += "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64/WbemUuid.Lib"#for reading video card info by WMI
LIBS += "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64/Version.Lib"

LIBS += -ladvapi32 -lShell32

DEFINES += QUAZIP_BUILD

QMAKE_LFLAGS_RELEASE += /MAP
QMAKE_CXXFLAGS_RELEASE += /Zi
QMAKE_CXXFLAGS_RELEASE += /Od
QMAKE_LFLAGS_RELEASE += /debug /opt:ref

RESOURCES += \
    resource/resource.qrc
