#ifndef NETDOWN_H
#define NETDOWN_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>

class NetDown : public QObject
{
    Q_OBJECT
public:
    explicit NetDown(QObject *parent = 0);
    ~NetDown();

    QNetworkReply *get(QString url, QString save);
    void clearData();
    QString savePath();
signals:
    void netDownFinish();
private slots:
    void readyRead();
    void replyFinish();
    void error(QNetworkReply::NetworkError error);
private:
    QNetworkAccessManager *m_netMan;
    QNetworkReply* m_reply;
    QString m_savePath;
    QFile m_file;
};

#endif // NETDOWN_H
