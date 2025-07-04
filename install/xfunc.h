#pragma execution_character_set("utf-8")
#ifndef XFUNC_H
#define XFUNC_H

#include <QObject>
#define NOMINMAX
#include <Windows.h>
#include <aclapi.h>
#include <dbghelp.h>
#include <wbemidl.h>
#include <comdef.h>
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

#define qD qDebug()<<"qD"<<
#define QINVOKE QMetaObject::invokeMethod
#define UPDAO UploadDAO::inst()
#define DDAO DownloadDAO::inst()
#define NET NetWork::inst()

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

    static QVariant getHKCU(QString path, QString keyName);
    static void setHKCU(QString path, QString keyName, QVariant value);
    static void clearLogFiles();
    static void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static int existProcess(QString proName);
    static void veryDel(const QString& dir);
    static QString ipByDomain(const QString& domain);
    static QByteArray intToByte(int i);
    static QByteArray qint64ToByte(qint64 i64);
    static int intLowToHigh(int i);
    static QByteArray jsonObjToByte(const QJsonObject& obj);
    static QString jsonObjToStr(const QJsonObject& obj);
    static QJsonObject jsonStrToObj(const QString& str);
    static QJsonArray jsonStrToArr(const QString& str);
    static QString getSizeString(qint64 i64SizeOfBit);
    static QString getSpeedString(qint64 i64SizeOfBit);
    static int createFixedSizeFile(QString fname, qint64 fsize);
    static void openDir(const QString& path);
    static QString VOL();
    static QString MAC();
    static QString ScreenWidth();
    static QString ScreenHeight();
    static bool SYS(MachineInfo& info);
    static QStringList getDistNames();

    static QList<DWORD> getProcessidFromName(QString name);

    static QString encodeURI(QString src);
};

#endif // XFUNC_H
