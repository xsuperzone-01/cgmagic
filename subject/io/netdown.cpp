#include "netdown.h"

#include <QFile>
#include "common/trayicon.h"
//#include "common/xfunc.h"

NetDown::NetDown(QObject *parent) :
    QObject(parent),
    m_netMan(NULL),
    m_reply(NULL)
{
    m_netMan = new QNetworkAccessManager;
}

NetDown::~NetDown()
{
    clearData();
    if (m_netMan) {
        m_netMan->deleteLater(); m_netMan = NULL;
    }
}

QNetworkReply* NetDown::get(QString url, QString save)
{
    //快速切换下载时清理资源
    if (m_reply) {
        clearData();
    }

    m_savePath = save;

    m_reply = m_netMan->get(QNetworkRequest(QUrl(url)));

    connect(m_reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(m_reply, SIGNAL(finished()), this, SLOT(replyFinish()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));

    return m_reply;
}

void NetDown::clearData()
{
    if (m_file.isOpen())
        m_file.close();
    if (m_reply) {
        disconnect(m_reply, 0, 0, 0);
        m_reply->abort();
        m_reply->deleteLater();
    }
}

QString NetDown::savePath()
{
    return m_savePath;
}

void NetDown::readyRead()
{
    if (!m_file.isOpen()) {
        m_file.setFileName(m_savePath);
        if(m_file.exists())
            m_file.remove();
        if (!m_file.open(QIODevice::ReadWrite)) {
            this->deleteLater();
            return;
        }
    }

    m_file.write(m_reply->readAll());
}

void NetDown::replyFinish()
{
    m_file.close();
    emit netDownFinish();
    this->deleteLater();
}

void NetDown::error(QNetworkReply::NetworkError error)
{
    qDebug() << __FUNCTION__ << error;
    this->deleteLater();
}
