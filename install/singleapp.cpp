#include "singleapp.h"

#include <QDebug>

SingleApp::SingleApp(int &argc, char *argv[]) :
    QApplication(argc, argv),
    m_socket(NULL),
    m_server(NULL)
{
    serN = "cgmagic_install";
}

bool SingleApp::exist()
{
//    QString serN = "cgmagic_install";

    m_socket = new QLocalSocket(this);
    m_socket->connectToServer(serN);
    if (m_socket->waitForConnected(1000)) {
        qDebug()<<"server exist";
        return true;
    } else {
        m_server = new QLocalServer(this);
        m_server->removeServer(serN);
        bool ret = m_server->listen(serN);
        QObject::connect(m_server, SIGNAL(newConnection()), this, SIGNAL(showUp()));
        return false;
    }
}

//由于install和uninstall共用SingleApp，在启服务时会共用ServerName，导致覆盖安装有冲突，因此进行修改服务名称
void SingleApp::setServerName(bool status){
    if(status){
        serN = "cgmagic_uninstall";
    }else{
        serN = "cgmagic_install";
    }
}
