#ifndef CHILDPROCESS_H
#define CHILDPROCESS_H

#include <QObject>
#include <QProcess>
#include <QTimer>

class ChildProcess : public QObject
{
    Q_OBJECT
public:
    explicit ChildProcess(QObject *parent = 0);
    ~ChildProcess();

    static int existProcess(QString proName);

    static bool loopProcess(QProcess *pro, int msecs = 30000);

signals:

private:
    static QString exeName(QString name);

private:
    QProcess m_pro;
    QTimer* m_checkTimer;
    bool m_selfRun;
    QString m_exe;

    bool m_socketRecord;
    int m_socketPort;
};

#endif // CHILDPROCESS_H
