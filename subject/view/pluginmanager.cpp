#include "pluginmanager.h"

#include <Windows.h>
#include <QDir>
#include <QDebug>
#include "tool/xfunc.h"
#include "tool/regedit.h"
#include "config/userinfo.h"



PluginManager::PluginManager(QObject *parent) : QObject(parent)
{
    allMaxPath();
    QString ms_name = MS_NAME;

    if (UserInfo::instance()->isOS()) {
        ms_name = ms_name + ".en";
    } else {
        ms_name = ms_name + ".cn";
    }

    m_ms = qApp->applicationDirPath() + "/mobao/" + ms_name;
    if (!QFileInfo(m_ms).exists()) {
        m_ms = qApp->applicationDirPath() + "/mobao/" + MS_NAME;
    }
        m_msHash = XFunc::fileHash(m_ms);
}

QStringList PluginManager::allMaxPath()
{
    QStringList maxL;
    maxL << "12.0\\MAX-1:409" << "12.0\\MAX-1:804"
         << "13.0\\MAX-1:409" << "13.0\\MAX-1:804"
         << "14.0\\MAX-1:409" << "14.0\\MAX-1:804"
         << "15.0" << "16.0" << "17.0" << "18.0"
         << "19.0" << "20.0" << "21.0" << "22.0" << "23.0"
         //2022开始
         << "24.0" << "25.0" << "26.0" << "27.0" << "28.0";

    foreach (QString Max, maxL) {
        for (int i = 1; i <= 2; ++i) {
            QString p = maxRegPath(i, Max);
            if (!p.isEmpty()) {
                QDir dir(p + "scripts\\Startup\\");
                if (dir.exists()) {
                    m_allMax << dir.absolutePath();
                }
            }
        }
    }
    qDebug()<< m_allMax <<"3DMax地址";
    return m_allMax;
}

QString PluginManager::maxRegPath(int type, QString name)
{
    QString path = QString("SOFTWARE\\Autodesk\\%1\\%2").arg(type == 1 ? "3dsMax" : "3dsMaxDesign").arg(name);
    QString keyName = "Installdir";
    return readReg(path,keyName);
}

QString PluginManager::readReg(QString path, QString keyName)
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

bool PluginManager::comparePlugins()
{
    if (!QFile::exists(m_ms)) {
        qDebug()<< "not found" << m_ms;
        return true;
    }
    for (int i = 0; i < m_allMax.length(); i++){
        QString maxPath = m_allMax.at(i);
        QString dest = maxPath + "/" + MS_NAME;//文件中的.ms文件
        if (!QFile::exists(dest)) {//文件不存在
            qDebug()<<"文件不存在";
            qDebug()<<"compare file none";
            return  false;
        }else {
            if (XFunc::fileHash(dest) != m_msHash) {//哈希值不相等
            qDebug()<<"哈希值不相等";
            qDebug()<<"compare Hash false";
                return false;
            }
        }
    }
    qDebug()<<"哈希值相等";
    qDebug()<<"compare Hash true";
    return true;
}

bool PluginManager::repairPlugin(bool admin)
{
    if (!QFile::exists(m_ms)) {
        qDebug()<< "not found" << m_ms;
        return false;
    }

    bool run = false;
    for (int i = 0; i < m_allMax.length(); ++i) {//地址中含有的文件数
        QString maxPath = m_allMax.at(i);

        QString dest = maxPath + "/" + MS_NAME;//文件中的.ms文件
        bool ok = true;
        if (!QFile::exists(dest)) {//文件不存在
            qDebug()<<"目标文件不存在，需要拷贝";
            ok = QFile::copy(m_ms, dest);//将下载的文件拷贝一份过来
        } else {//文件存在
            if (XFunc::fileHash(dest) != m_msHash) {//哈希值不相等
                qDebug()<<"目标文件哈希值不同，重新拷贝";
                ok = QFile::remove(dest);//文件中的.ms文件移除
                if (ok)//移除成功，拷贝文件
                {
                    ok = QFile::copy(m_ms, dest);
                    qDebug()<<__FUNCTION__<<"是否进行拷贝"<<ok;
                }
            }
        }
        QFile::remove(maxPath + "/" + MS_NAME_OLD);
        if (!ok){
            run = true;
            qDebug()<<"拷贝失败！！！";
        }
    }

    if (run && !admin) {
        int ret = XFunc::runAsAdmin(qApp->applicationFilePath(), "-ms");
        qDebug()<< __FUNCTION__ << "runAsAdmin" << ret;
        if(ret == -1){
            qDebug()<<"管理员权限未拿到";
            return false;//拷贝文件失败，或管理员权限未拿到。
        }
    }
    return true;
}


void PluginManager::removePlugins()
{
    foreach (QString maxPath, m_allMax) {
        QString dest = maxPath + "/" + MS_NAME;
        if (!QFile::exists(dest))
            continue;
        QFile::remove(dest);
    }
}
