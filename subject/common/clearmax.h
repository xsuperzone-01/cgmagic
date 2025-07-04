#ifndef CLEARMAX_H
#define CLEARMAX_H

#include <QThread>

class ClearMax : public QThread
{
    Q_OBJECT
public:
    explicit ClearMax(QObject *parent = 0);

protected:
    void run();
signals:

public slots:

};

#endif // CLEARMAX_H
