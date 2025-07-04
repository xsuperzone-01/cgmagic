#ifndef DOWNWORK_H
#define DOWNWORK_H

#include <QObject>
#include <QTcpSocket>
#include <QNetworkReply>
#include "tool/xfunc.h"
#include "db/downloaddao.h"

#define DTEMP ".xtemp"
#define DTEMPXR ".xr"

class DownWork : public QObject
{
    Q_OBJECT
public:
    explicit DownWork(QObject *parent = 0);
    ~DownWork();
    Q_INVOKABLE void downReq(db_downfile file);
    void setIPPort(QString ip, int port, int farmErr);
    void setLink(QString link, int error);
signals:
    void DownWorkFinished(db_downfile& file, int isDel);
private slots:
    void readData();
    void writeFile();
    void errorRecord(QAbstractSocket::SocketError error);
    void replyError(QNetworkReply::NetworkError error);
    void replyFinished();

    void connected();
    void timeOut();
private:
    void finishDown(int isDel);
    void dError(int err);
    bool fileHash();
    QString validPath(QString path);

private:
    QTcpSocket* m_socket;
    QPointer<QNetworkReply> m_reply;
    QString m_ip;
    int m_port;
    int m_error;
    QString m_link;

    QFile* m_file;
    QFile* m_cfgFile;
    qint64 m_startPoint;
    qint64 m_newSize;
    qint64 m_oldSize;

    QTimer* m_timer;
    QTimer* m_speedTimer;
    QList<qint64> m_speedList;
    int m_speedCount;

    bool m_isFirst;
public:
    db_downfile m_dbFile;
    db_downfile m_bakFile;
    bool m_isQuit;
};

#endif // DOWNWORK_H
