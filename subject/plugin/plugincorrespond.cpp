#include "plugincorrespond.h"
#include <QMutex>
#include <QMutexLocker>
#include <QStringList>
#include <QProcess>
#include "tool/childprocess.h"
#include <QApplication>
#include <QFileInfo>
#include <QDebug>
#include "common/session.h"

PluginCorrespond* PluginCorrespond::pluginCorrespond = nullptr;

PluginCorrespond::PluginCorrespond(QObject *parent) : QObject(parent)
{

}

//单例模式
PluginCorrespond* PluginCorrespond::getInstance(){
    if(!pluginCorrespond){
        static QMutex mutex;

        QMutexLocker locker(&mutex);

        if(!pluginCorrespond){
            pluginCorrespond = new PluginCorrespond;
        }
    }

    return pluginCorrespond;
}

//更新vip用户的插件状态
void PluginCorrespond::updateViplPlugins(QStringList vipModelList){
    int length = vipModelList.size();
    QStringList vipUsedModelsList;
    QString hideModels;
    for(int i = 0; i < length; i++){
        if(i == length -1){
            hideModels = vipModelList.at(i);
            break;
        }
        vipUsedModelsList.append(vipModelList.at(i));
    }
    QString vipUsedModels = vipUsedModelsList.join(",");
    mbUtil("MbUtil_UpdateWhiteListAuthorizedApps", QStringList()<<QString("-v=%1").arg(vipUsedModels));  //授权模块
    mbUtil("MbUtil_UpdateWhiteListHiddenApps", QStringList()<<QString("-v=%1").arg(hideModels));
}

//更新nonVip用户的插件状态
void PluginCorrespond::updateNonViplPlugins(QString freeTime, QStringList nonVipModelList){
    int length = nonVipModelList.size();
    QString empowerModels;
    QString freeModels;
    QString hideModels;
    for(int i = 0; i < length; i++){
        if(i== 0){
            empowerModels = nonVipModelList.at(i);
        }else if(i == length -1){
            hideModels = nonVipModelList.at(i);
        }else{
            freeModels = nonVipModelList.at(i);
        }
    }

    mbUtil("MbUtil_UpdateTrialTime", QStringList()<<QString("-v=%1").arg(freeTime));                //授权模块免费试用时间
    mbUtil("MbUtil_UpdateWhiteListTrialApps", QStringList()<<QString("-v=%1").arg(empowerModels));   //授权模块试用
    mbUtil("MbUtil_UpdateWhiteListAuthorizedApps", QStringList()<<QString("-v=%1").arg(freeModels)); //免费模块试用
    mbUtil("MbUtil_UpdateWhiteListHiddenApps", QStringList()<<QString("-v=%1").arg(hideModels));     //隐藏模块禁用

}

//点击Max插件板块，插件向客户端发起请求
void PluginCorrespond::makeRequestForMaxToClient(){
    int userStatus = Session::instance()->mainWid()->isUserVip;
    QString freeTime = Session::instance()->mainWid()->freeTime;
    QStringList vipPluginList = Session::instance()->mainWid()->vipModelList;
    QStringList nonVipPluginList = Session::instance()->mainWid()->nonVipModelList;

    if(userStatus == 1 && !vipPluginList.isEmpty()){
        qDebug()<<"Vip user modelList is:"<<vipPluginList;
        updateViplPlugins(vipPluginList);
    }else if(userStatus == 2 && !nonVipPluginList.isEmpty()){
        qDebug()<<"nonVip user freeTime and modelList is:"<<freeTime<<nonVipPluginList;
        updateNonViplPlugins(freeTime, nonVipPluginList);
    }else{
        qDebug()<<"Extra situations";
        Session::instance()->mainWid()->checkVip();
    }
}

//插件进程执行
QString PluginCorrespond::mbUtil(QString cmd, QStringList args){
    QString exe = qApp->applicationDirPath() + "/mobao/mbUtil.exe";
    qDebug()<<"mbUtil.exe path is:"<<exe;
    QProcess pro;
    pro.setWorkingDirectory(QFileInfo(exe).absolutePath());
    args.prepend(QString("-c=%1").arg(cmd));
    args.prepend(QString("-d=%1").arg("MbExt.dll"));
    pro.start(exe, QStringList()<< args);
    ChildProcess::loopProcess(&pro);

    QByteArray b = pro.readAllStandardOutput();

    QStringList bL = QString(b).split("\n");
    QString tag = "mbUtilReturn";
    foreach (QString s, bL) {
        if (s.contains(tag)) {
            QString m = s.mid(s.indexOf(tag) + tag.length() + 1);
            return m;
        }
    }
    return "";
}
