#include "singleapp.h"
#include <QDebug>

#define SERVER_NAME "xcgmagic.exe"

SingleApp::SingleApp(int &argc, char *argv[]) :
    QApplication(argc, argv),
    m_TCP_server(NULL),
    m_TCP_socket(NULL)
{
}

SingleApp::~SingleApp()
{
    if (m_TCP_server) {
        m_TCP_server->deleteLater();
        m_TCP_server = NULL;
    }
    if (m_TCP_socket) {
        m_TCP_socket->deleteLater();
        m_TCP_socket = NULL;
    }
}

bool SingleApp::Single(bool force)
{
    qDebug()<< __FUNCTION__ << QDir::home();
    m_TCP_socket = new QLocalSocket(this);
    m_TCP_socket->connectToServer(SERVER_NAME);

    if (!force && m_TCP_socket->waitForConnected(500)) {
        m_TCP_socket->write("hello");
        m_TCP_socket->flush();
        m_TCP_socket->waitForReadyRead(1000);
        m_response = m_TCP_socket->readAll();
        qDebug()<< "local server resp:" << m_response;
        return false;
    } else {
        return createServer(SERVER_NAME);
    }
}

bool SingleApp::exitSingle()
{
    if (m_TCP_server) {
        bool ok = m_TCP_server->removeServer(SERVER_NAME);
        qDebug()<< __FUNCTION__ << ok << m_TCP_server->errorString();
        m_TCP_server->deleteLater();
    }
    return true;
}

QLocalServer *SingleApp::server()
{
    return m_TCP_server;
}

QByteArray SingleApp::response()
{
    return m_response;
}

void SingleApp::exitOther()
{
    qDebug()<< __FUNCTION__;

    if (m_TCP_socket)
        m_TCP_socket->deleteLater();

    m_TCP_socket = new QLocalSocket(this);
    m_TCP_socket->connectToServer(SERVER_NAME);
    m_TCP_socket->waitForConnected(500);
    m_TCP_socket->write("exit");
    m_TCP_socket->close();

    createServer(SERVER_NAME);
}

bool SingleApp::createServer(QString name)
{
    if (m_TCP_server)
        m_TCP_server->deleteLater();

    m_TCP_server = new QLocalServer(this);
    m_TCP_server->removeServer(name);
    bool ok = m_TCP_server->listen(name);
    QObject::connect(m_TCP_server, SIGNAL(newConnection()), this, SIGNAL(showUp()));
    if (!ok) {
        qDebug()<< m_TCP_server->errorString();
    }
    return ok;
}
