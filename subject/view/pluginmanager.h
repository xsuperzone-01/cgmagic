#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#define MS_NAME "xcgmagicstartup.ms"
#define MS_NAME_OLD "cgmagicstartup.ms"

#include <QObject>

struct MaxPluginCopy {
    QString src;
    QString dest;
    QString srcHash;
};

class PluginManager : public QObject
{
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);

    QStringList allMaxPath();

    QString maxRegPath(int type, QString name);
    static QString readReg(QString path, QString keyName);

    bool repairPlugin(bool admin = false);

    void removePlugins();

    bool comparePlugins();


signals:

private:
    QStringList m_allMax;
    QString m_ms;
    QString m_msHash;
};

#endif // PLUGINMANAGER_H
