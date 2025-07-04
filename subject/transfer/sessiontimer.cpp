#include "sessiontimer.h"

TransScan* SessionTimer::m_upScan = NULL;
TransScan* SessionTimer::m_downScan = NULL;

bool SessionTimer::m_isScan = true;

SessionTimer::SessionTimer(QObject *parent) :
    QObject(parent)
{
}

void SessionTimer::startTimer()
{
    m_isScan = true;
    startUpTimer();
    startDownTimer();
}

void SessionTimer::stopTimer()
{
    m_isScan = false;
    stopUpTimer();
    stopDownTimer();
}

void SessionTimer::startUpTimer()
{
    if (!m_upScan){
        m_upScan = new TransScan(0);
        m_upScan->start();
    }
}

void SessionTimer::startDownTimer()
{
    if (!m_downScan){
        m_downScan = new TransScan(1);
        m_downScan->start();
    }
}

void SessionTimer::stopUpTimer()
{
    if (m_upScan) {
        m_upScan->deleteLater();
        m_upScan = NULL;
    }
}

void SessionTimer::stopDownTimer()
{
    if (m_downScan) {
        m_downScan->deleteLater();
        m_downScan = NULL;
    }
}
