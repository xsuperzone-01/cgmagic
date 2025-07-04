#ifndef PULLMSG_H
#define PULLMSG_H

#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QTimer>

class PullMsg : public QObject
{
    Q_OBJECT
public:
    explicit PullMsg(QObject *parent = 0);
    ~PullMsg();

    void pullTimer();

signals:
    void changeBalance(const QJsonObject obj);

private slots:
    void pullOut();

private:
    void balanceHandle();

    void userAuth();

    void messageHandle();

private:
    QPointer<QTimer> m_timer;
    int m_count;
    int m_maxId;
};

#endif // PULLMSG_H
