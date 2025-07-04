#include "transscan.h"

#include "common/session.h"


TransScan::TransScan(int transType, QObject *parent) :
    QThread(parent),
    m_handler(NULL),
    m_plgunLis(NULL)
{
    m_transType = transType;
}

TransScan::~TransScan()
{
    if (m_handler) {
        m_handler->deleteLater();
        m_handler = NULL;
    }
    if (m_plgunLis) {
        m_plgunLis->deleteLater();
        m_plgunLis = NULL;
    }
    exit();
    wait();
}

void TransScan::run()
{
    if (m_transType == 0) {
        m_handler = new UpHandler;
        Session::instance()->m_upHand = (UpHandler*)m_handler;

        //插件监听
        m_plgunLis = new PluginListen;
        m_plgunLis->initPluginListen();
        connect(m_plgunLis, SIGNAL(refreshOrder()), Session::instance()->mainWid(), SLOT(refreshOrder()));
    } else {
        m_handler = new DownHandler;
        Session::instance()->m_downHand = (DownHandler*)m_handler;
    }

    QINVOKE(m_handler, "startScan");
    exec();
}
