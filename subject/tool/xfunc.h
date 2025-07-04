#ifndef XFUNC_H
#define XFUNC_H

#include <QObject>
#define NOMINMAX
#ifdef Q_OS_WIN
#include <Windows.h>
#include <aclapi.h>
#include <dbghelp.h>
#include <wbemidl.h>
#include <comdef.h>
#endif
#include <QDebug>
#include <QHostInfo>
#include <QDateTime>
#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTimer>
#include <QEventLoop>
#include <QApplication>
#include <QNetworkInterface>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUrl>
#include <QDesktopServices>
#include <QAction>
#include <QJsonArray>
#include <QProcess>

#define QINVOKE QMetaObject::invokeMethod

class MachineInfo
{
public:
    QString OS;  //操作系统
    QString Disk; //硬盘型号
    QString DisplayCard; //显卡型号
    QString MotherBoard; //主板型号
    qint64 Memory;//内存
    QString CPU;
    MachineInfo() {
        Memory = 0;
    }
};

class XFunc : public QObject
{
    Q_OBJECT
public:
    explicit XFunc(QObject *parent = 0);

#ifdef Q_OS_WIN
    static LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException);
#endif
    static QVariant getHKCU(QString path, QString keyName);
    static void clearLogFiles();
    static void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static void veryDel(const QString& dir);
    static bool veryCopy(QString src, QString dest, QString srcroot = QString());
    static QString ipByDomain(const QString& domain);
    static QByteArray intToByte(int i);
    static QByteArray qint64ToByte(qint64 i64);
    static int intLowToHigh(int i);
    static int qint64LowToHigh(qint64 i64);
    static QString getSizeString(qint64 bit, int decimal = 2);
    static QString getSpeedString(qint64 i64SizeOfBit);
    static QString sysDir();    // 操作系统盘
    static void openDir(const QString& path);
    static QString VOL();
    static QString MAC(bool join = false);
    static int ScreenWidth();
    static int ScreenHeight();
    static QRect posScreenRect(QPoint pos);
    static QPoint widScrPos(QWidget* wid, QPoint pos = QPoint());
    static bool SYS(MachineInfo& info);
    static QStringList getDistNames();
    static QString encodeURI(QString src);
    static QString Md5(QString str);
    static void ucs2leToUtf8(QString file);
    static QString readHKCU(QString path, QString keyName);
    static int runAsAdmin(QString exe, QString arg);
    static QString fileHash(QString path);
    static qint64 diskFreeSpace(QString driver);
    static QString toQuery(const QJsonObject& obj);
    static QString exe(QString exe);
    static bool appQuit;

public:
    static QMap<QString, QJsonObject> m_projectConfig;
    static QList<QJsonObject> m_projectL;

};

#endif // XFUNC_H
