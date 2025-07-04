#ifndef SESSIONTIMER_H
#define SESSIONTIMER_H

#include <QObject>
#include "transscan.h"

class SessionTimer : public QObject
{
    Q_OBJECT
public:
    explicit SessionTimer(QObject *parent = 0);

    static void startTimer();
    static void stopTimer();
private:
    static void startUpTimer();
    static void startDownTimer();
    static void stopUpTimer();
    static void stopDownTimer();
private:
    static TransScan* m_upScan;
    static TransScan* m_downScan;
public:
    static bool m_isScan;
};

#endif // SESSIONTIMER_H
