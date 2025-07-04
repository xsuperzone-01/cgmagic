#ifndef TRANSHANDLER_H
#define TRANSHANDLER_H

#include <QObject>

#include "transfer/transset.h"

class TransHandler : public QObject
{
    Q_OBJECT
public:
    explicit TransHandler(QObject *parent = 0);

    Q_INVOKABLE virtual void startScan() = 0;

    DownloadUrl downloadLink(int farmId, int fileId);
    Farm farmInfo(int farmId, int fileId = 0);
    int missionUpState(QString num);
    bool reTrans(QString key);
    void removeFs(int farmId);
private:
    void GetFsIp(const QString url, Farm &farm, const int fileId);
    QMap<QString, int> m_reTrans;
    QMap<int, QByteArray> m_FsIps;
};

#endif // TRANSHANDLER_H
