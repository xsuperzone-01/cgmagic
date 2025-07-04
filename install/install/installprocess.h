#ifndef INSTALLPROCESS_H
#define INSTALLPROCESS_H

#include <QJsonObject>
#include <QObject>

class InstallProcess : public QObject
{
    Q_OBJECT
public:
    explicit InstallProcess();

    Q_INVOKABLE void copyPublicFile(QString appDir, QString installDir);

    Q_INVOKABLE void clientSet(QJsonObject obj);

signals:
    void copyPublicFileSuccess();

    void installClientSetSuccess();

    void pro();

private:
    QString instPath;
    QString appPath;
    QString exe;
    QString name;
    QString nameEN;
    QString version;
    QString third;
    QString lang;
    QString mark;
    QString namedef;
    bool startMenu;
    bool autoRun;
    bool cn;
};

#endif // INSTALLPROCESS_H
