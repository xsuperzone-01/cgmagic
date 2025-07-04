#include "installprocess.h"

#include <QDir>
#include <QProcess>
#include <QApplication>
#include <QJsonObject>
#include "../msgbox.h"
#include "../xfunc.h"
#include "../basewidget.h"

InstallProcess::InstallProcess() :
    QObject(NULL)
{

}

void InstallProcess::copyPublicFile(QString appDir, QString installDir)
{
    //这里多此一举了，不知道为啥--目前把所有文件全部拷贝至files.zip中，所以这里可以不需要！！后期优化

    //将安装程序的dll拷贝到客户端安装目录
    QStringList dllDir;
    dllDir << "\\bearer" << "\\iconengines" << "\\imageformats"
              << "\\platforms" << "\\styles";
    foreach (QString d, dllDir) {
        BaseWidget::copyDirectoryFiles(appDir + d, installDir + d, true);
    }

    QStringList dll;
    dll <<"\\D3Dcompiler_47.dll" <<"\\libEGL.dll" <<"\\libGLESv2.dll"
          <<"\\opengl32sw.dll" <<"\\Qt5Core.dll" <<"\\Qt5Gui.dll"
            <<"\\Qt5Network.dll" <<"\\Qt5Svg.dll" <<"\\Qt5Widgets.dll"
        << "\\msvcp140.dll" << "\\msvcp140_1.dll" << "\\msvcp140_2.dll" << "\\vcruntime140.dll" << "\\CitrixWorkspaceApp.exe";
    foreach (QString d, dll) {
        QFile::copy(appDir + d, installDir + d);
    }

    emit copyPublicFileSuccess();

    deleteLater();
}

void InstallProcess::clientSet(QJsonObject obj)
{
    instPath = obj.value("instPath").toString();
    appPath = obj.value("appPath").toString();
    exe = obj.value("exe").toString();
    name = obj.value("name").toString();
    nameEN = obj.value("nameEN").toString();
    version = obj.value("version").toString();
    third = obj.value("third").toString();
    lang = obj.value("lang").toString();
    mark = obj.value("mark").toString();
    namedef = obj.value("namedef").toString();
    startMenu = obj.value("startMenu").toBool();
    autoRun = obj.value("autoRun").toBool();
    cn = obj.value("cn").toBool();

    QString src = instPath + "\\" + "cgmagic.exe";
    QString dest = instPath + "\\" + exe;
    if(src != dest) {
        if (!QFile::rename(src, dest)) {
            MsgTool::msgOkLoop(cn ? "重命名失败" : "Rename file error");
            exit(0);
        }
    }

    QString insExe = QDir::toNativeSeparators(QDir(instPath).filePath(exe));
    QString uninsExe = QDir::toNativeSeparators(QDir(instPath).filePath("uninstall.exe"));

    //拷贝图标
    QFile::copy(appPath + "/XR.ico", instPath + "/XR.ico");
    QFile::copy(appPath + "/XR.ini", instPath + "/XR.ini");

    //先删旧的 如果有的话
    if (!name.isEmpty()) {
        QString dir = BaseWidget::stdAppPath() + "/" + name;
        QDir startDir(dir);
        if (startDir.exists())
            BaseWidget::veryDel(dir);
        BaseWidget::veryDel(BaseWidget::stdDeskPath() + "/" + QString("%1.lnk").arg(name));
    }

    if (!nameEN.isEmpty()) {
        QString dir = BaseWidget::stdAppPath() + "/" + nameEN;
        QDir startDir(dir);
        if (startDir.exists())
            BaseWidget::veryDel(dir);
        BaseWidget::veryDel(BaseWidget::stdDeskPath() + "/" + QString("%1.lnk").arg(nameEN));
    }

    //-是否添加开始菜单
    QString startMenuPath = BaseWidget::stdAppPath() + "/" + namedef;
    if (startMenu) {
        QDir().mkdir(startMenuPath);
        QFile::link(insExe, startMenuPath + "/" + QString("%1.lnk").arg(QString("%1%2").arg("").arg(namedef)));
        QFile::link(uninsExe, startMenuPath + "/" + QString("%1.lnk").arg(QString("%1%2").arg(cn ? "卸载" : "uninstall ").arg(namedef)));
    }
    //-快捷方式
    QFile::link(insExe, BaseWidget::stdDeskPath() + "/" + QString("%1.lnk").arg(namedef));

    emit pro();

    //-注册表
    //是否开机启动
    if (autoRun) {
        QSettings* m_autoRunSet = BaseWidget::m_autoRunSet;
        m_autoRunSet->setValue(exe, insExe + " RegAutoRun");
    }

    //卸载信息
    QString uninsExeAddQutos = "\"" + uninsExe + "\"";
    QSettings* m_uninstSet = BaseWidget::m_uninstSet;
    m_uninstSet->beginGroup(BaseWidget::thirdGroup());
    m_uninstSet->setValue("DisplayName", namedef);
    m_uninstSet->setValue("UninstallString", uninsExeAddQutos);  //由于默认安装是CG MAGIC；当在磁盘根目录下安装时，需要添加双引号在注册表中才行
    m_uninstSet->setValue("installDir", instPath);
    qDebug()<<"m_uninstSet值:installDir--第一部分"<<m_uninstSet->value("installDir").toString();
    m_uninstSet->setValue("DisplayVersion", version);
    if (m_uninstSet->status() == QSettings::NoError) {
        qDebug() << "clientSet方法中m_uninstSet注册表写入成功";
    } else {
        qDebug() << "clientSet方法中m_uninstSet注册表写入失败";
    }
    if (third.isEmpty()) {
        m_uninstSet->setValue("Publisher", "江苏赞奇科技股份有限公司");
    }
    m_uninstSet->setValue("DisplayIcon", insExe);
    m_uninstSet->endGroup();

    emit pro();

    qDebug()<<"third is:"<<third;
    //client自定义
    QSettings* m_clientSet = BaseWidget::clientSet();
    qDebug()<<"配置文件m_clientSet的内存:"<<m_clientSet;
    m_clientSet->setValue("ThirdPart", third);
    m_clientSet->setValue("Language", lang);
    m_clientSet->setValue("InstallDir", instPath);
    qDebug()<<"m_clientSet值:InstallDir--第一部分"<<m_clientSet->value("InstallDir").toString();
    QJsonObject ino;
    ino.insert("third", third);
    ino.insert("name", name);
    ino.insert("nameEN", nameEN);
    ino.insert("exe", exe);
    ino.insert("mark", mark);
    ino.insert("lang", lang);

    //由于上述再二次覆盖安装时，无法写入注册表；因此，进行兼容性覆盖
    if (m_clientSet->status() == QSettings::NoError) {
        qDebug() << "m_clientSetNewWrite注册表写入成功";

        m_clientSet->setValue("info", XFunc::jsonObjToStr(ino));
    } else {
        qDebug() << "m_clientSetNewWrite注册表写入失败";
        QSettings* m_clientSetNewWrite = BaseWidget::clientSetNewWrite();
        m_clientSetNewWrite->setValue("ThirdPart", third);
        m_clientSetNewWrite->setValue("Language", lang);
        m_clientSetNewWrite->setValue("InstallDir", instPath);
        qDebug()<<"m_clientSetNewWrite值:InstallDir--直接添加值"<<m_clientSetNewWrite->value("InstallDir").toString();

        m_clientSetNewWrite->setValue("info", XFunc::jsonObjToStr(ino));
    }

    emit pro();

    {
        QProcess pro;
        pro.start(insExe, QStringList()<< "-InstallDir");
        pro.waitForFinished();
    }

    emit pro();

    {
        QProcess pro;
        pro.start(insExe, QStringList()<< "-ms");
        pro.waitForFinished();
    }

    emit pro();

    //将安装目录的权限提升 方便读写
    {
        QDir gd(instPath);
        // gd.cdUp();   //针对上层目录，多余
        QProcess ic;
        ic.start("icacls", QStringList() << gd.absolutePath() << "/grant"<<"EveryOne:F" << "/t" << "/C");
        ic.waitForFinished();
    }

    emit installClientSetSuccess();

    deleteLater();
}

