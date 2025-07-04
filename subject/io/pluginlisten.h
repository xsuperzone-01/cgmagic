#ifndef PLUGINLISTEN_H
#define PLUGINLISTEN_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include "db/userconfig.h"

class PluginListen : public QObject
{
    Q_OBJECT
public:
    explicit PluginListen(QObject *parent = 0);
    ~PluginListen();

    void initPluginListen();
    int initLoginListen();

    static void savePlugins();
    static void saveCGs();
    void msgOk(QString tip);
    static QStringList unityVersion();
    static QJsonObject getPlugins();
    static void setMaxPlugins();
    static QMap<QString, int> vrverMap;
signals:
    void refreshOrder();
    void vrscene(QJsonObject obj);
    void qqlogin(QJsonObject obj);
public slots:
    void commit(int type, QJsonObject obj);
private slots:
    void newConnect();
    void newData();
    void unityData();
    void sketchData();
    void vrsceneData();
private:
    void commitOrder(QJsonObject& obj);
    void commitUnity(QJsonObject& obj);
    void modifyPlugin(QJsonObject& obj);
    QJsonArray modifySp(db_sp sp, QJsonArray arr);
private:
    QTcpServer* m_server;

    QMap<QTcpSocket*, int> m_stMap;
    QMap<QTcpSocket*, QByteArray> m_stbMap;
    static QJsonObject m_plugin;
    static QJsonObject m_Max_plugins;

    QTcpServer* m_unityServer;
    QByteArray m_unityByte;
    static QStringList m_unityVersion;

    QTcpServer* m_sketchServer;
    QByteArray m_sketchByte;
    static QJsonObject m_sketchVersion;

    QTcpServer* m_vrscene;
    QByteArray m_vrsceneByte;
};

#endif // PLUGINLISTEN_H
