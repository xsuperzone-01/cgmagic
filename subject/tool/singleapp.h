#ifndef SINGLEAPP_H
#define SINGLEAPP_H

#include <QApplication>
#include <QPointer>
#include <QLocalServer>
#include <QLocalSocket>
#include "common/protocol.h"

class SingleApp : public QApplication
{
    Q_OBJECT
public:
    explicit SingleApp(int &argc, char *argv[]);
    ~SingleApp();

    bool Single(bool force);

    QLocalServer *server();
    QByteArray response();

    void exitOther();

signals:
    void showUp();

public slots:
    bool exitSingle();

private:
    bool createServer(QString name);

private:
    QPointer<QLocalServer> m_TCP_server;
    QPointer<QLocalSocket> m_TCP_socket;

    QByteArray m_response;
};

#endif // SINGLEAPP_H
