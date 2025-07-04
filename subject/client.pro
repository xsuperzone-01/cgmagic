QT       += core gui xml
QT       += network
QT       += sql
QT       += webenginewidgets concurrent
QT       += core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CODECFORTR = utf8
TRANSLATIONS += zh_cn.ts
TRANSLATIONS += en_us.ts

CONFIG += resources_big
CONFIG += c++11
INCLUDEPATH += "."
#单例
#DEFINES += SINGLE

#------win------#
win32 {
#加快编译速度
#CONFIG += precompile_header
#PRECOMPILED_HEADER = ../XSuperZone/stable.h
#release模式生成pdb
QMAKE_LFLAGS_RELEASE += /MAP
QMAKE_CXXFLAGS_RELEASE += /Zi
QMAKE_CXXFLAGS_RELEASE += /Od
QMAKE_LFLAGS_RELEASE += /debug /opt:ref /FORCE:MULTIPLE
QMAKE_LFLAGS_WINDOWS += /LARGEADDRESSAWARE
QMAKE_PROJECT_DEPTH = 0
##编译
#src_exe = $$OUT_PWD/release/XRenderYS.exe
#src_pdb = $$OUT_PWD/release/XRenderYS.pdb
#    src_exe ~= s,/,\\,g
#    dst_exe ~= s,/,\\,g
#QMAKE_POST_LINK += copy $$src_exe
#QMAKE_POST_LINK += copy $$src_pdb
#QMAKE_POST_LINK += call ../rc.bat
TARGET=cgmagic
#开发库
LIBS += \
    "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64/Imm32.Lib" \#English input method(login UI)
    "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64/WbemUuid.Lib"#for reading video card info by WMI
LIBS += "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64/dxguid.lib"
LIBS += "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64/Dbghelp.lib"
LIBS += "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64/Version.Lib"
LIBS += -ladvapi32 -lShell32
#其他
RC_FILE += \
    exe.rc
DEFINES += OS_WIN
}

#------mac------#
unix:macx {
ICON = mac.icns
QMAKE_INFO_PLIST += Info.plist
TARGET=xdemo
}

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS


# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    base/baseheader.cpp \
    changeMax/change.cpp \
    changeMax/choosemaxversion.cpp \
    changeMax/defaulttablepage.cpp \
    changeMax/progress.cpp \
    common/baseclickwidget.cpp \
    common/basewidget.cpp \
    common/buttongroup.cpp \
    common/clearmax.cpp \
    common/debounce.cpp \
    common/eventfilter.cpp \
    common/flowlayout.cpp \
    common/headview.cpp \
    common/protocol.cpp \
    common/pullmsg.cpp \
    common/session.cpp \
    common/tablewidgetdropfile.cpp \
    common/trayicon.cpp \
    common/widgetgroup.cpp \
    config/userinfo.cpp \
    db/downloaddao.cpp \
    db/downloadmaxdb.cpp \
    db/uploaddao.cpp \
    db/userconfig.cpp \
    db/userdao.cpp \
    io/netdown.cpp \
    io/pluginlisten.cpp \
    main.cpp \
    plugin/plugincorrespond.cpp \
    quazip/JlCompress.cpp \
    quazip/qioapi.cpp \
    quazip/quazip.cpp \
    quazip/quazipfile.cpp \
    quazip/quazipfileinfo.cpp \
    quazip/quazipnewinfo.cpp \
    quazip/unzip.c \
    quazip/zip.c \
    tool/crash.cpp \
    tool/machineprofiles.cpp \
    tool/regedit.cpp \
    tool/winutil.cpp \
    transfer/down/downhandler.cpp \
    transfer/down/downwork.cpp \
    transfer/sessiontimer.cpp \
    transfer/transfer.cpp \
    transfer/transhandler.cpp \
    transfer/transscan.cpp \
    transfer/transset.cpp \
    transfer/up/uphandler.cpp \
    transfer/up/upwork.cpp \
    transferMax/downloadinfos.cpp \
    transferMax/downloadmaxset.cpp \
    transferMax/downloadprocess.cpp \
    view/Item/combobox.cpp \
    view/Item/menu.cpp \
    view/Item/transferhand.cpp \
    view/about.cpp \
    view/headsearch.cpp \
    tool/base64.cpp \
    tool/childprocess.cpp \
    tool/jsonutil.cpp \
    tool/msgtool.cpp \
    tool/network.cpp \
    tool/singleapp.cpp \
    tool/webtool.cpp \
    tool/webview.cpp \
    tool/xfunc.cpp \
    versions/versionmanager.cpp \
    view/Item/baselabel.cpp \
    view/Item/baselineedit.cpp \
    view/Item/widget.cpp \
    view/backgroundmask.cpp \
    view/firstrun.cpp \
    view/leftnavitem.cpp \
    view/Item/baseitemdelegate.cpp \
    view/Item/checkboxitem.cpp \
    view/account.cpp \
    view/login.cpp \
    view/mainwindow.cpp \
    view/msgbox.cpp \
    view/pagination.cpp \
    view/pluginmanager.cpp \
    view/preview.cpp \
    view/set/envblock.cpp \
    view/set/envnew.cpp \
    view/set/envset.cpp \
    view/set/maxset.cpp \
    view/set/renderset.cpp \
    view/set/resultrule.cpp \
    view/set/set.cpp \
    view/set/setting.cpp \
    view/set/setupgrader.cpp \
    view/set/trayset.cpp \
    view/set/traysetaccount.cpp \
    view/set/updatecontent.cpp \
    view/set/updateitem.cpp \
    view/set/updateset.cpp \
    view/softwareupdate.cpp \
    windows/activitywidget.cpp \
    windows/downloadmaxwidget.cpp \
    windows/noticewidget.cpp \

HEADERS += \
    base/baseheader.h \
    changeMax/change.h \
    changeMax/choosemaxversion.h \
    changeMax/defaulttablepage.h \
    changeMax/progress.h \
    common/BaseScale.h \
    common/Singleton.h \
    common/baseclickwidget.h \
    common/basewidget.h \
    common/buttongroup.h \
    common/clearmax.h \
    common/debounce.h \
    common/eventfilter.h \
    common/flowlayout.h \
    common/headview.h \
    common/protocol.h \
    common/pullmsg.h \
    common/session.h \
    common/tablewidgetdropfile.h \
    common/trayicon.h \
    common/widgetgroup.h \
    config/userinfo.h \
    db/downloaddao.h \
    db/downloadmaxdb.h \
    db/uploaddao.h \
    db/userconfig.h \
    db/userdao.h \
    io/netdown.h \
    io/pluginlisten.h \
    plugin/plugincorrespond.h \
    quazip/JlCompress.h \
    quazip/crypt.h \
    quazip/ioapi.h \
    quazip/quazip.h \
    quazip/quazip_global.h \
    quazip/quazipfile.h \
    quazip/quazipfileinfo.h \
    quazip/quazipnewinfo.h \
    quazip/unzip.h \
    quazip/zconf.h \
    quazip/zip.h \
    quazip/zlib.h \
    tool/crash.h \
    tool/machineprofiles.h \
    tool/regedit.h \
    tool/winutil.h \
    transfer/down/downhandler.h \
    transfer/down/downwork.h \
    transfer/sessiontimer.h \
    transfer/transfer.h \
    transfer/transhandler.h \
    transfer/transscan.h \
    transfer/transset.h \
    transfer/up/uphandler.h \
    transfer/up/upwork.h \
    transferMax/downloadinfos.h \
    transferMax/downloadmaxset.h \
    transferMax/downloadprocess.h \
    view/Item/combobox.h \
    view/Item/menu.h \
    view/Item/transferhand.h \
    view/about.h \
    view/defaulttablepage.h \
    view/headsearch.h \
    tool/base64.h \
    tool/childprocess.h \
    tool/jsonutil.h \
    tool/msgtool.h \
    tool/network.h \
    tool/singleapp.h \
    tool/webtool.h \
    tool/webview.h \
    tool/xfunc.h \
    version.h \
    versions/versionmanager.h \
    view/Item/baselabel.h \
    view/Item/baselineedit.h \
    view/Item/widget.h \
    view/backgroundmask.h \
    view/firstrun.h \
    view/leftnavitem.h \
    view/Item/baseitemdelegate.h \
    view/Item/checkboxitem.h \
    view/account.h \
    view/login.h \
    view/mainwindow.h \
    view/msgbox.h \
    view/pagination.h \
    view/pluginmanager.h \
    view/preview.h \
    view/set/envblock.h \
    view/set/envnew.h \
    view/set/envset.h \
    view/set/maxset.h \
    view/set/renderset.h \
    view/set/resultrule.h \
    view/set/set.h \
    view/set/setting.h \
    view/set/setupgrader.h \
    view/set/trayset.h \
    view/set/traysetaccount.h \
    view/set/updatecontent.h \
    view/set/updateitem.h \
    view/set/updateset.h \
    view/softwareupdate.h \
    windows/activitywidget.h \
    windows/downloadmaxwidget.h \
    windows/noticewidget.h \

FORMS += \
    changeMax/change.ui \
    changeMax/choosemaxversion.ui \
    changeMax/defaulttablepage.ui \
    changeMax/progress.ui \
    transfer/transfer.ui \
    view/Item/transferhand.ui \
    view/about.ui \
    view/headsearch.ui \
    view/login.ui \
    view/backgroundmask.ui \
    view/firstrun.ui \
    view/leftnavitem.ui \
    view/account.ui \
    view/mainwindow.ui \
    view/msgbox.ui \
    view/pagination.ui \
    view/preview.ui \
    view/set/envblock.ui \
    view/set/envnew.ui \
    view/set/envset.ui \
    view/set/maxset.ui \
    view/set/renderset.ui \
    view/set/resultrule.ui \
    view/set/set.ui \
    view/set/setting.ui \
    view/set/setupgrader.ui \
    view/set/traysetaccount.ui \
    view/set/updatecontent.ui \
    view/set/updateitem.ui \
    view/set/updateset.ui \
    view/softwareupdate.ui \
    windows/activitywidget.ui \
    windows/downloadmaxwidget.ui \
    windows/noticewidget.ui \

UI_DIR        += zzz_ui_dir
RCC_DIR       += zzz_rcc_dir
MOC_DIR       += zzz_moc_dir
OBJECTS_DIR   += zzz_objects_dir

DEFINES += QUAZIP_BUILD

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource/resource.qrc

DISTFILES += \
    resource/bottomlogo.png \
    resource/closeBtn.png \
    resource/closeBtn_hover.png \
    resource/closeBtn_pressed.png
