#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <QObject>
#include <QTimer>

class Debounce : public QObject
{
    Q_OBJECT
public:
    explicit Debounce(QObject *parent = 0);

    void setDebTime(int msec);
    void startDeb(int msec);
    void startThr(int time);
    bool isThrActive();
signals:
    void debout();
    void throut();

public slots:
    void startDeb();

private:
    QTimer* createTimer(QTimer* timer, int type);
private:
    QTimer* m_debounce;
    int m_debTime;
    QTimer* m_throttle;
};

#endif // DEBOUNCE_H
