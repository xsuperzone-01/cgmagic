#include "versionmanager.h"
#include "common/protocol.h"
#include "tool/jsonutil.h"
#include "tool/network.h"
#include "common/session.h"
#include <QDebug>
#include <QProcess>
#include "version.h"
#include "config/userinfo.h"
#include "tool/xfunc.h"
#include "tool/msgtool.h"
#include "common/trayicon.h"

PATTERN_SINGLETON_IMPLEMENT(VersionManager);

QString CLIENT_ID = "40";
QString PRODUCT_ID = "15";

VersionManager::VersionManager(QObject *parent):
    QObject(parent),
    m_softUpdateWid(NULL)
{
    m_timer.setInterval(60 * 1000);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));

    m_root = qApp->applicationDirPath();
    m_host = "http://update.xrender.com";
    m_branch = "windows";
#ifdef Q_OS_MAC
    m_root.chop(QString("MacOS").length());
    m_branch = "mac";
#endif
    m_profile = USERINFO->profiles();

    if (USERINFO->isOS()) {
        CLIENT_ID = "36";
        PRODUCT_ID = "12";
        m_host = "http://update.xrender.cloud";
    }

}

VersionManager::~VersionManager()
{
    if (m_timer.isActive()) {
        m_timer.stop();
    }
    //延迟100ms，确保退出命令发出了
    QPointer<QEventLoop> loop = new QEventLoop;
    QTimer::singleShot(100, loop, SLOT(quit()));
    NET->post(updateExitUrl, "", [&](FuncBody f) {
        if (!loop) return;
        loop->quit();
    }, this);
    loop->exec();
    loop->deleteLater();
}

bool VersionManager::initVesions(bool ignoreFirst, QString proxy)
{
    m_proxy = proxy;

    if (!ignoreFirst) {
        qDebug()<<"开启更新检测";
        check();
        popup();
    }

    //如果需要用admin启动
    if (m_softUpdateWid && m_softUpdateWid->admin())
        return false;

    m_pro = new QProcess(this);
    m_pro->setWorkingDirectory(m_root);
    m_pro->start(exe(), args());
    //goland命令行调试参数：-ginPort=47162 -Brance=windows -ProductId=7 -Profiles=test -isadmin=true -AppVersion="1.0.31"

    qDebug()<< __FUNCTION__ << exe() << args() << m_pro->error() << m_pro->errorString();

    connect(m_pro, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug()<< "cgmagicUpgrader.exe exit" << exitCode << exitStatus;
        QTimer::singleShot(1000, [=](){
            m_pro->start(exe(), args());
        });
    });

    qDebug() << "start softUpdate.exe success" << ginPort;
    m_timer.start();
    return m_softUpdateWid == NULL;
}

/*
    只管主程序的版本
*/
void VersionManager::check()
{
    m_client = QJsonObject();
    plugins.clear();

    //提前校验版本 softwareId=23 softwareTag=xdemo客户端
    QPointer<QObject> ptr = this;
    FuncBody f = NET->sync(GET, QString("%5/release-version/product?productId=%1&branch=%2&profile=%3&stamp=%4")
                            .arg(PRODUCT_ID).arg(m_branch).arg(m_profile).arg(QDateTime::currentSecsSinceEpoch()).arg(m_host), "", 10);
    if (!ptr)
        return;

    QJsonArray softs = f.j.value("data").toObject().value("softwares").toArray();
    //目前列表是按版本升序
    foreach (QJsonValue v, softs) {
        QJsonObject o = v.toObject();
        detail(o);
        if (o.value("softwareId").toInt() == CLIENT_ID.toInt()) {
            m_client = o;
        } else {
            plugins << o;
        }
    }
}

QString VersionManager::newVersion()
{
    return m_client.value("version").toString();
}

QString VersionManager::detail()
{
    return m_client.value("detail").toString();
}

void VersionManager::detail(QJsonObject &obj)
{
    QString policyUrl = obj.value("policyUrl").toString();
    if (!policyUrl.isEmpty()) {
        QString readmeUrl = QString(policyUrl).replace("policy.json", "readme.txt");
        FuncBody f = NET->sync(GET, readmeUrl, "", 10);
        QString text = splitDetail(QString(f.b));
        obj.insert("detail", text);
    }
}

QString VersionManager::splitDetail(QString detail)
{
    QJsonObject obj = JsonUtil::jsonStrToObj(detail);
    if (!obj.isEmpty()) {
        QString key = "cn";
        switch (USERINFO->appLangNum()) {
        case UserInfo::language_en:
            key = "en";
            break;
        default:
            break;
        }
        detail = obj.value(key).toString();
    }
    return detail;
}

QJsonObject VersionManager::client()
{
    return m_client;
}

QString VersionManager::clientTime(QJsonObject obj)
{
    QString mt = obj.value("modifiedTime").toString();
    if (!mt.isEmpty()) {
        QStringList mL = mt.split(" ");
        if (!mL.isEmpty()) {
            mt = mL.first();
            mt.replace("-", ".");
        }
    }
    return mt;
}

QString VersionManager::exe()
{
    return XFunc::exe(m_root + "/cgmagicUpgrader.exe");
}

QStringList VersionManager::args()
{
    QStringList argL;
    argL << QString("-host=%1").arg(m_host);
    argL << QString("-ginPort=%1").arg(ginPort);
    argL << QString("-Brance=%1").arg(m_branch);
    argL << QString("-ProductId=%1").arg(PRODUCT_ID);
    argL << QString("-Profiles=%1").arg(m_profile);
    argL << QString("-isadmin=false");
    argL << QString("-clientId=%1").arg(CLIENT_ID);
    argL << QString("-AppVersion=%1").arg(CLIENT_VERSION);
    if (!m_proxy.isEmpty()) {
        QJsonObject obj;
        obj.insert("proxy", m_proxy);
        argL << QString("-Server=\"%1\"").arg(JsonUtil::jsonObjToStr(obj).replace("\"", "\\\""));
    }
    argL << "-updaterVersion=2";

    return argL;
}

void VersionManager::exitProcess()
{
    if (m_pro) {
        m_pro->deleteLater();
    }
}

void VersionManager::popup()
{
    if (newVersion().isEmpty())
        return;

    if (newVersion() != CLIENT_VERSION) {
        closeTip();
        if (!m_softUpdateWid) {
            QWidget *w = Session::instance()->CurWid();
            qDebug()<<"现在储存的窗口是"<<w;
            m_softUpdateWid = new SoftwareUpdate(w);

            QJsonObject obj;
            QJsonArray arr;
            arr.append(m_client);
            obj.insert("force", arr);
            qDebug()<<"这里是手点的"<<obj;

            m_softUpdateWid->initDetail(obj, false, true);
            connect(m_softUpdateWid, &SoftwareUpdate::destroyed, [=](){
                qDebug()<<"更新窗口已销毁，现在储存的窗口是"<<Session::instance()->CurWid()<<w;
                TrayIcon::instance()->setCurWid(w);
            });
        }
    }
}

void VersionManager::timeout()
{
    if (m_softUpdateWid && !m_softUpdateWid->isHidden())
        return;

    NET->get(listUpdateUrl, [=](FuncBody f) {
        int code = f.j.value("data").toObject().value("code").toInt();
        if (code == 200) {
            QJsonObject data = f.j.value("data").toObject().value("data").toObject();
            m_softs = data;
            QJsonArray arr = data.value("force").toArray();
            if (arr.isEmpty()) {
                closeTip();
                return;
            }
            if (m_ignore == arr)
                return;

            bool isForce = false;
            foreach (QJsonValue v, arr) {
                QJsonObject o = v.toObject();
                if (o.value("force").toBool())
                    isForce = true;
            }

            if (!m_softUpdateWid) {
                QWidget *w = Session::instance()->CurWid();
                qDebug()<<"现在储存的窗口是"<<w;
                m_softUpdateWid = new SoftwareUpdate(w);
                qDebug()<<"这里是自动的"<<data;
                m_softUpdateWid->initDetail(data, false, isForce);
                connect(m_softUpdateWid, &SoftwareUpdate::nextClicked, this, [=]{
                    m_ignore = arr;
                });
                connect(m_softUpdateWid, &SoftwareUpdate::destroyed, [=](){
                    qDebug()<<"更新窗口已销毁，现在储存的窗口是"<<Session::instance()->CurWid()<<w;
                    TrayIcon::instance()->setCurWid(w);
                });
            }
        } else
            qDebug() << "list Update info err:" << f.j;
    }, this);
}

void VersionManager::closeTip()
{
}
