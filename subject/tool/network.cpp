#include "msgtool.h"
#include "network.h"
#include "jsonutil.h"
#include "common/session.h"
#include "config/userinfo.h"
#include "xfunc.h"
#include "config/userinfo.h"
#include "common/protocol.h"
#include <QMetaEnum>
#include "common/eventfilter.h"
#include "common/trayicon.h"

QMetaEnum networkErrorEnum;

NetWork* NetWork::m_d = NULL;
NetWork::NetWork(QObject *parent) :
    QObject(parent),
    m_netMan(NULL),
    m_sn(0),
    m_proxyFactory(NULL)
{
    m_netMan = new QNetworkAccessManager(this);
    connect(m_netMan, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    qRegisterMetaType< Request >("Request");
    qRegisterMetaType< FuncBody >("FuncBody");

    networkErrorEnum = QMetaEnum::fromType<QNetworkReply::NetworkError>();
}

NetWork *NetWork::inst()
{
    if (m_d == NULL) {
        m_d = new NetWork;
    }
    return m_d;
}

NetWork::~NetWork()
{
    disconnect(m_netMan,0,0,0);
}

void NetWork::release()
{
    m_replyM.clear();
}

void NetWork::del(const QString &url, FuncBack func, QObject* qo)
{
    QThread* th = QThread::currentThread();
    QINVOKE(this, "ndel", Qt::QueuedConnection,
            Q_ARG(QString, url),
            Q_ARG(FuncBack, func),
            Q_ARG(QThread*, th),
            Q_ARG(QObject*, qo));
}

void NetWork::ndel(const QString &url, FuncBack func, QThread* backTh, QObject* qo)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    setRawHeader(request);

    setSession(request);

    QNetworkReply* reply = m_netMan->deleteResource(request);
    setMap(reply, func, backTh, Request(qo));
}

void NetWork::put(const QString &url, const QByteArray& byte, FuncBack func, QObject* qo)
{
    QThread* th = QThread::currentThread();
    QINVOKE(this, "nput", Qt::QueuedConnection,
            Q_ARG(QString, url),
            Q_ARG(QByteArray, byte),
            Q_ARG(FuncBack, func),
            Q_ARG(QThread*, th),
            Q_ARG(QObject*, qo));
}

void NetWork::getThird(QNetworkRequest request, FuncBack func, QObject* qo)
{
    setSession(request);
    QNetworkReply* reply = m_netMan->get(request);
    QThread* backTh = QThread::currentThread();
    setMap(reply, func, backTh, Request(qo));
}

/*
    timeout 单位s
*/
FuncBody NetWork::sync(QString method, QString url, QByteArray data, int timeout)
{
    FuncBody resp;
    EventLoop loop(0);
    QTimer timer;
    timer.setSingleShot(true);
    timeout = timeout > 0 ? timeout : 30;

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(QUrl(url));
    setRawHeader(request);
    setSession(request);

    timer.start(timeout*1000);

    //同一线程中创建QNetworkAccessManager和QNetworkReply
    QNetworkAccessManager netMan;

    QNetworkReply* reply = NULL;
    if (method == GET)
        reply = netMan.get(request);
    else if (method == POST)
        reply = netMan.post(request, data);
    else if (method == PUT)
        reply = netMan.put(request, data);
    if (!reply)
        return resp;

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    reply->setProperty("sync", "SYNC ");
    setMap(reply, NULL, NULL, Request("", data, NULL));
    if (loop.execUntilAppQuit()) {
        reply->deleteLater();
        return resp;
    }
    resp = handReply(reply, 0);
    if ((resp.httpCode >= 200 && resp.httpCode < 300) && QNetworkReply::NoError == resp.netCode)
        resp.succ = 1;
    if (resp.b.startsWith("{")) {
        resp.j = JsonUtil::jsonStrToObj(resp.b);
    } else if (resp.b.startsWith("["))
        resp.arr = JsonUtil::jsonStrToArr(resp.b);

    timer.stop();
    return resp;
}

FuncBody NetWork::sync(QString method, QString url, QJsonObject data, int timeout)
{
    return sync(method, url, JsonUtil::jsonObjToByte(data), timeout);
}

QString NetWork::bs()
{
    QString domain;

    return domain;
}

void NetWork::nput(const QString &url, const QByteArray &byte, FuncBack func, QThread *backTh, QObject* qo)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(url);
    setRawHeader(request);

    setSession(request);

    QNetworkReply* reply = m_netMan->put(request, byte);
    Request req(qo);
    req.byte = byte;
    setMap(reply, func, backTh, req);
}

void NetWork::get(const QString &url, FuncBack func, QObject* qo)
{
    QThread* th = QThread::currentThread();
    QINVOKE(this, "nget", Qt::QueuedConnection,
            Q_ARG(QString, url),
            Q_ARG(FuncBack, func),
            Q_ARG(QThread*, th),
            Q_ARG(QObject*, qo));
}

void NetWork::nget(const QString &url, FuncBack func, QThread* backTh, QObject* qo)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    setRawHeader(request);

    setSession(request);

    QNetworkReply* reply = m_netMan->get(request);
    setMap(reply, func, backTh, Request(qo));
}

void NetWork::post(const QString &url, const QByteArray& byte, FuncBack func, QObject* qo, QNetworkRequest request)
{
    Request req(url, byte, qo);
    req.netreq = request;
    post(req, func);
}

QNetworkReply *NetWork::postR(const QString &url, const QByteArray &byte, FuncBack func, QObject *qo)
{
    Request req(url, byte, qo);
    return npost(req, func, QThread::currentThread());
}

void NetWork::post(Request req, FuncBack func)
{
    QThread* th = QThread::currentThread();

    QINVOKE(this, "npost", Qt::QueuedConnection,
            Q_ARG(Request, req),
            Q_ARG(FuncBack, func),
            Q_ARG(QThread*, th));
}

QNetworkReply* NetWork::npost(Request req, FuncBack func, QThread *backTh)
{
    req.netreq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.netreq.setUrl(req.url);
    setRawHeader(req.netreq);

    setSession(req.netreq);

    QNetworkReply* reply = m_netMan->post(req.netreq, req.byte);
    setMap(reply, func, backTh, req);
    return reply;
}

QString operationText(int operation) {
    switch (operation) {
    case 2:
        return "GET";
    case 3:
        return "PUT";
    case 4:
        return "POST";
    case 5:
        return "DELETE";
    default:
        break;
    }
    return "";
}

void NetWork::setMap(QNetworkReply* reply, FuncBack func, QThread* backTh, Request req)
{
    req.sn = m_sn;
    req.func = func;
    req.thread = backTh;
    m_replyMu.lock();
    m_replyM.insert(reply, req);
    m_replyMu.unlock();

    QString url = reply->url().toString();
    if (!url.contains("/speedtest/upload") && !url.contains("/speedtest/download")) {
        qDebug().noquote()<< QString("--%1-").arg(m_sn++) + " " + reply->property("sync").toString() + operationText(reply->operation()) + " " + url + " " + req.byte;
    }

    if (req.timeout) {
        QTimer* out = new QTimer(this);
        out->setSingleShot(true);
        out->start(req.timeout * 1000);
        connect(out, &QTimer::timeout, [=]{
            out->deleteLater();
            handReply(reply, 1);
        });
    }
}

void NetWork::replyFinished(QNetworkReply *reply)
{
    handReply(reply, 0);
}

FuncBody NetWork::handReply(QNetworkReply *reply, int timeout)
{
    FuncBody rep;
    m_replyMu.lock();
    if (!m_replyM.contains(reply)) {
        m_replyMu.unlock();
        return rep;
    }

    QString debug;
    Request req = m_replyM.take(reply);
    m_replyMu.unlock();

    rep.datetime = reply->rawHeader("date");
    qDebug()<< __func__<< rep.datetime;

    if (timeout) {
        rep.timeout = true;
        debug = QString("time out:%1").arg(req.timeout);
    } else {
        // 缓存中的session为null
        if (UserInfo::instance()->getSession().isEmpty()){
            if (reply->hasRawHeader("set-cookie")) {
                QString cookie(reply->rawHeader("set-cookie"));
                QStringList cookies = cookie.split(";");
                for (int i = 0; i < cookies.size(); i++) {
                    if (cookies.at(i).indexOf("SESSION=") == 0) {
                        QString session(cookies.at(i));
                        session.remove("SESSION=");
                        qDebug() << "拿到session:" << session;
                        UserInfo::instance()->setSession(session);
                    }
                }
            }
            if (UserInfo::instance()->getSession().isEmpty()) {
                QString session = reply->rawHeader("x-auth-token");
                if (!session.isEmpty()) {
                    qDebug() << "从x-auth-token拿到session:" << session;
                    UserInfo::instance()->setSession(session);
                }
            }
        }

        if (reply->hasRawHeader("userid")){
            qDebug() << "拿到userid:" << reply->rawHeader("userid");
            UserInfo::instance()->setUserId(reply->rawHeader("userid"));
        }
        int httpCode = 0;
        int netCode = reply->error();
        QString errMsg = reply->errorString();

        if (reply->error() == QNetworkReply::NoError) {
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            httpCode = status;
            if(status >= 200 && status < 300) {

            } else {
                QString httpS = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
                errMsg = httpS;
                QString err = QString("%1_%2_%3").arg(reply->url().toString()).arg(status).arg(httpS);
                debug = QString("HttpError:%1").arg(err);
            }
        } else {
            QString url = reply->url().toString();
            QString err = QString("%1_%2").arg(networkErrorEnum.valueToKey(reply->error())).arg(reply->errorString().remove(url));
            debug = QString("NetworkError:%1").arg(err);

            if (!url.contains("127.0.0.1") && !url.contains("/bs/user/login")) {
                // Plugin类是放在多线程执行的，同时QT是不允许子线程操作ui，防止子线程操作ui影响主线程操作ui，甚至导致程序崩溃
                QMetaObject::invokeMethod(TrayIcon::instance()->traySet(), "showNetworkErrorTip", Qt::AutoConnection, Q_ARG(QString, tr("网络异常，请稍后再试！")));
            }

            if (reply->error() == QNetworkReply::AuthenticationRequiredError) {

                int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                qDebug()<< "HttpStatusCodeAttribute" << status;
                if (401 == status && !USERINFO->userId().isEmpty()) {
                    status = reply->rawHeader("status").toInt();
                    qDebug()<<"即将重登 bs"<< status;
                    qputenv("date", rep.datetime.toUtf8());

                    // 切换host时会出现，其他异地登录等靠gw.xrender.com返回
                    int gwCode = 0;
                    switch (status) {
                    case 11990000:
                        gwCode = 10403;
                        break;
                    default:
                        break;
                    }
                    if (gwCode > 0)
                        Session::instance()->reLogin(gwCode);
                }
            }
        }

        QByteArray data;
        if (reply->isOpen())
            data = reply->readAll();
//        if (debug.isEmpty())
            debug = data + " " + debug;

        rep.httpCode = httpCode;
        rep.netCode = netCode;
        rep.errMsg = errMsg;
        rep.b = data;
    }

    if (req.thread) {
        BackFuncTool* tool = new BackFuncTool;
        tool->moveToThread(req.thread);
        QINVOKE(tool, "beginBack", Qt::QueuedConnection,
                Q_ARG(Request, req),
                Q_ARG(FuncBody, rep));
    }

    QString url = reply->url().toString();
    if (!url.contains("/speedtest/upload") && !url.contains("/speedtest/download")) {
        qDebug().noquote()<< QString("--%1- %2").arg(req.sn).arg(debug);
    }

    reply->deleteLater();
    return rep;
}

void NetWork::setRawHeader(QNetworkRequest& request)
{
    request.setRawHeader("sign", sign);
    request.setRawHeader("appname", appname);
    request.setRawHeader("productType", productType);
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("mac", UserInfo::instance()->mac().toUtf8());
    if (USERINFO->isOS())
        request.setRawHeader("x-site", "1");

    if (!UserInfo::instance()->userId().isEmpty())
        request.setRawHeader("userId", UserInfo::instance()->userId().toUtf8());
    //子账号登录需要，和xneo方式不一致
    request.setRawHeader("x-auth-token", UserInfo::instance()->getSession().toUtf8());

    QString url = request.url().toString();
    if (url.contains("/uc/validation/xneofss")) {
        request.setRawHeader("sign", "c6bbb372b64b28ba280ab6bbbeca9519");
        request.setRawHeader("appname", "netdisk");
    }

    //bs
    request.setRawHeader("token", USERINFO->accessToken());
    request.setRawHeader("lang", QString::number(USERINFO->appLangNum()).toUtf8());

    setSsl(request);
}

void NetWork::setSsl(QNetworkRequest &request)
{
    //ssl
    QSslConfiguration ssl;
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    ssl.setProtocol(QSsl::AnyProtocol);
    request.setSslConfiguration(ssl);
}

void NetWork::setSession(QNetworkRequest &request)
{
    QString session = QString("SESSION=%1").arg(UserInfo::instance()->getSession());
    request.setRawHeader("Cookie", session.toUtf8());
    m_netMan->setCookieJar(NULL);
}

void NetWork::xrget(const QString &url, FuncBack func, QObject *qo)
{
    QString xrUrl = USERINFO->bs() + url;
    get(xrUrl, func, qo);
}

void NetWork::xrpost(const QString &url, const QByteArray &byte, FuncBack func, QObject *qo)
{
    QString xrUrl = USERINFO->bs() + url;
    post(xrUrl, byte, func, qo, QNetworkRequest());
}

void NetWork::xrput(const QString &url, const QByteArray &byte, FuncBack func, QObject *qo)
{
    QString xrUrl = USERINFO->bs() + url;
    put(xrUrl, byte, func, qo);
}

void BackFuncTool::beginBack(Request req, FuncBody rep)
{
    if (req.object) {
        if (!rep.timeout) {
            if ((rep.httpCode >= 200 && rep.httpCode < 300) && QNetworkReply::NoError == rep.netCode)
                rep.succ = 1;
            if (rep.b.startsWith("{")) {
                rep.j = JsonUtil::jsonStrToObj(rep.b);
            } else if (rep.b.startsWith("["))
                rep.arr = JsonUtil::jsonStrToArr(rep.b);

            int code = rep.j.value("code").toString().toInt();
            if (code == 10403 || code == 12403 || code == 13403 || code == 14403 || code == 15403) {
                qDebug()<<"即将重登"<<rep.j;
                qputenv("date", rep.datetime.toUtf8());
                Session::instance()->reLogin(code);
            }
        }
        req.func(rep);
    }
    deleteLater();
}

#include <QTcpServer>

#include <view/msgbox.h>

QStringList ProxyFactory::m_proxyHostL =
        QStringList() << "bsm1.xrender.com:29909" << "www.xrender.com:80" << "movie.xrender.com:80";
QStringList ProxyFactory::m_whiteL =
        QStringList() << "bsm1.xrender.com";
QMap<QString, QJsonArray> ProxyFactory::m_proxyMap;

QList<QNetworkProxy> ProxyFactory::queryProxy(const QNetworkProxyQuery &query)
{
    QNetworkProxy proxy(QNetworkProxy::NoProxy);
    QString srchost = query.peerHostName();
    if (m_whiteL.contains(srchost))
        return QList<QNetworkProxy>() << proxy;

    proxy.setType(QNetworkProxy::HttpProxy);

    if (int port = m_hostMap.value(srchost)) {
        proxy.setHostName(m_remoteHost);
        proxy.setPort(port);
    }

    return QList<QNetworkProxy>() << proxy;
}

bool ProxyFactory::openProxy(QStringList proxyL, bool bindDel)
{
    QString appDir = qApp->applicationDirPath();
    QString appName = qApp->applicationName();
    QString exe = QString("%1/%2.exe").arg(appDir).arg(appName);
#ifdef OS_LINUX
    exe = QString("%1/%2.sh").arg(appDir).arg(appName);
#endif

    QMap<QString, QJsonArray> proxyMap;
    QJsonArray adminArr;
    QJsonArray subarr;
    QStringList checkPortL;
    //若存在直接读取
    {
        QFile net(UserInfo::instance()->allUserPath() + "/netshadd");
        if (net.exists()) {
            net.open(QFile::ReadOnly);
            QJsonArray arr = JsonUtil::jsonStrToArr(net.readAll());
            if (!arr.isEmpty()) {
                foreach (QJsonValue val, arr) {
                    QJsonArray subarr = val.toArray();
                    for (int i = 0; i < proxyL.length(); ++i) {
                        for (int j = 0; j < subarr.size(); ++j) {
                            QJsonArray arr = subarr.at(j).toArray();
                            if (JsonUtil::jsonArrToStr(arr).contains(proxyL.at(i).split(":").first())) {
                                proxyMap.insert(proxyL.at(i), arr);
                                checkPortL << QString(":%1").arg(arr.first().toString());
                            }
                        }
                    }
                }
            }
            if (!checkPortL.isEmpty()) {
                QProcess pro;
                pro.start("netstat -an");
                pro.waitForFinished();
                QString b = pro.readAll();
                if (!b.contains(checkPortL.first())) {
                    qD "存在netshadd记录" << JsonUtil::jsonArrToStr(arr);
                    qD "netstat -an,检测不存在端口" << checkPortL.first();
                    proxyMap.clear();
                    checkPortL.clear();
                    net.close();
                    net.remove();
                }
            }
        }
    }
    if (proxyMap.isEmpty()) {
        //需要在登录后 根据账号的 角色 开启以下端口
        for (int i = 0; i < proxyL.length(); ++i) {
            QTcpServer ts;
            if (ts.listen(QHostAddress::Any)) {
                QJsonArray sub;
                sub.append(QString::number(ts.serverPort()));sub.append(proxyL.at(i).split(":").first());sub.append(proxyL.at(i).split(":").last());
                subarr.append(sub);
                checkPortL << QString(":%1").arg(ts.serverPort());
                ts.close();
                proxyMap.insert(proxyL.at(i), sub);
            } else {
                qD "master open port fail for" << proxyL.at(i);
                MsgTool::msgOkLoop(QObject::tr("端口监听失败！%1").arg(proxyL.at(i)));
                return false;
            }
        }
        adminArr.append(subarr);

        QString ads = JsonUtil::jsonArrToStr(adminArr);
        ads.replace("\"", "\\\"");
    #ifdef OS_WIN
            if (-1 == XFunc::runAsAdmin(exe, QString("netshadd \"%1\"").arg(ads))) {
                return false;
            }

            //检测是否开启成功
            {
                int i = 0;
                for (i; i < 5; ++i) {
                    QProcess pro;
                    pro.start("netstat -an");
                    pro.waitForFinished();
                    QString b = pro.readAll();
                    if (!checkPortL.isEmpty() && !b.contains(checkPortL.first())) {
                        qD checkPortL.first() << i + 1 << b;
                        QThread::msleep(2000);
                    } else
                        break;
                }
                if (5 == i) {
                    MsgTool::msgOkLoop(QObject::tr("端口转发失败！请检查系统相关服务是否正常运行。"));
                    return false;
                }
            }
    #elif OS_LINUX
            QString httpPort = QString("firewall-cmd --query-port=%1/tcp").arg(UserInfo::clientServerPort());
            QString IP = "firewall-cmd --query-masquerade";
            QString permanent = "--permanent。firewall-cmd --reload";
            QProcess pro;
            pro.start(httpPort);
            pro.waitForFinished();
            if (pro.readAllStandardError().contains("not running")) {
                MsgTool::msgOkLoop(QObject::tr("请开启防火墙。systemctl start firewalld"));
                return false;
            }
            if (pro.readAll().contains("no")) {
                MsgTool::msgOkLoop(QObject::tr("请开启客户端http服务端口。%1 %2").arg(httpPort.replace("query", "add")).arg(permanent));
                return false;
            }
            pro.start(IP);
            pro.waitForFinished();
            if (pro.readAll().contains("no")) {
                MsgTool::msgOkLoop(QObject::tr("请开启防火墙IP伪装。%1 %2").arg(IP.replace("query", "add")).arg(permanent));
                return false;
            }
            pro.start(exe, QStringList()<<"netshadd" << ads);
            pro.waitForFinished();
    #endif

            //持久化 为了netdelete
            {
                QFile net(UserInfo::instance()->allUserPath() + "/netshadd");
                net.open(QFile::ReadOnly);
                QJsonArray arr = JsonUtil::jsonStrToArr(net.readAll());
                arr.prepend(subarr);
                net.close();
                net.open(QFile::WriteOnly);
                net.write(JsonUtil::jsonArrToByte(arr));
                net.close();
            }
    }

        {
            QList<QString> psL = proxyMap.keys();
            foreach (QString ps, psL) {
                //跨线程
                m_proxyMap.insert(ps, proxyMap.value(ps));
            }
        }

        if (bindDel) {
            QObject::connect(qApp, &QCoreApplication::aboutToQuit, [=]{
    #ifdef OS_WIN
    #elif OS_LINUX
                QProcess pro;
                pro.start(exe, QStringList()<<"netshdelete");
                pro.waitForFinished();
    #endif
            });
        }

    return true;
}
