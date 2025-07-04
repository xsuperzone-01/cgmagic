#include "view/mainwindow.h"
#include <QTranslator>
#include <QIcon>
#include <QSplashScreen>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QTimer>
#include <config/userinfo.h>
#include <tool/xfunc.h>
#include <tool/singleapp.h>
#include <common/session.h>

#include <QJsonObject>
#include "common/trayicon.h"
#include "db/userdao.h"
#include "tool/jsonutil.h"
#include "versions/versionmanager.h"
#include "tool/network.h"
#include <QFontDatabase>

#include "version.h"
#include <QSurfaceFormat>
#include "common/eventfilter.h"
#include "tool/msgtool.h"
#include <QSslSocket>
#include "tool/crash.h"
#include "view/mainwindow.h"
#include "transfer/sessiontimer.h"
#include "common/session.h"
#include "tool/regedit.h"
#include "view/pluginmanager.h"
#include "view/set/set.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
#endif

    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);

#ifdef Q_OS_MAC
    //macOS升级到BigSu，Qt应用程序未响应
    //加了会导致 边框阴影在拖动时变黑
    qputenv("QT_MAC_WANTS_LAYER", "1");
#endif

    SingleApp a(argc, argv);

    QString appName = QObject::tr("CG MAGIC");
    qApp->setApplicationDisplayName(appName);
#ifdef Q_OS_MAC
    //如果Info.plist中定义了CFBundleName，则qApp->applicationName会变成CFBundleName的值
    qApp->setApplicationName("xdemo");
#endif

    //日志输出
    qInstallMessageHandler(XFunc::msgHandler);

    QCommandLineOption op1("L");
    op1.setValueName("path");
    QCommandLineOption op2("A");
    op2.setValueName("0/1:自启动");
    QCommandLineOption op3("U");
    op3.setValueName("notupdate");
    QCommandLineOption op4("R");
    op4.setValueName("path");
    QCommandLineOption op5("D");
    op5.setValueName("path");
    QCommandLineOption ms("ms");
    QCommandLineOption dms("dms");
    QCommandLineOption InstallDir("InstallDir");

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addHelpOption();  // 添加帮助选项 （"-h" 或 "--help"）
    parser.addVersionOption();  // 添加版本选项 ("-v" 或 "--version")
    parser.addOptions(QList<QCommandLineOption>()<< op1 << op2 << op3 << op4 << op5 << dms << ms << InstallDir);
    parser.process(a); // 异常 Unexpected value after '-a'.
    if (parser.isSet(op2)) {
        UserInfo::instance()->AutoRun(parser.value(op2).toInt());
        return 0;
    }

    if (parser.isSet(op5)) {
        XFunc::veryDel(UserInfo::instance()->appDataPath());
        return 0;
    }

    if (parser.isSet(ms)) {
        PluginManager pm;
        pm.repairPlugin(true);
        return 0;
    }

    if (parser.isSet(dms)) {
        PluginManager pm;
        pm.removePlugins();
        USERINFO->setFirstRun(0);
        Set::setLoginSilent(false);
        Set::setLoginHide2(false);
        return 0;
    }

    if (parser.isSet(InstallDir)) {
        QString root = ".DEFAULT\\Software\\Xsuperzone\\Xcgmagic";
        RegEdit::setUS(root, "InstallDir", QDir::toNativeSeparators(qApp->applicationDirPath() + "/"));
        RegEdit::setUS(root, "pluginDir", QDir::toNativeSeparators(qApp->applicationDirPath() + "/mobao/"));

        PVOID OldValue = NULL;
        Wow64DisableWow64FsRedirection(&OldValue);
        QProcess::startDetached("ie4uinit -show");
        Wow64RevertWow64FsRedirection(OldValue);
        return 0;
    }

    if (parser.isSet(op1) || parser.isSet(op4)) {
        //重登 延时1s
        QPointer<QEventLoop> loop = new QEventLoop;
#ifdef Q_OS_WIN
        QTimer::singleShot(1000, loop, SLOT(quit()));
#else
        QTimer::singleShot(3000, loop, SLOT(quit()));
#endif
        loop->exec();
        loop->deleteLater();
    }

    if(parser.isSet(op1)){
        //本步骤是更新重启登录时删除resources目录下的全部文件--旧QT5文件；之后的资源文件存于安装总目录而非子目录
        QString resourcesDirPath =QCoreApplication::applicationDirPath() + "/resources";
        qDebug()<<"resourcesDirPath is:"<<resourcesDirPath;
        QDir dir(resourcesDirPath);
        if (!dir.exists()) {
            qDebug() << "ResourcesDir does not exist: " << resourcesDirPath;
        }else{
            QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            if (entries.isEmpty()) {
                qDebug() << "ResourcesDir is empty: " << resourcesDirPath;
            }else{
                foreach (const QString &entry, entries) {
                    QString entryPath = dir.absoluteFilePath(entry);
                    QFileInfo entryInfo(entryPath);

                    if (entryInfo.isFile()) {
                        if (dir.remove(entryPath)) {
                            qDebug() << "Deleted file: " << entry;
                        } else {
                            qDebug() << "Failed to delete file: " << entry;
                        }
                    } else{
                        QDir subDir(entryPath);
                        if (subDir.removeRecursively()) {
                            qDebug() << "Deleted directory: " << entry;
                        } else {
                            qDebug() << "Failed to delete directory: " << entry;
                        }
                    }
                }
            }
        }
    }else{
        qDebug()<<"Normal login";
    }

    //debug
#ifdef Q_OS_WIN
    Crash::dmpPath = UserInfo::instance()->dmpPath();
    Crash::version = CLIENT_VERSION;
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)Crash::applicationCrashHandler);
#endif

    //清理日志
    XFunc::clearLogFiles();
    QTimer clf;
    clf.start(3600*1000);
    QObject::connect(&clf, &QTimer::timeout, &a, [=]{
        XFunc::clearLogFiles();
    });

#ifdef Q_OS_MAC
   MacUtil::setupDockClickEvent();
#endif

    //语言
    QTranslator tran;
    QString lan = Set::changeLan();
//    lan = "en_us";
    bool ret = tran.load(QString(":/language/%1.qm").arg(lan));
    if (ret) {
        a.installTranslator(&tran);//安装语言
    }

    int tra;
    if (lan.isEmpty()) {
        lan = "zh_cn";
    }
    if (lan == "zh_cn") {
        tra = UserInfo::language_cn;
    } else if (lan == "en_us") {
        tra = UserInfo::language_en;
    } else {
        tra = UserInfo::language_cn;
    }

    //全局样式表
    qApp->setStyleSheet(Session::instance()->globalStyle(tra));//0->tra
    qApp->setWindowIcon(QIcon(QString(":/third/%1/online.ico").arg(UserInfo::instance()->thirdGroup())));

    QString fontFamily= "";
    if(lan == "en_us"){
       fontFamily = "Arial";
    }else{
       fontFamily = "Microsoft Yahei";
    #ifdef Q_OS_MAC
        fontFamily = ".PingFang SC";
    #endif
    }

    qDebug()<< fontFamily;

    QFont font;
    font.setFamily(fontFamily);
    font.setPixelSize(qRound(12 * UserInfo::instance()->getScaleW()));
    qApp->setFont(font);
    qDebug()<< qApp->font();

    //信号槽 参数注册
    Session::instance()->setRegisterMetaType();

    if(!a.Single(parser.isSet(op1) || parser.isSet(op4)))
    {
        bool exit = true;
        if (exit) {
            qDebug()<< "已经存在实例，此exe退出";
            return 0;
        }
    }

    qDebug() << QString("---------------New Run v%1---------------").arg(CLIENT_VERSION);
    qDebug() << qApp->applicationPid() << qApp->applicationDirPath() << qApp->applicationName();
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    qDebug()<<"全局屏蔽代理：QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);";


#ifdef Q_OS_WIN
    QStringList adminArgs;
    if (!adminArgs.isEmpty())
        XFunc::runAsAdmin(qApp->applicationFilePath(), adminArgs.join(" "));
#endif

    bool showLogin = true;
    //testupdate
    if (parser.isSet(op3) && parser.value(op3) == "notupdate")
        qDebug() << "notupdate";
    else
        showLogin = VersionManager::instance()->initVesions(parser.value(op3) == "ignoreFirst");
    QObject::connect(a.server(), &QLocalServer::newConnection, Session::instance()->LoginWid(), &Login::onSingleConnected);

    if (showLogin) {
        if (parser.isSet(op1)) {
            Session::instance()->LoginWid()->initLogin(Login::UpdateAutoLogin, JsonUtil::jsonStrToObj(parser.value(op1)));
        } else if(parser.isSet(op4)) {
            Session::instance()->LoginWid()->initLogin(parser.value(op4).toInt());
        } else {
            Session::instance()->LoginWid()->initLogin();
        }
    }

    QObject::connect(&a, SIGNAL(showUp()), TrayIcon::instance(), SLOT(showCurWid()));

    //把更新程序安装插件的功能移到主程序
    PluginManager pm;
    pm.repairPlugin();

    int exec = a.exec();
    qDebug() << __FUNCTION__ << "exec =" << exec << qApp->applicationPid();
    XFunc::appQuit = true;
    if (exec == 0) {    //正常退出
    } else if (exec == 1) {     // 软件更新重启

    }

    delete VersionManager::instance();
    delete TrayIcon::instance();
    delete UserDAO::instance();
    delete UserInfo::instance();

    return exec;
}
