#include "xfunc.h"

#include <QDateTime>
#include <QDir>
#include <QSettings>
#include <QProcess>
#include <QDirIterator>
#include <QJsonArray>
#include "../version.h"
#ifdef Q_OS_WIN
#include <tlhelp32.h>
#include <tchar.h>
#include <qmutex.h>
#include <QCryptographicHash>
#include "config/userinfo.h"
#include <common/session.h>
#endif
#include "common/protocol.h"
#include "common/session.h"
#include <QCryptographicHash>
#include <QtConcurrent>
#include <QTextCodec>

QMap<QString, QJsonObject> XFunc::m_projectConfig;
QList<QJsonObject> XFunc::m_projectL;
bool XFunc::appQuit = false;

XFunc::XFunc(QObject *parent) :
    QObject(parent)
{
}

QVariant XFunc::getHKCU(QString path, QString keyName)
{
#ifdef Q_OS_UNIX
    QSettings regSet(USERINFO->instance()->appDataPath() + "/HKCU.ini", QSettings::IniFormat);
    return regSet.value(QString("Settings/%1").arg(keyName));
#else
    QSettings regSet("HKEY_CURRENT_USER\\" + path, QSettings::NativeFormat);
    return regSet.value(keyName);
#endif
}

void XFunc::clearLogFiles()
{
    QDir dir(USERINFO->instance()->allUserPath());
    QFileInfoList list = dir.entryInfoList(QStringList()<<"log-*", QDir::Files, QDir::Name);
    while (list.size() > 7) {
        QFile file(list.first().absoluteFilePath());
        file.remove();
        list.removeFirst();
    }

    QtConcurrent::run([=]{
        QString root = USERINFO->instance()->allUserPath();

        //临时对历史日志加密
        QDir dir(root);
        QFileInfoList list = dir.entryInfoList(QStringList()<<"log-*", QDir::Files, QDir::Name);
        while (list.size() > 0) {
            QString src = list.first().absoluteFilePath();
            if (src.endsWith("a.txt")) {
                list.removeFirst();
                continue;
            }
            QFile file(list.first().absoluteFilePath());
            file.remove();
            list.removeFirst();
        }
    });
}

void XFunc::msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex msgHandler_mutex;
    static QFile msgFile;

    QMutexLocker locker(&msgHandler_mutex);

    QByteArray localMsg = msg.toUtf8();

    QString ds = QDateTime::currentDateTime().toString("yyyyMMdd");

    if (!msgFile.isOpen()) {
        msgFile.setFileName(QString("%1/log-%2a.txt").arg(USERINFO->instance()->allUserPath()).arg(ds));
        msgFile.open(QIODevice::WriteOnly | QIODevice::Append);
    } else {
        QString fn = msgFile.fileName();
        if (!fn.contains(ds)) {
            msgFile.close();
            msgFile.setFileName(QString("%1/log-%2a.txt").arg(USERINFO->instance()->allUserPath()).arg(ds));
            msgFile.open(QIODevice::WriteOnly | QIODevice::Append);
        }
    }

    QString strMsg;
    switch(type)
    {
    case QtDebugMsg:
        break;
    case QtWarningMsg:
        strMsg = QString("Warning:");
        break;
    case QtCriticalMsg:
        strMsg = QString("Critical:");
        break;
    case QtFatalMsg:
        strMsg = QString("Fatal:");
        break;
    }
    QString strDateTime = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString strMessage = strMsg + localMsg + QString("[%1]").arg(strDateTime);

    msgFile.write(strMessage.toUtf8() + "\r\n");
    msgFile.flush();

    fprintf(stdout, "%s\n", strMessage.toLocal8Bit().constData());
    fflush(stdout);

    if (Session::instance()->m_debugText)
        Session::instance()->m_debugText->append(strMessage.toUtf8() + "\r\n");
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

bool XFunc::veryCopy(QString src, QString dest, QString srcroot)
{
    if (dest.isEmpty() || src.isEmpty())
        return false;

    src = QDir::fromNativeSeparators(src);
    dest = QDir::fromNativeSeparators(dest);

    QFileInfo srcf(src);
    QFileInfo destf(dest);
    if (srcf.isFile() && destf.isDir()) {
        QFile::copy(src, dest + "/" + srcf.fileName());
    }

    if (srcf.isDir() && destf.isDir()) {
        QDirIterator d(src, QDir::Files, QDirIterator::Subdirectories);
        while (d.hasNext()) {
            d.next();
            QFileInfo fi = d.fileInfo();
            QString srcPath = fi.absoluteFilePath();

            QString destDir = dest + fi.absolutePath().remove(src);
            QDir().mkpath(destDir);

            QString destPath = QDir(destDir).filePath(fi.fileName());
            QFile::remove(destPath);

            QFile srcFile(srcPath);
            bool ok = srcFile.copy(srcPath, destPath);
            qDebug()<<"将"<<srcPath<<"复制到"<<destPath;
            if (!ok) {
                //传递一个复制失败信号
                qDebug()<< srcFile.errorString()<<"copy error";
                return false;
            }
        }
        return true;
    }
    return false;
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

int XFunc::qint64LowToHigh(qint64 i64)
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
    qint64 k;
    memcpy(&k, byte, 8);
    return k;
}

QString XFunc::getSizeString(qint64 bit, int decimal)
{
    QString strSize;
    if (bit >= 1073741824) {
        strSize = QString::number((double)bit / 1073741824, 'f', decimal) + " GB";
    } else if (bit >= 1048576) {
        strSize = QString::number((double)bit / 1048576, 'f', decimal) + " MB";
    } else if (bit >= 1024) {
        strSize = QString::number((double)bit / 1024, 'f', decimal) + " KB";
    } else {
        strSize = QString::number(bit) + " B";
    }
    strSize.replace(decimal == 2 ? ".00" : ".0", "");
    return strSize;
}

QString XFunc::getSpeedString(qint64 i64SizeOfBit)
{
    QString strSize = getSizeString(i64SizeOfBit, 1);
    strSize.append("/s");
    return strSize;
}

QString XFunc::sysDir()
{
#ifdef Q_OS_WIN
    TCHAR sysDir[128];
    GetSystemDirectory(sysDir, 128 * sizeof(TCHAR));
    QString sysDisk = QString(sysDir[0]);
    return sysDisk;
#else
   return "";
#endif
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

    return strResult;
}

QString XFunc::MAC(bool join)
{
    QStringList strMacAddress;
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
            qDebug()<<"hdmac is src"<<strTemp << iface.flags() << "join:" << join;
            if (join) {
                if (iface.flags().operator &(0x2) == 0x2) {
                    strMacAddress.append(strTemp);
                }
            } else {
                if(strTemp.size() == 17)
                {
                    strMacAddress = QStringList(strTemp);
                }
            }
        }
    }
    return strMacAddress.join(",");
}

int XFunc::ScreenWidth()
{
    QScreen* desktopWidget = QGuiApplication::primaryScreen();
    QRect screenRect = desktopWidget->geometry();
    return screenRect.width();
}

int XFunc::ScreenHeight()
{
    QScreen* desktopWidget = QGuiApplication::primaryScreen();
    QRect screenRect = desktopWidget->geometry();
    return screenRect.height();
}

QRect XFunc::posScreenRect(QPoint pos)
{
    QScreen* desk = QGuiApplication::primaryScreen();
    return desk->geometry();
}

QPoint XFunc::widScrPos(QWidget *wid, QPoint pos)
{
    if (pos.isNull())
        pos = QCursor::pos();
    QRect rect = XFunc::posScreenRect(pos);
    if (pos.y() + wid->height() > rect.height())
        pos = QPoint(pos.x(), pos.y() - wid->height());
    if (pos.x() + wid->width() > rect.width())
        pos = QPoint(pos.x() - wid->width(), pos.y());
    return pos;
}

bool XFunc::SYS(MachineInfo &info)
{
#ifdef Q_OS_WIN
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
    sL << "Win32_OperatingSystem"
       << "Win32_VideoController"
       << "Win32_BaseBoard"
       << "Win32_PhysicalMemory"
       << "Win32_Processor";
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
                hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);  //os
                info.OS = QString::fromWCharArray(vtProp.bstrVal);
            }
            if (i == 1) {
                hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);
                info.DisplayCard = info.DisplayCard.append("," + QString::fromWCharArray(vtProp.bstrVal));
            }
            if (i == 2) {
                hr = pclsObj->Get(L"Product", 0, &vtProp, 0, 0);
                info.MotherBoard = QString::fromWCharArray(vtProp.bstrVal);
            }
            if (i == 3) {
                hr = pclsObj->Get(L"Capacity", 0, &vtProp, 0, 0);
                QString tm = QString::fromWCharArray(vtProp.bstrVal);
                info.Memory += tm.toLongLong();
            }
            if (i == 4) {
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
#endif
    return true;
}

#ifdef Q_OS_WIN
LONG XFunc::ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
{
    QString dumpFileName;
    dumpFileName = QDateTime::currentDateTime().toString("yyyy.MM.dd_HH.mm.ss_zzz")+".dmp";
    dumpFileName.insert(dumpFileName.lastIndexOf("."), QString("_%1").arg(CLIENT_VERSION));
    dumpFileName = USERINFO->instance()->dmpPath() + "/" + dumpFileName;
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
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
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

QString XFunc::encodeURI(QString src)
{
    QStringList cL;
    cL <<":"<<"/"<<"?"<<"&"<<"=";
    for (int i = 0; i < cL.length(); ++i) {
        src.replace(cL.at(i), "%" + cL.at(i).toUtf8().toHex().toUpper());
    }

    return src;
}

QString XFunc::Md5(QString str)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(str.toUtf8());
    return hash.result().toHex();
}

void XFunc::ucs2leToUtf8(QString file)
{
    QFile f(file);
    if (!f.exists())
        return;
    if (f.size() > 2*1024*1024) {
        f.remove();
        return;
    }
    if (!f.open(QFile::ReadOnly)) {
        qDebug()<< f.errorString();
        return;
    }

    QByteArray ba = f.readLine();

    QTextCodec::ConverterState state;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    codec->toUnicode(ba.constData(), ba.size(), &state);

    int ic = state.invalidChars;
    qDebug()<< "invalidChars:" << ic;

    if (2 == ic) {
        f.seek(0);
        ba = f.readAll();
        f.close();
        f.open(QFile::WriteOnly|QFile::Truncate);

        ba = QTextCodec::codecForName("UTF-16LE")->toUnicode(ba).toUtf8();
        f.write(ba);
    }

    f.close();
}

QString XFunc::readHKCU(QString path, QString keyName)
{
    QString value;
#ifdef Q_OS_WIN
    HKEY key;
    std::wstring pathWstr = path.toStdWString();
    LPCWSTR subkey = pathWstr.data();

    if(RegOpenKeyEx(HKEY_CURRENT_USER,subkey,0,KEY_READ|KEY_WOW64_64KEY,&key)==0){
        BYTE *buff = new BYTE[1024];
        std::wstring keyNameWstr = keyName.toStdWString();
        LPCWSTR subkey = keyNameWstr.data();
        ULONG cbSize = 1024;
        if (RegQueryValueEx(key,subkey,NULL,NULL,buff,&cbSize) == 0)
            value = QString::fromUtf16((ushort*)buff);

        RegCloseKey(key);
        delete [] buff;
    }
#endif
    return value;
}

// return -1 cancel
int XFunc::runAsAdmin(QString exe, QString arg)
{
#ifdef Q_OS_WIN
    SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
    sei.lpVerb = TEXT("runas");

    const QChar* strexe = exe.unicode();
    sei.lpFile = LPCWSTR(strexe);

    const QChar* strargs = arg.unicode();
    sei.lpParameters = LPCWSTR(strargs);

    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (!ShellExecuteEx(&sei)) {
        DWORD dwStatus = GetLastError();

        if (dwStatus == ERROR_CANCELLED) {
            return -1;
        }
        else
            if (dwStatus == ERROR_FILE_NOT_FOUND) {
            }
    }
    DWORD dw =  WaitForSingleObject(sei.hProcess,5000);
    switch (dw)
    {
    case WAIT_OBJECT_0:
    // hProcess所代表的进程在5秒内结束
    break;

    case WAIT_TIMEOUT:
    // 等待时间超过5秒
    break;

    case WAIT_FAILED:
    // 函数调用失败，比如传递了一个无效的句柄
    break;
    }
#endif
    return 1;
}

QString XFunc::fileHash(QString path)
{
    QString hashCode;

    QCryptographicHash hash(QCryptographicHash::Md5);
    QFile file(path);
    if (!file.exists())
        return hashCode;

    if(!file.open(QIODevice::ReadOnly))
        return hashCode;

    while ((file.pos() != file.size())){
        QByteArray data;
        data.resize(1024*1024);
        qint64 fr = file.read(data.data(), data.length());
        if (fr == -1) {
            return hashCode;
        }
        data.resize(fr);
        hash.addData(data);
    }
    file.close();

    hashCode = hash.result().toHex();
    return hashCode;
}

qint64 XFunc::diskFreeSpace(QString driver)
{
    qint64 space = 0;
#ifdef Q_OS_WIN
    LPCWSTR lpcwstrDriver = (LPCWSTR)driver.utf16();
    ULARGE_INTEGER liFreeBytesAvailable,liTotalBytes,liTotalFreeBytes;
    if (!GetDiskFreeSpaceEx(lpcwstrDriver,&liFreeBytesAvailable,&liTotalBytes,&liTotalFreeBytes)) {
        return -1;
    }
    space = (qint64)liTotalFreeBytes.QuadPart;
#endif
    return space;
}

QString XFunc::toQuery(const QJsonObject &obj)
{
    QString query = QString("?productType=%1&").arg(productType.toInt());
    QStringList keys = obj.keys();
    for (int i = 0; i < keys.size(); i++) {
        query += QString("%1=%2&").arg(keys.at(i)).arg(QString(QUrl::toPercentEncoding(obj.value(keys.at(i)).toString())));
    }

    query.remove(query.lastIndexOf("&"), 1);

    return query;
}

QString XFunc::exe(QString exe)
{
#ifdef Q_OS_WIN
    if (!exe.endsWith(".exe"))
        exe.append(".exe");
#else
    if (exe.endsWith(".exe"))
        exe.remove(exe.length()-4, 4);
#endif
    return exe;
}

