#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <functional>
#include <QMap>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QPointer>
#include <QMutex>

#include <QDebug>
#define qD qDebug()<<
#define NET NetWork::inst()

#include <QNetworkProxyFactory>
class ProxyFactory : public QNetworkProxyFactory
{
public:
    class ProxyAddr {
    public:
        ProxyAddr(QString addr) {
            QString domain = QString(addr).remove(QRegularExpression("http://|https://"));
            protocol = addr.split("//").first() + "//";
            host = domain.split(":").first();
            port = domain.split(":").last();
        }
        QString proxyAddr(int proxyPort) {
            pf->m_hostMap.insert(host, port.toInt());
            return QString("%1%2:%3").arg(protocol).arg(pf->m_remoteHost).arg(proxyPort);
        }

        QString protocol;
        QString host;
        QString port;
        ProxyFactory* pf;
    };

    ProxyFactory() {

    }

    QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query = QNetworkProxyQuery());

    static bool openProxy(QStringList proxyL, bool bindDel = false);
public:
    QMap<QString, int> m_hostMap;
    QString m_remoteHost;
    static QStringList m_proxyHostL;
    static QStringList m_whiteL;
    static QMap<QString, QJsonArray> m_proxyMap;
};

#define LOCALHTTP "http://127.0.0.1:46160"

class SkyDriveResponse {
public:
    int result;
    int code;
    QString msg;
    QJsonObject obj;
    QJsonArray arr;
    SkyDriveResponse(){}
    SkyDriveResponse(QJsonObject obj) {
        result = obj["result"].toInt();
        code = obj["code"].toInt();
        msg = obj["msg"].toString();

        if (obj["data"].isObject())
            this->obj = obj["data"].toObject();
        if (obj["data"].isArray())
            arr = obj["data"].toArray();
    }
};

class FuncBody{
public:
    FuncBody() {
        succ = 0;
        timeout = false;
        netCode = QNetworkReply::NoError;
    }

    int succ;
    int httpCode;
    int netCode;
    QString errMsg;
    QByteArray b;
    QJsonObject j;
    QJsonArray arr;
    bool timeout;

    QString datetime;
};

typedef std::function<void (FuncBody)> FuncBack;

class Request {
public:
    Request() {}
    Request(QObject* object) {
        this->object = object;
        timeout = 0;
        sn = -1;
        thread = 0;
    }
    Request(QString url, QByteArray byte, QObject* object, int timeout = 0) {
        this->url = url;
        this->byte = byte;
        this->object = object;
        this->timeout = timeout;
        sn = -1;
        thread = 0;
    }

    QPointer<QObject> object;
    QString url;
    QNetworkRequest netreq;
    QByteArray byte;
    int timeout;

    QPointer<QThread> thread;
    FuncBack func;
    int sn;


};

class BackFuncTool : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE void beginBack(Request req, FuncBody rep);
};

#define GET "GET"
#define POST "POST"
#define PUT "PUT"

class NetWork : public QObject
{
    Q_OBJECT
public:
    static NetWork* inst();
    ~NetWork();
    void release();

    void get(const QString& url, FuncBack func, QObject *qo);
    void post(const QString& url, const QByteArray &byte, FuncBack func, QObject *qo, QNetworkRequest request = QNetworkRequest());
    QNetworkReply* postR(const QString& url, const QByteArray &byte, FuncBack func, QObject *qo);
    void post(Request req, FuncBack func);
    void del(const QString &url, FuncBack func, QObject *qo);
    void put(const QString &url, const QByteArray &byte, FuncBack func, QObject *qo);

    void getThird(QNetworkRequest request, FuncBack func, QObject *qo);

    FuncBody sync(QString method, QString url, QByteArray data = "", int timeout = 0);
    FuncBody sync(QString method, QString url, QJsonObject data, int timeout = 0);

    QString bs();

    static void setRawHeader(QNetworkRequest &request);
    static void setSsl(QNetworkRequest &request);
    void setSession(QNetworkRequest &request);

    void xrget(const QString& url, FuncBack func, QObject* qo);
    void xrpost(const QString& url, const QByteArray &byte, FuncBack func, QObject *qo);
    void xrput(const QString &url, const QByteArray &byte, FuncBack func, QObject *qo);

private:
    explicit NetWork(QObject *parent = 0);
    Q_INVOKABLE void nget(const QString& url, FuncBack func, QThread *backTh, QObject *qo);
    Q_INVOKABLE QNetworkReply* npost(Request req, FuncBack func, QThread *backTh);
    Q_INVOKABLE void ndel(const QString &url, FuncBack func, QThread *backTh, QObject *qo);
    Q_INVOKABLE void nput(const QString &url, const QByteArray &byte, FuncBack func, QThread *backTh, QObject *qo);
    void setMap(QNetworkReply *reply, FuncBack func, QThread *backTh, Request req);

signals:
private slots:
    void replyFinished(QNetworkReply* reply);
    FuncBody handReply(QNetworkReply* reply, int timeout);
private:
    static NetWork* m_d;
    QNetworkAccessManager *m_netMan;
    int m_sn;
    QMutex m_replyMu;
    QMap<QNetworkReply*, Request> m_replyM;
public:
    ProxyFactory* m_proxyFactory;
};

#endif // NETWORK_H
