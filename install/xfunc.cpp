#include "xfunc.h"

#include <QDateTime>
#include <QDir>
#include <QSettings>
#include <QProcess>
#include <QDirIterator>
#include <QJsonArray>
#include <tlhelp32.h>
#include <tchar.h>
#include <QRegularExpression>
#include <QScreen>
#include <QGuiApplication>

XFunc::XFunc(QObject *parent) :
    QObject(parent)
{
}

QVariant XFunc::getHKCU(QString path, QString keyName)
{
    QSettings regSet("HKEY_CURRENT_USER\\" + path, QSettings::NativeFormat);
    return regSet.value(keyName);
}


void XFunc::setHKCU(QString path, QString keyName, QVariant value)
{
    QSettings regSet("HKEY_CURRENT_USER\\" + path, QSettings::NativeFormat);
    regSet.setValue(keyName, value);
}

void XFunc::clearLogFiles()
{

}

void XFunc::msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{

}

int XFunc::existProcess(QString proName)
{
    if (!proName.endsWith(".exe"))
        proName.append(".exe");
    QProcess process;
    process.start("tasklist.exe", QStringList() << "-fi" << QString("imagename eq %1").arg(proName));
    if (process.waitForFinished()) {
        QByteArray byte = process.readAll();
        qD QString::fromLocal8Bit(byte);
        return byte.count(proName.toUtf8());
    }
    return 0;
}

void XFunc::veryDel(const QString &dir)
{
    if (dir.isEmpty())
        return;

    QFileInfo info(dir);
    if (info.isSymLink() || info.isFile()) {
        QFile file(info.absoluteFilePath());
        file.setPermissions(QFile::WriteOwner);
        file.remove();
    }

    if (info.isDir()) {
        QDirIterator d(dir, QDir::Files, QDirIterator::Subdirectories);
        while (d.hasNext()) {
            d.next();
            QFileInfo info = d.fileInfo();
            QFile file(info.absoluteFilePath());
            file.setPermissions(QFile::WriteOwner);
            file.remove();
        }
        QDirIterator d2(dir, QDir::Dirs|QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (d2.hasNext()) {
            d2.next();
            QFileInfo info = d2.fileInfo();
            QDir().rmpath(info.absoluteFilePath());
        }
        QDir().rmpath(dir);
    }
}

QString XFunc::ipByDomain(const QString &domain)
{
    QString ip = domain;
    if (ip.indexOf(QRegularExpression("(\\d{1,3}\\.){3}\\d{1,3}")) == -1) {
        QHostInfo host = QHostInfo::fromName(domain);
        if (host.error() == QHostInfo::NoError) {
            ip = host.addresses().first().toString();
        }
    }
    return ip;
}

QByteArray XFunc::intToByte(int i)
{
    QByteArray byte;
    byte.resize(4);
    byte[0] = (uchar)((0xff000000 & i) >> 24);
    byte[1] = (uchar)((0x00ff0000 & i) >> 16);
    byte[2] = (uchar)((0x0000ff00 & i) >> 8);
    byte[3] = (uchar)(0x000000ff & i);
    return byte;
}

QByteArray XFunc::qint64ToByte(qint64 i64)
{
    QByteArray byte;
    byte.resize(8);
    byte[0] = (uchar)((0xff00000000000000 & i64) >> 56);
    byte[1] = (uchar)((0x00ff000000000000 & i64) >> 48);
    byte[2] = (uchar)((0x0000ff0000000000 & i64) >> 40);
    byte[3] = (uchar)((0x000000ff00000000 & i64) >> 32);
    byte[4] = (uchar)((0x00000000ff000000 & i64) >> 24);
    byte[5] = (uchar)((0x0000000000ff0000 & i64) >> 16);
    byte[6] = (uchar)((0x000000000000ff00 & i64) >> 8);
    byte[7] = (uchar)(0x00000000000000ff & i64);
    return byte;
}

int XFunc::intLowToHigh(int i)
{
    QByteArray byte;
    byte.resize(4);
    byte[0] = (uchar)((0xff000000 & i) >> 24);
    byte[1] = (uchar)((0x00ff0000 & i) >> 16);
    byte[2] = (uchar)((0x0000ff00 & i) >> 8);
    byte[3] = (uchar)(0x000000ff & i);
    int k;
    memcpy(&k, byte, 4);
    return k;
}

QByteArray XFunc::jsonObjToByte(const QJsonObject &obj)
{
    QJsonDocument document;
    document.setObject(obj);
    return document.toJson(QJsonDocument::Compact);
}

QString XFunc::jsonObjToStr(const QJsonObject &obj)
{
    QJsonDocument document;
    document.setObject(obj);
    QByteArray ba = document.toJson(QJsonDocument::Compact);
    return QString::fromUtf8(ba);
}

QJsonObject XFunc::jsonStrToObj(const QString &str)
{
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());
    return doc.object();
}

QJsonArray XFunc::jsonStrToArr(const QString &str)
{
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());
    return doc.array();
}

QString XFunc::getSizeString(qint64 i64SizeOfBit)
{
    QString strSize;
    if ((i64SizeOfBit/8796093022208)>0){
        //Tb级别
        qint64 i64SizeOfGb = i64SizeOfBit/1073741824;
        double dSizeOfGb = (double)i64SizeOfGb;
        double dSizeOfTb = dSizeOfGb/1024;
        QString strTb = QString::number(dSizeOfTb, 'f', 2);
        strTb.replace(".00","");

        strSize.append(strTb);
        strSize.append("TB");
    }
    else if((i64SizeOfBit/1073741824)>0)
    {
        //Gb级别
        qint64 i64SizeOfMb = i64SizeOfBit/1048576;
        double dSizeOfMb = (double)i64SizeOfMb;
        double dSizeOfGb = dSizeOfMb/1024;
        QString strGb = QString::number(dSizeOfGb, 'f', 2);
        strGb.replace(".00","");

        strSize.append(strGb);
        strSize.append("GB");
    }
    else if((i64SizeOfBit/1048576)>0)
    {
        //Mb级别
        qint64 i64SizeOfKb = i64SizeOfBit/1024;
        double dSizeOfKb = (double)i64SizeOfKb;
        double dSizeOfMb = dSizeOfKb/1024;
        QString strMb = QString::number(dSizeOfMb, 'f', 2);
        strMb.replace(".00","");

        strSize.append(strMb);
        strSize.append("MB");
    }
    else if((i64SizeOfBit/1024)>0)
    {
        //Kb级别
        double dSizeOfBit = (double)i64SizeOfBit;
        double dSizeOfKb = dSizeOfBit/1024;
        QString strKb = QString::number(dSizeOfKb, 'f', 2);
        strKb.replace(".00","");

        strSize.append(strKb);
        strSize.append("KB");
    }
    else
    {
        //byte级别
        strSize.append(QString::number(i64SizeOfBit));
        strSize.append("Byte");
    }
    return strSize;
}

QString XFunc::getSpeedString(qint64 i64SizeOfBit)
{
    QString strSize = "";
    if((i64SizeOfBit/8796093022208)>0)
    {
        //Tb级别
        qint64 i64SizeOfGb = i64SizeOfBit/1073741824;
        double dSizeOfGb = (double)i64SizeOfGb;
        double dSizeOfTb = dSizeOfGb/1024;
        QString strTb = QString::number(dSizeOfTb, 'f', 1);

        strSize.append(strTb);
        strSize.append("TB");
    }
    else if((i64SizeOfBit/1073741824)>0)
    {
        //Gb级别
        qint64 i64SizeOfMb = i64SizeOfBit/1048576;
        double dSizeOfMb = (double)i64SizeOfMb;
        double dSizeOfGb = dSizeOfMb/1024;
        QString strGb = QString::number(dSizeOfGb, 'f', 1);

        strSize.append(strGb);
        strSize.append("GB");
    }
    else if((i64SizeOfBit/1048576)>0)
    {
        //Mb级别
        qint64 i64SizeOfKb = i64SizeOfBit/1024;
        double dSizeOfKb = (double)i64SizeOfKb;
        double dSizeOfMb = dSizeOfKb/1024;
        QString strMb = QString::number(dSizeOfMb, 'f', 1);

        strSize.append(strMb);
        strSize.append("MB");
    }
    else if((i64SizeOfBit/1024)>0)
    {
        //Kb级别
        double dSizeOfBit = (double)i64SizeOfBit;
        double dSizeOfKb = dSizeOfBit/1024;
        QString strKb = QString::number(dSizeOfKb, 'f', 1);

        strSize.append(strKb);
        strSize.append("KB");
    }
    else
    {
        //byte级别
        strSize.append(QString::number(i64SizeOfBit));
        strSize.append("B");
    }
    return strSize;
}

int XFunc::createFixedSizeFile(QString fname, qint64 fsize)
{
    int ret = 0;
    HANDLE hFile = CreateFile(fname.toStdWString().c_str(),GENERIC_READ | GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        ret = -1;
    }
    LARGE_INTEGER liDistanceToMove;
    liDistanceToMove.QuadPart = fsize;
    if (!SetFilePointerEx(hFile, liDistanceToMove, NULL, FILE_BEGIN)) {
        ret = -2;
    }
    if (!SetEndOfFile(hFile)) {
        ret = -2;
    }
    CloseHandle(hFile);
    if (ret == -2)
        DeleteFile(fname.toStdWString().c_str());
    if (-1 == ret)
        ret = -2;
    return ret;
}

void XFunc::openDir(const QString &path)
{
    QFileInfo info(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir() ? path : info.absoluteDir().path()));
}

QString XFunc::VOL()
{
    QProcess p(0);
    p.start("cmd");
    p.waitForStarted();
    p.write("vol\n");
    p.closeWriteChannel();
    p.waitForFinished();
    QString strResult = QString::fromLocal8Bit(p.readAllStandardOutput());

    QString strFind = "卷的序列号是";
    int index = strResult.indexOf(strFind);
    strResult = strResult.right(strResult.length()-index-7);
    strResult = strResult.left(strResult.indexOf("\r\n"));

//    qDebug()<<"vol number is "<<strResult;
    return strResult;
}

QString XFunc::MAC()
{
    QString strMacAddress = "";
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    foreach (QNetworkInterface iface, list)
    {
        //保证获取的是本地的IP地址， 不是虚拟机，隧道之类的网络地址
        //以下语句可以优化
        if(!(iface.humanReadableName().contains("VMware", Qt::CaseInsensitive))&&
                !(iface.humanReadableName().contains("Tunnel", Qt::CaseInsensitive))&&
                !(iface.humanReadableName().contains("Tunneling", Qt::CaseInsensitive))&&
                !(iface.humanReadableName().contains("Loopback", Qt::CaseInsensitive))&&
                !(iface.humanReadableName().contains("Pseudo", Qt::CaseInsensitive))&&
                !(iface.humanReadableName().contains("isatap", Qt::CaseInsensitive))&&
                !(iface.humanReadableName().contains("VMware", Qt::CaseInsensitive)))
        {
            QString strTemp = iface.hardwareAddress().toUpper();
            if(strTemp.size() == 17)
            {
                strMacAddress = strTemp;
            }
        }
    }
    return strMacAddress;
}

QString XFunc::ScreenWidth()
{
    QScreen* desktopWidget = QGuiApplication::primaryScreen();
    QRect screenRect = desktopWidget->geometry();
    return QString::number(screenRect.width());
}

QString XFunc::ScreenHeight()
{
    QScreen* desktopWidget = QGuiApplication::primaryScreen();
    QRect screenRect = desktopWidget->geometry();
    return QString::number(screenRect.height());
}

bool XFunc::SYS(MachineInfo &info)
{
    HRESULT hres;

    hres =  CoInitialize( NULL );
    if (FAILED(hres)) {
        return false;
    }

    hres =  CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
        );


    if ((hres != RPC_E_TOO_LATE) && FAILED(hres)) {
        CoUninitialize();
        return false;
    }

    IWbemLocator *pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID *) &pLoc);

    if (FAILED(hres)) {
        CoUninitialize();
        return false;
    }

    IWbemServices *pSvc = NULL;

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
        );

    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
        );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;

    QStringList sL;
    QString preSel = "SELECT * FROM ";
    sL << "Win32_Processor";
    for (int i = 0; i < sL.length(); ++i) {
        hres = pSvc->ExecQuery(
            SysAllocString(L"WQL"),
            SysAllocString((OLECHAR*)(preSel + sL.at(i)).unicode()),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);

        if (FAILED(hres)) {
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return false;
        }

        while (pEnumerator) {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
                &pclsObj, &uReturn);

            if(0 == uReturn) {
                break;
            }

            VARIANT vtProp;
            if (i == 0) {
                hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
                info.CPU = QString::fromWCharArray(vtProp.bstrVal);
                hr = pclsObj->Get(L"NumberOfCores", 0, &vtProp, 0, 0);
                info.CPU.prepend(QString("[%1]").arg(vtProp.uintVal));
            }
            pclsObj->Release();
        }

        pEnumerator->Release();
        pEnumerator = NULL;
    }

    info.DisplayCard.remove(0, 1);
    info.Memory = info.Memory / 1024 / 1024 / 1024;

    pSvc->Release();
    pLoc->Release();

    CoUninitialize();

    return true;
}

//获取本地磁盘名称
QStringList XFunc::getDistNames()
{
    QStringList ret;

    QFileInfoList iL = QDir::drives();
    foreach (QFileInfo i, iL) {
        QString tmp = i.absolutePath();
        tmp.chop(1);
        ret << tmp;
    }
    return ret;
}

QList<DWORD> XFunc::getProcessidFromName(QString name)
{
    PROCESSENTRY32 pe;
    DWORD id=0;
    HANDLE hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    pe.dwSize=sizeof(PROCESSENTRY32);
    QList<DWORD> list;

    if(!Process32First(hSnapshot,&pe))
      return list;
    while(1)
    {
      pe.dwSize=sizeof(PROCESSENTRY32);
      if(Process32Next(hSnapshot,&pe)==FALSE)
        break;
      QString pName = QString::fromWCharArray(pe.szExeFile);

      if(pName == name)
      {
        id=pe.th32ProcessID;
        list.append(id);
      }
    }
    CloseHandle(hSnapshot);
    return list;
}

QString XFunc::encodeURI(QString src)
{
    QStringList cL;
    cL <<":"<<"/"<<"?"<<"&"<<"=";
    for (int i = 0; i < cL.length(); ++i) {
        src.replace(cL.at(i), "%" + cL.at(i).toUtf8().toHex().toUpper());
    }

    return src;
}
