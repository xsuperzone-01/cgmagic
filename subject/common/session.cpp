#include "session.h"

#include <QAbstractSocket>
#include <QFile>
#include <QThread>
#include <QTranslator>
#include <QWidget>
#include <QWindow>
#include "tool/network.h"

#include <config/userinfo.h>
#include <QProcess>
#include "tool/xfunc.h"
#include "tool/jsonutil.h"
#include "tool/msgtool.h"
#include <QScreen>
#include "tool/singleapp.h"
#include "versions/versionmanager.h"

PATTERN_SINGLETON_IMPLEMENT(Session);


Session::Session(QObject *parent):
    QObject(parent),
    m_debugText(NULL),
    m_mainWid(NULL),
    m_loginWid(NULL),
    m_loginHandled(false)
{

}

Session::~Session()
{
    qDebug() << __FUNCTION__;
    if (m_debugText) {
        delete m_debugText;
    }

    if (m_mainWid) {
        m_mainWid->deleteLater();
    }

    if (m_loginWid) {
        m_loginWid->deleteLater();
    }
}

QString Session::globalStyle(int x)
{
    QFile sf(":/default.qss");

    sf.open(QFile::ReadOnly);
    QByteArray sb = sf.readAll();//读取qss中的内容

    sf.close();
    //如果是英文，添加一个英文的额外样式表
    if (1 == x)
    {
        QFile sf(":/default_en.qss");
        sf.open(QFile::ReadOnly);
        sb += sf.readAll();
        sf.close();
    }
    //移除平台化的样式
#ifdef Q_OS_WIN
    QByteArray r0 = "/*win remove0*/";
    while (sb.contains(r0)) {
        int idx = sb.indexOf(r0);
        if (-1 != idx) {
            QString r1 = "/*win remove1*/";
            int idx2 = sb.indexOf(r1.toUtf8());
            if (-1 != idx2) {
                sb = sb.remove(idx, idx2 + r1.length() - idx);
            }
        }
    }
#endif
    return ScaleSheet(sb, UserInfo::instance()->getScaleW(), UserInfo::instance()->getScaleH());
}

QString Session::ScaleSheet(QString sheet, float scaleW, float scaleH)
{
    QRegularExpression rx("\\d+px", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator i = rx.globalMatch(sheet);
    int index = -1;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        int start = match.capturedStart(); // 获取匹配的起始位置
        QString matchedText = match.capturedTexts().at(0);
        int capLen = matchedText.length() - 2; // 计算匹配文本的长度减去2
        index = start; // 更新索引

        QString snum = sheet.mid(index, capLen);
        // 向前推，获取px类型
        int pxTypeIndex = index - 8;
        if (pxTypeIndex >= 0) {
            QString pxType = sheet.mid(pxTypeIndex, 8);
            int px = 0;
            if (pxType.contains("width")
                    || pxType.contains("left")
                    || pxType.contains("right")) {
                px = qRound(snum.toInt() * scaleW);
            } else if (pxType.contains("height")
                       || pxType.contains("top")
                       || pxType.contains("bottom")){
                px = qRound(snum.toInt() * scaleH);
            } else {
                px = qRound(snum.toInt() * scaleW);
            }

            //计算得到的px值为零，且原先值不为零时，默认设置为1px
            if (px == 0 && snum.toInt() != 0) {
                px = 1;
            }

            snum = QString::number(px);
            sheet.replace(index, capLen, snum);
        } else {
            int px = qRound(snum.toInt() * scaleW);
            //计算得到的px值为零，且原先值不为零时，默认设置为1px
            if (px == 0 && snum.toInt() != 0) {
                px = 1;
            }
            snum = QString::number(px);
            sheet.replace(index, capLen, snum);
        }
        index += snum.length();
        if (index > sheet.size() - 2) {
            break;
        }
    }

    return sheet;
}

QString Session::ScaleSheetPt(QString sheet, float scaleW, float scaleH)
{
    QRegularExpression rx("\\d+pt", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator i = rx.globalMatch(sheet);
    int index = -1;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        int start = match.capturedStart(); // 获取匹配的起始位置
        QString matchedText = match.capturedTexts().at(0);
        int capLen = matchedText.length() - 2; // 计算匹配文本的长度减去2
        index = start; // 更新索引

        QString snum = sheet.mid(index, capLen);
        // 向前推，获取px类型
        int pxTypeIndex = index - 8;
        if (pxTypeIndex >= 0) {
            QString pxType = sheet.mid(pxTypeIndex, 8);
            int px = 0;
            if (pxType.contains("width")
                    || pxType.contains("left")
                    || pxType.contains("right")) {
                px = qRound(snum.toInt() * scaleW);
            } else if (pxType.contains("height")
                       || pxType.contains("top")
                       || pxType.contains("bottom")){
                px = qRound(snum.toInt() * scaleH);
            } else {
                px = qRound(snum.toInt() * scaleW);
            }

            //计算得到的px值为零，且原先值不为零时，默认设置为1px
            if (px == 0 && snum.toInt() != 0) {
                px = 1;
            }

            snum = QString::number(px);
            sheet.replace(index, capLen, snum);
        } else {
            int px = qRound(snum.toInt() * scaleW);
            //计算得到的px值为零，且原先值不为零时，默认设置为1px
            if (px == 0 && snum.toInt() != 0) {
                px = 1;
            }
            snum = QString::number(px);
            sheet.replace(index, capLen, snum);
        }
        index += snum.length();
        if (index > sheet.size() - 2) {
            break;
        }
    }

    return sheet;
}

QString Session::xPng(QString png)
{
    QScreen* screen = qApp->primaryScreen();
    return png;
}

void Session::setRegisterMetaType()
{
    qRegisterMetaType< FuncBack >("FuncBack");
    qRegisterMetaType< QThread* >("QThread");
    qRegisterMetaType< QString >("QString&");
    qRegisterMetaType< QAbstractSocket::SocketError >("QAbstractSocket::SocketError");
    qRegisterMetaType< QVariant >("QVariant");

    qRegisterMetaType< db_upfile >("db_upfile");
    qRegisterMetaType< db_upfile >("db_upfile&");
    qRegisterMetaType< QList<db_upfile> >("QList<db_upfile>");
    qRegisterMetaType< db_downfile >("db_downfile");
    qRegisterMetaType< db_downfile >("db_downfile&");
    qRegisterMetaType< QList<db_downfile> >("QList<db_downfile>");

    qRegisterMetaType< QVector<int> >("QVector<int>");
    qRegisterMetaType< QStringList >("QStringList");
    qRegisterMetaType< QByteArray >("QByteArray&");

    qRegisterMetaType< QJsonObject >("QJsonObject");
    qRegisterMetaType< QWidget* >("QWidget");
    qRegisterMetaType< QPointer<QObject> >("QPoiner<QObject>");
}

void Session::reLogin(int relogin)
{
    if (Login::UserReLogin == relogin) {
        if (!toClose()) {
            return;
        }
    }

    //阻止session过期时多次触发
    QMutexLocker lock(&m_loginMu);
    if (m_loginHandled) {
        qDebug()<< "已经重启了";
        return;
    }
    m_loginHandled = true;

    QString exe = qApp->applicationDirPath() + QString("/%1.exe").arg(qApp->applicationName());
    QProcess pro;
    bool sd = pro.startDetached(XFunc::exe(exe), fillArgs(QStringList()<<QString("-R=%1").arg(relogin)<<QString("-U=ignoreFirst")));
    qDebug()<< __FUNCTION__ << "startDetached" << sd;
    exitApp(0);
}

void Session::proExit(int retcode, bool autoRun)
{
    qDebug() << __FUNCTION__ << retcode;

    // 软件更新重启(自动登录)
    if (retcode == 1) {
        QString exe = qApp->applicationDirPath() + QString("/%1.exe").arg(qApp->applicationName());
        QStringList argL;
        argL << QString("-L=%1").arg(JsonUtil::jsonObjToStr(UserInfo::instance()->getUserInfo()));
        argL << QString("delay");
        if (Session::instance()->LoginWid()->isSsoLogin()) {
            USERINFO->reqSsoCode();
            argL << QString("--scheme=daimao://main?code=%1").arg(USERINFO->ssoCode());
        }

#ifdef Q_OS_MAC
        //2.5.7及以前的版本exe是"呆猫"
        QString version = VersionManager::instance()->newVersion();
        qDebug()<< "mac更换exe?" << version << (version <= "2.5.7" ? 1 : 0);
        if (!version.isEmpty() && version <= "2.5.7") {
            QString old = qApp->applicationDirPath() + QString("/%1").arg("呆猫");
            if (QFile::exists(old))
                exe = old;
        }
#endif

        QProcess pro;
        bool ok = pro.startDetached(XFunc::exe(exe), argL);
        qDebug()<<"更新重启"<<XFunc::exe(exe)<< argL << ok;
        exitApp(retcode);
    } else if (retcode == 2) {      // 软件设置自动
        qDebug() << QString("-A=%1").arg(autoRun);
        XFunc::runAsAdmin(qApp->applicationDirPath() + QString("/%1.exe").arg(qApp->applicationName()), QString("-A=%1").arg(autoRun));
    } else {
        if (toClose()) {
            exitApp(retcode);
        }
    }
}

MainWindow* Session::mainWid()
{
    static QMutex main_mutex;
    QMutexLocker lock(&main_mutex);
    if(NULL == m_mainWid) {
        m_mainWid = new MainWindow();
    }

    return m_mainWid;
}

void Session::releaseMainWid()
{
    if (m_mainWid) {
        m_mainWid->deleteLater();
    }
}

Login* Session::LoginWid()
{
    static QMutex login_mutex;
    QMutexLocker lock(&login_mutex);
    if (NULL == m_loginWid)
        m_loginWid = new Login();

    return m_loginWid;
}

QWidget* Session::CurWid()
{
    if (m_mainWid)
        return m_mainWid;

    if (m_loginWid)
        return m_loginWid;

    return NULL;
}

bool Session::toClose()
{
    bool toclose = true;
    return toclose;
}

void Session::exitApp(int code)
{
    qDebug()<< __FUNCTION__ << code;
    if (SingleApp *sa = qobject_cast<SingleApp *>(qApp)) {
    }

    //可以加快阴影窗口的消失
    foreach (QWindow *wid, qApp->allWindows()) {
       wid->hide();
    }

    if (m_mainWid) {
        m_mainWid->deleteLater();
    }

    qApp->exit(code);
    QTimer::singleShot(3000, this, [=]{
        qDebug()<< "force exit";
        exit(code);
    });
}

void Session::setMode(QString mode)
{
    m_mode = mode;
}

bool Session::isDeadline()
{
    return m_mode == "deadline";
}

QStringList Session::fillArgs(QStringList args)
{
    if (isDeadline()) {
        args << "--mode=deadline";
    }
    return args;
}
