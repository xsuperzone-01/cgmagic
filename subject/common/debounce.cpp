#include "debounce.h"

Debounce::Debounce(QObject *parent) :
    QObject(parent)
{
    m_debounce = m_throttle = NULL;
    m_debTime = 0;
}

void Debounce::setDebTime(int msec)
{
    m_debTime = msec;
}

void Debounce::startDeb(int msec)
{
    m_debounce = createTimer(m_debounce, 0);
    m_debounce->start(msec);
}

void Debounce::startThr(int time)
{
    m_throttle = createTimer(m_throttle, 1);
    if (isThrActive())
        return;
    m_throttle->start(time);
}

bool Debounce::isThrActive()
{
    return m_throttle && m_throttle->isActive();
}

void Debounce::startDeb()
{
    startDeb(m_debTime);
}

QTimer *Debounce::createTimer(QTimer *timer, int type)
{
    if (!timer) {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        if (0 == type)
            connect(timer, SIGNAL(timeout()), this, SIGNAL(debout()));
        if (1 == type)
            connect(timer, SIGNAL(timeout()), this, SIGNAL(throut()));
    }
    return timer;
}
