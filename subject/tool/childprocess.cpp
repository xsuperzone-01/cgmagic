#include "childprocess.h"

#include <QDebug>
#define qD qDebug()<<
#include <QFile>
#include <QEventLoop>

ChildProcess::ChildProcess(QObject *parent) :
    QObject(parent),
    m_selfRun(false),
    m_checkTimer(NULL),
    m_socketRecord(false),
    m_socketPort(0)
{

}

ChildProcess::~ChildProcess()
{
    disconnect(&m_pro, 0, 0, 0);
    if (m_checkTimer) {
        m_checkTimer->stop(); m_checkTimer->deleteLater(); m_checkTimer = NULL;
    }
}

int ChildProcess::existProcess(QString proName)
{
    proName = exeName(proName);

    QProcess process;
#ifdef Q_OS_WIN
    process.start("tasklist.exe", QStringList() << "-fi" << QString("imagename eq %1").arg(proName));
#else
    process.start("/bin/bash", QStringList()<< "-c" << QString("ps -A|grep %1|grep -v grep").arg(proName));
#endif
    if (loopProcess(&process)) {
        QByteArray byte = process.readAll();
#ifdef Q_OS_WIN
        qD __FUNCTION__ << QString::fromLocal8Bit(byte);
#else
        qD __FUNCTION__ << QString::fromUtf8(byte);
#endif
        return byte.count(proName.toUtf8());
    }
    return 0;
}

bool ChildProcess::loopProcess(QProcess *pro, int msecs)
{
    if (!pro)
        return false;

    bool ok = true;

    QTimer out;
    QEventLoop loop;
    connect(pro, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), &loop, [=, &loop, &ok](int exitCode, QProcess::ExitStatus exitStatus){
        loop.quit();
        if (exitCode != 0) {
            ok = false;
        }
    });
    connect(&out, &QTimer::timeout, &loop, [=, &loop, &ok]{
        loop.quit();
        ok = false;
    });
    out.start(msecs);
    loop.exec();
    out.stop();

    return ok;
}

QString ChildProcess::exeName(QString name)
{
#ifdef Q_OS_WIN
    if (!name.endsWith(".exe"))
        name.append(".exe");
#else
    if (name.endsWith(".exe"))
        name.remove(".exe");
#endif
    return name;
}
