#ifndef UPHANDLER_H
#define UPHANDLER_H

#include <QObject>
#include <QThread>
#include "transfer/transhandler.h"
#include "upwork.h"
#include "tool/xfunc.h"

class UpHandler : public TransHandler
{
    Q_OBJECT
public:
    explicit UpHandler(TransHandler *parent = 0);
    ~UpHandler();
    Q_INVOKABLE void startScan();

    Q_INVOKABLE void readFile();
    Q_INVOKABLE void readFileExceptError();
    Q_INVOKABLE void readFileError();
    Q_INVOKABLE void delErrorFile();
    Q_INVOKABLE void delUp(QString order);
private:
    QString workRule(db_upfile& file);
    void setProgress(QList<db_upfile> fileL);

private slots:
    void scan();
    void workFinished(db_upfile file, int isDel);
    void upSpeed(db_upfile file, qint64 speed);
    void updateFile(int type, db_upfile file);
private:
    QMap<QString, UpWork*> m_work;
    QMap<QThread*, UpWork*> m_pool;

    QMap<QString, int> m_missionState;
    //统计任务速度等信息
    QMap<QString, QJsonObject> m_upDetail;
    int m_detailC;
    int m_fileId;
};

#endif // UPHANDLER_H
