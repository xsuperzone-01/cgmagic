#ifndef PLUGINCORRESPOND_H
#define PLUGINCORRESPOND_H

#include <QObject>

class PluginCorrespond : public QObject
{
    Q_OBJECT

private:
    explicit PluginCorrespond(QObject *parent = nullptr);
    static PluginCorrespond* pluginCorrespond;

signals:

public:
    static PluginCorrespond* getInstance();                                     //单例模式
    void updateViplPlugins(QStringList vipModelList);                           //更新Vip用户的插件状态
    void updateNonViplPlugins(QString freeTime, QStringList nonVipModelList);   //更新nonVip用户的插件状态

    QString mbUtil(QString cmd, QStringList args);                              //插件进程执行
    Q_INVOKABLE void makeRequestForMaxToClient();                               //点击Max插件板块，插件向客户端发起请求
};

#endif // PLUGINCORRESPOND_H
