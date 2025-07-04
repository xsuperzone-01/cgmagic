#ifndef CRASH_H
#define CRASH_H

#include <QObject>
#ifdef Q_OS_WIN
#include <Windows.h>
#include <dbghelp.h>
#endif

class Crash : public QObject
{
    Q_OBJECT
public:
    explicit Crash(QObject *parent = nullptr);
#ifdef Q_OS_WIN
    static LONG  applicationCrashHandler(EXCEPTION_POINTERS *pException);
#endif

    void initMsgBox();

    static QString dmpPath;
    static QString version;

protected:
    bool eventFilter(QObject *watched, QEvent *event);

signals:
    void startException();

};

#endif // CRASH_H
