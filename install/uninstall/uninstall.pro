#-------------------------------------------------
#
# Project created by QtCreator 2018-01-23T17:33:18
#
#-------------------------------------------------

QT       += core gui network
QT       += sql
QT       += core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TRANSLATIONS += zh_cn.ts\
                en_us.ts

TARGET = uninstall
TEMPLATE = app

INCLUDEPATH += "."

QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"
QMAKE_PROJECT_DEPTH = 0

SOURCES += main.cpp\
        widget.cpp \
    ../basewidget.cpp \
    ../msgbox.cpp \
    ../singleapp.cpp \
    ../xfunc.cpp

HEADERS  += widget.h \
    ../basewidget.h \
    ../msgbox.h \
    ../singleapp.h \
    ../xfunc.h

FORMS    += widget.ui \
    ../msgbox.ui

DEFINES += UNINSTALL
RC_FILE += ../../subject/exe.rc

LIBS += "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64/WbemUuid.Lib"#for reading video card info by WMI
LIBS += "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64/Version.Lib"
LIBS += -ladvapi32 -lShell32

RESOURCES += \
    resource/resource.qrc

DISTFILES += \
    resource/logo.png
