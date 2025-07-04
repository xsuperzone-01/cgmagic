#ifndef DOWNHANDLER_H
#define DOWNHANDLER_H

#include <QObject>
#include "transfer/down/downwork.h"
#include <QThread>
#include "tool/xfunc.h"
#include "transfer/transhandler.h"

class DownHandler : public TransHandler
{
    Q_OBJECT
public:
    explicit DownHandler(TransHandler *parent = 0);
    ~DownHandler();
    Q_INVOKABLE void startScan();

    Q_INVOKABLE void readFile();
    Q_INVOKABLE void updateFileState(QList<db_downfile> fileL, int toState);
    Q_INVOKABLE void delErrorFile();

    Q_INVOKABLE void missionResult(int mid, bool autoRes, bool isBatch = false);
    Q_INVOKABLE void coverResult(int mid, int cover);

    void setMissionCover(QString order, int cover);
    int missionCover(QString order);

private:
    QString workRule(db_downfile& file);
signals:
    void downWorkOver();
private slots:
    void scan();
    void workFinished(db_downfile &file, int isDel);
    void workCanceled(int id);

    void delayDown();

    void removeTempFile(db_downfile file);

private:
    QMap<QString, DownWork*> m_work;
    QMap<QThread*, DownWork*> m_pool;

    QMap<int, QJsonObject> m_cover;
    QList<db_downfile> m_downList;
    QTimer m_delayTimer;

    QMutex m_mcMu;
    QMap<QString, int> m_missionCover;
};

#endif // DOWNHANDLER_H
