#ifndef UPWORK_H
#define UPWORK_H

#include <QObject>
#include <QTcpSocket>
#include "tool/xfunc.h"
#include "db/downloaddao.h"

class UpWork : public QObject
{
    Q_OBJECT
public:
    explicit UpWork(QObject *parent = 0);
    ~UpWork();
    Q_INVOKABLE void upReq(const db_upfile& file);
    void setIPPort(QString ip, int port, int farmErr);
    void setSignMd5(QString rule, QString md5);
    QString zipPath();
signals:
    void UpWorkFinished(db_upfile& file, int isDel);
    void upSpeed(db_upfile file, qint64 speed);
    void updateFile(int type, db_upfile file);
private slots:
    void readData();
    void errorRecord(QAbstractSocket::SocketError error);
    void bytesWritten(qint64 len);
    void connected();
    void timeOut();

    void sendData();
private:
    void startZip();
    void startUp();
    void finishUp(int isDel);
    void uError(int err);
private:
    QTcpSocket* m_socket;
    QString m_ip;
    int m_port;

    qint64 m_startPoint;
    qint64 m_uppedSize;
    qint64 m_oldSize;
    qint64 m_srcSize;
    double m_zipRatio;
    QFile* m_file;

    int m_error;

    QTimer* m_timer;
    QTimer* m_speedTimer;
    QList<qint64> m_speedList;

    QString m_sign;

    bool m_send;
    int m_dataSize;

    qint64 m_curTime;
public:
    db_upfile m_dbFile;
    bool m_isQuit;
};

#endif // UPWORK_H
