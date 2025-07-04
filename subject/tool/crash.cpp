#include "crash.h"

#include <QDateTime>
#include <QMessageBox>
#include <QProcess>
#include <QApplication>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QPushButton>
#include <QStyleFactory>
#include <QStyle>

QMessageBox *msgbPtr;//在子线程内创建的messageBox会有问题，所以只能在主线程创建并传递过过来

QString Crash::dmpPath;
QString Crash::version;

Crash::Crash(QObject *parent) : QObject(parent)
{
    initMsgBox();
}

#ifdef Q_OS_WIN
LONG Crash::applicationCrashHandler(EXCEPTION_POINTERS *pException)
{
    qDebug()<<"程序崩溃，正在生成dmp文件"<<"dmp文件地址："<<Crash::dmpPath;
    QString dumpFileName;
    dumpFileName = QDateTime::currentDateTime().toString("yyyy.MM.dd_HH.mm.ss_zzz")+".dmp";
    dumpFileName.insert(dumpFileName.lastIndexOf("."), QString("_%1").arg(Crash::version));
    dumpFileName = Crash::dmpPath + "/" + dumpFileName;
    HANDLE hDumpFile
            = CreateFile(
                (LPCWSTR)(dumpFileName.utf16()),
                GENERIC_WRITE,FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hDumpFile!=INVALID_HANDLE_VALUE){
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = pException;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;

        bool dump = MiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),
                          hDumpFile,MiniDumpNormal,&dumpInfo,NULL,NULL);
    }

    EXCEPTION_RECORD* record = pException->ExceptionRecord;
    QString errCode(QString::number(record->ExceptionCode,16)),errAdr(QString::number((uint)record->ExceptionAddress,16)),errMod;
    qDebug() << "main thread:" << qApp->thread() << QThread::currentThread() << errCode << errAdr;

    if (msgbPtr) {
        int lastTime = 5;
        QTimer *timer = new QTimer();
        timer->setInterval(1000);
        QString text = tr("抱歉，程序遇到了错误，将在%1s后重启。");
        msgbPtr->setText(text.arg(lastTime));

        QObject::connect(timer, &QTimer::timeout, [=, &lastTime](){
            lastTime--;
            if (lastTime < 0) {
                qDebug() << "crash start application:" << QProcess::startDetached(qApp->applicationFilePath(), QStringList());//重启
                msgbPtr->close();
                return;
            }
            msgbPtr->setText(text.arg(lastTime));
        });
        timer->start();
        msgbPtr->exec();
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void Crash::initMsgBox()
{
    msgbPtr = new QMessageBox();
    msgbPtr->setWindowFlag(Qt::WindowStaysOnTopHint);
    msgbPtr->setIcon(QMessageBox::Warning);
    msgbPtr->installEventFilter(this);

    QPushButton *rebootButton = msgbPtr->addButton(tr("重启"), QMessageBox::AcceptRole);
    QPushButton *rejectButton = msgbPtr->addButton(tr("退出"), QMessageBox::RejectRole);

    QStringList styleL = QStyleFactory::keys();
    QStyle *style = QStyleFactory::create("windowsvista");
    if (!style && !styleL.isEmpty())
        style = QStyleFactory::create(styleL.first());
    if (style) {
        rebootButton->setStyle(style);
        rejectButton->setStyle(style);
    }

    QObject::connect(msgbPtr, &QMessageBox::buttonClicked, [=](QAbstractButton *button){
        if (msgbPtr->clickedButton() == (QAbstractButton*)rebootButton) {
            qDebug() << "crash start application:" << QProcess::startDetached(qApp->applicationFilePath(), QStringList());//重启
        }
        msgbPtr->close();
    });
}

bool Crash::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == msgbPtr) {
        if (event->type() == QEvent::Show) {
            emit startException();
        }
    }
    return QObject::eventFilter(watched, event);
}
