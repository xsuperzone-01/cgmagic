#include "basewidget.h"

#include <QDebug>
#include <QApplication>
#include <QLocale>
#include <QStandardPaths>
#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <qmath.h>
#include <QTextCodec>
#define NOMINMAX
#include <Windows.h>
#include "../xfunc.h"
#include <QGraphicsDropShadowEffect>

bool BaseWidget::m_cn = true;
QString BaseWidget::m_exe;
QString BaseWidget::m_third;
QSettings *BaseWidget::m_autoRunSet;
QSettings *BaseWidget::m_uninstSet;
QSettings *BaseWidget::m_xrootSet;
QSettings *BaseWidget::m_xszSet;
QSettings *BaseWidget::m_clientSet;

BaseWidget::BaseWidget(QWidget *parent) :
    QWidget(parent),
    m_movePoint(QPoint(0,0))
{
    setWindowFlags(Qt::FramelessWindowHint|Qt::WindowMinimizeButtonHint|Qt::WindowMaximizeButtonHint);
    setAttribute(Qt::WA_TranslucentBackground);

    if (!m_autoRunSet)
        m_autoRunSet = new QSettings("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (!m_uninstSet)
        m_uninstSet = new QSettings("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall", QSettings::NativeFormat);

    m_appPath = qApp->applicationDirPath();
    m_instPath = m_appPath;
    qDebug()<< "BaseWidget: install path" << m_instPath;

    //只在安装时存在ini
    if (!QFile::exists(m_appPath + "/XR.ini")) {
        QString info = clientSet()->value("info").toString();
        QJsonObject ino = XFunc::jsonStrToObj(info);

        m_third = ino.value("third").toString();
        m_name = ino.value("name").toString();
        m_nameEN = ino.value("nameEN").toString();
        m_exe = ino.value("exe").toString();
        m_mark = ino.value("mark").toString();
        m_lang = ino.value("lang").toString();
    } else {
        //读取三方信息
        QSettings set(m_appPath + "/XR.ini", QSettings::IniFormat);
        // set.setIniCodec(QTextCodec::codecForName("GB2312"));
        m_exe = set.value("info/exe").toString();
        m_name = set.value("info/name").toString();
        m_nameEN = set.value("info/nameEN").toString();
        m_third = set.value("info/third").toString();//empty就是自己
        m_mark = set.value("info/mark").toString();
        m_lang = set.value("info/lang").toString();
    }

    m_cn = (m_lang == "en_us" ? false : true);

    QTimer::singleShot(50, this, [=]{
        setWindowTitle(m_name);
    });
}

QString BaseWidget::lang()
{
    return m_lang;
}

bool BaseWidget::needMove(QMouseEvent *e)
{
    return true;
}

bool BaseWidget::langType()
{
    return m_cn;
}

QString BaseWidget::stdAppPath()
{
    return "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs";
//    return QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
}

QString BaseWidget::stdDeskPath()
{
    return "C:\\Users\\Public\\Desktop";
//    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

bool BaseWidget::existProcess(QString proName)
{
    if (!proName.endsWith(".exe"))
        proName.append(".exe");
    QProcess process;
    process.start("tasklist.exe", QStringList() << "-fi" << QString("imagename eq %1").arg(proName));
    if (loopProcess(&process)) {
        QByteArray byte = process.readAll();
        if (byte.contains(proName.toUtf8()))
            return true;
    }
    return false;
}

void BaseWidget::killProcess(const QString &proName)
{
    QProcess process;
    process.start("taskkill.exe", QStringList() << "-F" << "/IM" << proName);
    if (loopProcess(&process)) {
        QByteArray byte = process.readAll();
        qDebug()<<byte;
    }
}

bool BaseWidget::loopProcess(QProcess *pro, int msecs)
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

void BaseWidget::veryDel(const QString &dir)
{
    if (dir.isEmpty())
        return;
    qDebug()<<dir;
    QFileInfo info(dir);
    if (info.isSymLink() || info.isFile()) {
        QFile file(info.absoluteFilePath());
        file.setPermissions(QFile::WriteOwner);
        file.remove();
        return;
    }

    QDirIterator d(dir, QDir::Files, QDirIterator::Subdirectories);
    while (d.hasNext()) {
        d.next();
        QFileInfo info = d.fileInfo();
        qDebug()<<"subfile:"<<info.absoluteFilePath()<<info.fileName();
        QFile file(info.absoluteFilePath());
        file.setPermissions(QFile::WriteOwner);
        qDebug()<<"subfile:"<<file.remove();
    }
    QDirIterator d2(dir, QDir::Dirs, QDirIterator::Subdirectories);
    while (d2.hasNext()) {
        d2.next();
        QFileInfo info = d2.fileInfo();
        qDebug()<<"subdir:"<<info.absoluteFilePath();
        qDebug()<<"subdir:"<<QDir().rmpath(info.absoluteFilePath());
    }
    qDebug()<<"dir:"<<QDir().rmpath(dir);
}

QString BaseWidget::readReg(QString path, QString keyName)
{
    QString value;

    HKEY key;
    std::wstring pathWstr = path.toStdWString();
    LPCWSTR subkey = pathWstr.data();

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,subkey,0,KEY_READ|KEY_WOW64_64KEY,&key)==0){
        BYTE *buff = new BYTE[1024];
        std::wstring keyNameWstr = keyName.toStdWString();
        LPCWSTR subkey = keyNameWstr.data();
        ULONG cbSize = 1024;
        if (RegQueryValueEx(key,subkey,NULL,NULL,buff,&cbSize) == 0)
            value = QString::fromUtf16((ushort*)buff);

        qDebug()<<RegCloseKey(key);
        delete [] buff;

    }

    return value;
}

QString BaseWidget::thirdInfo()
{
    //读取三方信息
    QSettings set(qApp->applicationDirPath() + "/XR.ini", QSettings::IniFormat);
    // set.setIniCodec(QTextCodec::codecForName("GB2312"));
    return set.value("info/third").toString();//empty就是自己
}

QString BaseWidget::thirdGroup()
{
    return "Xcgmagic";
}

QSettings *BaseWidget::clientSet()
{
    QString xrootSet = "HKEY_USERS\\.DEFAULT\\Software";
    QString xszSet = xrootSet + "\\Xsuperzone";
    QString clientSet = xszSet + "\\" + thirdGroup();
    if (!m_xrootSet)
        m_xrootSet = new QSettings(xrootSet, QSettings::NativeFormat);
    if (!m_xszSet)
        m_xszSet = new QSettings(xszSet, QSettings::NativeFormat);
    if (!m_clientSet)
        m_clientSet = new QSettings(clientSet, QSettings::NativeFormat);

    return m_clientSet;
}

QSettings *BaseWidget::clientSetNewWrite(){
    QString xrootSet = "HKEY_USERS\\.DEFAULT\\Software";
    QString xszSet = xrootSet + "\\Xsuperzone";
    QString clientSet = xszSet + "\\" + thirdGroup();

    QSettings* m_clientSet = new QSettings(clientSet, QSettings::NativeFormat);

    return m_clientSet;
}


void BaseWidget::removeClientSet(bool uninstall)
{
    BaseWidget::clientSet();
    if (m_xrootSet->childGroups().contains("Xsuperzone")) {
        if (uninstall || m_clientSet->childKeys().isEmpty()) {
            m_xszSet->remove("Xcgmagic");
            if (m_xszSet->childGroups().isEmpty())
                m_xrootSet->remove("Xsuperzone");
        }
    }
}

void BaseWidget::removeLnk(QString target)
{
    QString userDesk = stdDeskPath();
    QStringList pubL = userDesk.split("/");
    pubL.replace(2, "Public");

    QStringList deskL;
    deskL << userDesk << pubL.join("/");
    for (int i = 0; i < deskL.length(); ++i) {
        QDir pubDir(deskL.at(i));
        QFileInfoList pubLnkL = pubDir.entryInfoList(QStringList() << "*.lnk");
        foreach (QFileInfo tmp, pubLnkL) {
            if (QFile(tmp.absoluteFilePath()).symLinkTarget() == target) {
                QFile::remove(tmp.absoluteFilePath());
                break;
            }
        }
    }
}

#include <tchar.h>
#include "stdio.h"
#include "WinVer.h"
struct
{
    WORD wLanguage;
    WORD wCodePage;
} *lpTranslate;

struct
{
    WORD w;
} *lp;
int BaseWidget::ChangeFileInfo(QString file, QString FileDescription, QString value)
{
    LPCWSTR lpszFile = (LPCWSTR)file.unicode();

    BOOL bRet = FALSE;
    DWORD dwHandle = 0;
    DWORD dwSize = 0;

    dwSize = GetFileVersionInfoSize(lpszFile, &dwHandle);

    if( 0 >= dwSize)
        return bRet;

    LPBYTE lpBuffer = new BYTE[dwSize];
    memset( lpBuffer, 0, dwSize );

    if (GetFileVersionInfo(lpszFile, dwHandle, dwSize, lpBuffer) != FALSE)
    {
        HANDLE hResource = BeginUpdateResource(lpszFile, FALSE);
        if (NULL != hResource)
        {

            UINT uTemp;
            DWORD dwVer[4] = {0};


            QString tl = "\\VarFileInfo\\Translation";
            LPCWSTR wtl = tl.toStdWString().c_str();
            if (VerQueryValue(lpBuffer, wtl, (LPVOID *)&lpTranslate, &uTemp))
            {
                qDebug() << "VerQueryValue 0" << lpTranslate->wLanguage << lpTranslate->wCodePage;

                // 修改版本的文本信息
                LPTSTR lpStringBuf = NULL;
                DWORD dwStringLen = 0;
                TCHAR szTemp[MAX_PATH];


                UINT lpsize;
                LPVOID lpData;

                QString sfi = "\\StringFileInfo\\080404B0\\" + FileDescription;

                if(VerQueryValue(lpBuffer, (LPCWSTR)sfi.unicode()/*L"\\StringFileInfo\\080404B0\\FileDescription"*/, (LPVOID*)&lpData, &lpsize) != FALSE ) {
                    std::wstring stds = (LPCTSTR)lpData;
                    QString src = QString::fromStdWString(stds);
                    qDebug() << "FileDescription:" << QString::fromStdWString(stds);

                    //clear
                    src.fill(QChar());
                    std::wstring clear = src.toStdWString();
                    memcpy(lpData, clear.data(), clear.length()*2);

                    QString rep = value;
                    std::wstring repws = rep.toStdWString();
                    memcpy(lpData, repws.data(), repws.length()*2);
                }

                // 更新
                if (UpdateResource(hResource, RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO), lpTranslate->wLanguage, lpBuffer, dwSize) != FALSE)
                {
                    qDebug() << "UpdateResource";
                    if (EndUpdateResource(hResource, FALSE) != FALSE)
                        bRet = TRUE;
                }
            }
        }
    }

    if( lpBuffer )
        delete [] lpBuffer;

    return bRet;
}

void BaseWidget::setDropShadow(QWidget *wid, int x, int y, int radius, QColor color)
{
    if (QGraphicsEffect *ef = wid->graphicsEffect()) {
        ef->deleteLater();
    }
    QGraphicsDropShadowEffect *shadow_effect = new QGraphicsDropShadowEffect(wid);
    shadow_effect->setOffset(x, y);
    shadow_effect->setColor(color);
    shadow_effect->setBlurRadius(radius);
    wid->setGraphicsEffect(shadow_effect);
}

QString BaseWidget::globalStyle(bool lan)
{
    QFile sf(":/default.qss");
    sf.open(QFile::ReadOnly);
    QByteArray sb = sf.readAll();
    if (!lan) {
        QFile sf(":/default_en.qss");
        sf.open(QFile::ReadOnly);
        sb += sf.readAll();
        qDebug()<<"添加了英文qss补丁";
    }
    sf.close();
    return sb;
}

void BaseWidget::setClass(QWidget *wid, QString className)
{
    setProperty(wid, "class", className);
}

void BaseWidget::setProperty(QWidget *wid, QString name, QVariant value)
{
    if (!wid)
        return;
    wid->setProperty(name.toStdString().c_str(), value);
    wid->setStyle(qApp->style());
}

void BaseWidget::setLetterSpacing(QWidget *wid, qreal space)
{
    QFont f = wid->font();
    f.setLetterSpacing(QFont::AbsoluteSpacing, space);
    wid->setFont(f);
}




void BaseWidget::setIngText(QPointer<QLabel> label, QString text)
{
    if (!label) {
        return;
    }

    label->setText(text);
    QTimer *timer = new QTimer(this);
    timer->start(1000);
    timer->setProperty("count", 0);
    connect(timer, &QTimer::timeout, this, [=]{
        if (!label) {
            timer->stop();
            return;
        }
        int c = timer->property("count").toInt();
        label->setText(text + QString().fill('.', c++ % 4));
        timer->setProperty("count", c);
    });
}



void BaseWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (m_movePoint.x() != 0) {
        move(e->globalPos() - m_movePoint);
    }
    return QWidget::mouseMoveEvent(e);
}

void BaseWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && needMove(e)) {
        m_movePoint = e->globalPos() - pos();
    }
    return QWidget::mousePressEvent(e);
}

void BaseWidget::mouseReleaseEvent(QMouseEvent *e)
{
    m_movePoint = QPoint(0,0);
}

//http://blog.csdn.net/chenlycly/article/details/45418447
//IconFX.exe 可查看.ico
bool BaseWidget::ChangeExeIcon(LPWSTR IconFile, LPWSTR ExeFile)
{
    ICONDIR stID;
    ICONDIRENTRY stIDE;
    GRPICONDIR stGID;
    HANDLE hFile;
    DWORD nSize, nGSize, dwReserved;
    HANDLE hUpdate;
    PBYTE pIcon, pGrpIcon;
    BOOL ret;
    hFile = CreateFile(IconFile, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
       return false;
    }
    ZeroMemory(&stID, sizeof(ICONDIR));
    ret = ReadFile(hFile, &stID, sizeof(ICONDIR), &dwReserved, NULL);

    hUpdate = BeginUpdateResource(ExeFile, false);

    WORD count = stID.idCount;
    QList<ICONDIRENTRY> idL;
    QList<ICONDIRENTRY> idL4;
    QList<ICONDIRENTRY> idL8;
    QList<ICONDIRENTRY> idL32;
    for (int i = 1; i <= count; ++i) {
        ICONDIRENTRY stIDE;
        ZeroMemory(&stIDE, sizeof(ICONDIRENTRY));

        ReadFile(hFile, &stIDE, sizeof(ICONDIRENTRY), &dwReserved, NULL);
        nSize = stIDE.dwBytesInRes;
        WORD bc = stIDE.wBitCount;
        qDebug()<< bc << stIDE.bWidth;
/*
32 128
32 64
32 48
32 32
32 24
32 20
32 16
*/
        if (bc == 32)
            idL32.prepend(stIDE);
    }
    idL << idL4 << idL8 << idL32;
//return true;
    while (idL.length() > 7) {
        idL.removeFirst();
    }
    for (int i = 0; i < 7; ++i) {
        ICONDIRENTRY stIDE = i < idL.length() ? idL.at(i) : idL.last();
        DWORD nSize = stIDE.dwBytesInRes;
        PBYTE pIcon = (PBYTE)malloc(nSize);
        SetFilePointer(hFile, stIDE.dwImageOffset, NULL, FILE_BEGIN);
        ReadFile(hFile, (LPVOID)pIcon, nSize, &dwReserved, NULL);
        UpdateResource(hUpdate, RT_ICON, MAKEINTRESOURCE(7 - i), 0x0409, (LPVOID)pIcon, nSize);
    }
    EndUpdateResource(hUpdate, false);

    CloseHandle(hFile);
    return true;
}

//拷贝文件夹：
bool BaseWidget::copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist)
{
    QDir sourceDir(fromDir);
    QDir targetDir(toDir);
    if(!targetDir.exists())
    {
        /**< 如果目标目录不存在，则进行创建 */
        if(!targetDir.mkpath(targetDir.absolutePath()))
            return false;
    }

    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach(QFileInfo fileInfo, fileInfoList)
    {
        if(fileInfo.fileName() == "." || fileInfo.fileName() == ".." || fileInfo.fileName() == QString("readme.txt"))
            continue;

        if(fileInfo.isDir())
        {
            /**< 当为目录时，递归的进行copy */
            if(!copyDirectoryFiles(fileInfo.filePath(), targetDir.filePath(fileInfo.fileName()), coverFileIfExist))
                return false;
        }
        else
        {
            /**< 当允许覆盖操作时，将旧文件进行删除操作 */
            if(coverFileIfExist && targetDir.exists(fileInfo.fileName()))
            {
                //如果删除失败，将正在使用的文件重命名
                if(!targetDir.remove(fileInfo.fileName()))
                {
                    targetDir.rename(fileInfo.fileName(), QString("Script_tmp_") + fileInfo.fileName());
                }
            }

            /// 进行文件copy
            if(!QFile::copy(fileInfo.filePath(), targetDir.filePath(fileInfo.fileName())))
                return false;
        }
    }

    return true;
}
