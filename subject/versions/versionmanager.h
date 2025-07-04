#ifndef VERSIONMANAGER_H
#define VERSIONMANAGER_H

#include "common/Singleton.h"
#include <QTimer>
#include "view/softwareupdate.h"
#include <QJsonArray>
#include <QProcess>

class VersionManager : public QObject
{
    Q_OBJECT
    PATTERN_SINGLETON_DECLARE(VersionManager);
public:
    bool initVesions(bool ignoreFirst, QString proxy = "");

    void check();
    QString newVersion();
    QString detail();
    void detail(QJsonObject &obj);
    QString splitDetail(QString detail);
    QJsonObject client();
    QString clientTime(QJsonObject obj);

    QString exe();
    QStringList args();
    void exitProcess();

    QList<QJsonObject> plugins;

public slots:
    void popup();

private slots:
    void timeout();

private:
    void closeTip();

private:
    QTimer m_timer;
    QString m_root;
    QString m_host;
    QString m_branch;
    QString m_profile;
    QString m_proxy;

    QPointer<SoftwareUpdate> m_softUpdateWid;
    QJsonObject m_client;
    QJsonArray m_ignore;
    QJsonObject m_softs;
    QPointer<QProcess> m_pro;
};

#endif // VERSIONMANAGER_H
