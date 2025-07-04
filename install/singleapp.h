#ifndef SINGLEAPP_H
#define SINGLEAPP_H

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>

class SingleApp : public QApplication
{
    Q_OBJECT
public:
    explicit SingleApp(int &argc, char *argv[]);

    bool exist();

    //由于install和uninstall共用SingleApp，在启服务时会共用ServerName，导致覆盖安装有冲突，因此进行修改服务名称
    void setServerName(bool status);

signals:
    void showUp();

private:
    QLocalSocket* m_socket;
    QLocalServer* m_server;

    QString serN;
};

#endif // SINGLEAPP_H
