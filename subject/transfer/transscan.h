#ifndef TRANSSCAN_H
#define TRANSSCAN_H

#include <QThread>

#include "down/downhandler.h"
#include "up/uphandler.h"
#include "tool/xfunc.h"
#include "io/pluginlisten.h"

class TransScan : public QThread
{
    Q_OBJECT
public:
    explicit TransScan(int transType, QObject *parent = 0);
    ~TransScan();
protected:
    void run();
private:
    int m_transType;//0 上传 1 下载
    TransHandler* m_handler;
    PluginListen* m_plgunLis;
};

#endif // TRANSSCAN_H
