#include "pullmsg.h"

#include "common/protocol.h"
#include "tool/network.h"
#include "common/session.h"
#include "io/pluginlisten.h"

#define PULLSTEP 10000


PullMsg::PullMsg(QObject *parent) :
    QObject(parent),
    m_timer(NULL),
    m_count(0),
    m_maxId(0)
{

}

PullMsg::~PullMsg()
{
    if (m_timer) {
        if (m_timer->isActive()) {
            m_timer->stop();
        }
        m_timer->deleteLater();
    }
}

void PullMsg::pullTimer()
{
    if (!m_timer) {
        m_timer = new QTimer(this);
        if (m_timer) {
            connect(m_timer, SIGNAL(timeout()), this, SLOT(pullOut()));
            m_timer->setInterval(PULLSTEP);
            m_timer->start();
        }
    }

    pullOut();
}

void PullMsg::pullOut()
{
    //余额 30s
    if (m_count % 3 == 0)
        balanceHandle();

    //消息队列 30s
    if (m_count % 3 == 0)
        messageHandle();

    m_count++;
}

void PullMsg::balanceHandle()
{
    NET->get(balanceUrl, [=](FuncBody f){
        QJsonObject resultData = f.j.value("data").toObject().value("data").toObject();
        if (!resultData.isEmpty()) {
            emit changeBalance(resultData);
        } else {
            qDebug() << "查询余额失败!!!" << f.j;
        }
    }, this);
}

void PullMsg::userAuth()
{
    if (USERINFO->instance()->isUserAuth())
        return;

    NET->get(userAuthUrl, [=](FuncBody f) {
        int cert = f.j.value("data").toObject().value("data").toObject().value("certify").toInt();

        if (1 == cert) {
            USERINFO->instance()->setUserAuth(true);
        }
    }, this);
}

void PullMsg::messageHandle()
{
    if (USERINFO->isOS())
        return;

    NET->xrget("/bs/user/messages", [=](FuncBody f){
        QJsonArray arr = f.j["rows"].toArray();
        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject row = arr.at(i).toObject();
            int id = row["id"].toInt();
            if (id >= m_maxId)
                m_maxId = id;

            QJsonObject con = row["content"].toObject();
            int type = con["type"].toInt();
            //结果文件
            if (2 == type) {
                int mid = con["missionId"].toInt();
                QINVOKE(Session::instance()->m_downHand, "missionResult",
                        Q_ARG(int, mid),
                        Q_ARG(bool, true));
            }
            //插件更新
            if (3 == type) {
                PluginListen::savePlugins();
            }
        }

        //已处理消息
        if (m_maxId != 0)
            NET->xrput(QString("/bs/user/%1/messages").arg(m_maxId), "", [=](FuncBody f){

            }, this);
        m_maxId = 0;
    }, this);
}
