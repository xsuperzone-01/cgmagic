#include "userinfo.h"

#include <tool/xfunc.h>
#include <QDir>
#include <QScreen>
#include <QTimer>
#include "tool/jsonutil.h"
#include "common/protocol.h"
#include "db/userdao.h"
#include "tool/network.h"
#include <QTextCodec>
#include "db/userconfig.h"
#include "view/set/set.h"
#include "view/set/maxset.h"
#include "view/set/renderset.h"
#include "tool/regedit.h"
#include <QEventLoop>
#include <QTimer>

PATTERN_SINGLETON_IMPLEMENT(UserInfo);

UserInfo::UserInfo(QObject *parent) :
    QObject(parent),
    m_parent(true),
    m_autoRunSet(NULL),
    m_scaleW(1.0),
    m_scaleH(1.0),
    m_userAuth(false),
    renderTime(0)
{
#ifdef Q_OS_WIN
    m_appDataPath = RegEdit::CU("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "AppData").toString();
#else
    m_appDataPath = getenv("HOME");
#endif
    m_appDataPath.replace('\\', '/');
    m_appDataPath += QString("/%1").arg(thirdGroup());
    QDir().mkpath(m_appDataPath);

    m_clientSet = QString("Software\\Xsuperzone\\Xcgmagic");

    m_allUserPath = m_appDataPath + "/AllUser";
    QDir().mkpath(m_allUserPath);
    qDebug() << m_appDataPath;
    qDebug() << m_allUserPath;

    QString allCfg = QString("%1/config.ini").arg(m_allUserPath);
    QFile acfg(allCfg);
    acfg.open(QIODevice::ReadWrite);
    acfg.close();
    XFunc::ucs2leToUtf8(allCfg);

    m_tempPath = m_allUserPath + "/temp";

    //清除temp目录
    XFunc::veryDel(m_tempPath);
    QDir().mkpath(m_tempPath);

    initService();
    m_autoRunSet = new QSettings("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
}

UserInfo::~UserInfo()
{
    if (m_autoRunSet)
        delete m_autoRunSet;
}

//设置机器码
void UserInfo::setMachineCode(QString machineCode){
    userMachineCode = machineCode;
}

//返回机器码
QString UserInfo::returnMachineCode(){
    return userMachineCode;
}

void UserInfo::initScale()
{
    QScreen* screen = qApp->primaryScreen();
    m_scaleW = screen->size().width() / 1680.0;
    m_scaleH = screen->size().height() / 1050.0;
    if (m_scaleW > 1.0) {
        m_scaleW = 1.0;
    }
    if (m_scaleH > 1.0) {
        m_scaleH = 1.0;
    }
    qDebug()<< __FUNCTION__ << screen->size() << m_scaleW << m_scaleH;
}

float UserInfo::getScaleW()
{
    return m_scaleW;
}

float UserInfo::getScaleH()
{
    return m_scaleH;
}

void UserInfo::initService()
{
    if (m_allUserPath.isEmpty())
        return;

    saveAllIni("Set", "Language", isOS() ? "en_us" : "");

    QJsonObject def;
    def.insert("region", isOS() ? "os" : "");
    def.insert("version", "1");
    def.insert("cgmagic", isOS() ? "https://gw.xrender.cloud" : "https://gw.xrender.com");
    def.insert("bs", "http://bs.xrender.com");
    def.insert("cmax", "http://www.maxplugin.com");

    QString serviceFile = m_allUserPath + "/service.json";
    QFile fr(serviceFile);
    fr.open(QIODevice::ReadOnly);
    QJsonObject obj = JsonUtil::jsonStrToObj(fr.readAll());
    fr.close();

    if (obj.isEmpty() || obj.value("region").toString() != def.value("region").toString()) {
        QFile fw(serviceFile);
        fw.open(QIODevice::WriteOnly|QFile::Truncate);
        fw.write(JsonUtil::jsonObjToByte(def));
        fw.close();

        obj = def;
    }

    m_service = obj.value("cgmagic").toString();
    if (!isOS())
        m_bs = obj.value("bs").toString();
    m_cmax = obj.value("cmax").toString();
}

QString UserInfo::getDownloadUrl(){
    QString serviceFile = m_allUserPath + "/service.json";
    QFile fr(serviceFile);
    fr.open(QIODevice::ReadOnly);
    QJsonObject obj = JsonUtil::jsonStrToObj(fr.readAll());
    fr.close();

    QString url = obj.value("cgmagic").toString();

    return url;
}

QString UserInfo::Service()
{
    return m_service;
}

QString UserInfo::profiles()
{
    if (m_service.indexOf("dev") >= 0)
        return "dev";
    else if (m_service.indexOf("test") >= 0)
        return "test";
    else
        return "prod";
}

//www.xrender.com 测试和生产的ip不同
UserInfo::FnInfo UserInfo::Fn()
{
    FnInfo info;
    if (service.contains("dev")) {
        info.urlPrefix = "https://www.xrender.com/v23/";
        info.certifyUrl = "https://dev-certify-web.cudatec.com/";
    } else if (service.contains("test")) {
        info.urlPrefix = UserInfo::isEns() ? "https://test-os-www.xrender.com/" : "https://www.xrender.com/v23/";
        info.certifyUrl = UserInfo::isEns() ? "http://test-os-certify-web.cudatec.com/" :"http://test-certify-web.cudatec.com/";
    } else {
        info.urlPrefix = UserInfo::isEns() ? "https://www.xrender.cloud/" : "https://www.xrender.com/v23/";
        info.certifyUrl =  UserInfo::isEns() ? "https://certify.xrender.cloud/" : "https://certify.xrender.com/";
    }
    info.result = QString("coco:%1,youyou:%2,plunge:%3")
            .arg(getSession())
            .arg(userId())
            .arg(QString(sign));
    info.loginUrl = "0";
    if (!UserInfo::instance()->isParent()) {
        info.loginUrl = "1";
    }
    QDateTime time = QDateTime::currentDateTime();   //获取当前时间
    int timeT = time.toSecsSinceEpoch();
    info.timestamp = QString::number(timeT);
    return info;
}

QString UserInfo::Fn(QString path, QString domain)
{
    FnInfo info = Fn();
    reqSsoCode();
    QString url = QString("%1%2&s=%3&co=%4&go=%5&code=%6")
            .arg(domain.isEmpty() ? info.urlPrefix : domain)
            .arg(path)
            .arg(info.loginUrl)
            .arg(info.result)
            .arg(info.timestamp)
            .arg(ssoCode());
    return url;
}

// clientType: 1:跳转 2:内嵌
QString cgPersonal(QString path, int clientType = 1) {
    QString s = USERINFO->Service();
    QString domain;
    if (s.contains("dev")) {
        domain = "http://dev-xmax.cudatec.com";
    } else if (s.contains("test")) {
        domain = USERINFO->isEns() ? "https://test-os-www.xrender.com" : "https://www.xrender.com";
    } else {
        domain = USERINFO->isEns() ? "https://www.xrender.cloud": "https://www.xrender.com";
    }

    QString code = USERINFO->reqSsoCode();
    QString url = QString("%1%2?code=%3&loginType=1&clientType=%4").arg(domain).arg(path).arg(code).arg(clientType);
    return url;
}

QString UserInfo::openPersonalCenter()
{
    QString url = cgPersonal("/CGPersonal/home");
    qDebug()<< __FUNCTION__ << url;
    return url;
}

QString UserInfo::openRecharge()
{
    QString url = cgPersonal("/CGPersonal/member-client", 2);
    qDebug()<< __FUNCTION__ << url;
    return url;
}

//个人中心地址
QString UserInfo::getPersonalUrl(QString url){
    QString personalUrl = cgPersonal(url, 1);
    qDebug()<< "getPersonalUrl is:"<< personalUrl;
    return personalUrl;
}

//云转模型url
QString UserInfo::getChangeMaxUrl(){
    QString url = cgPersonal("/CGPersonal/client-convert-version", 2);
    qDebug()<< "getChangeMaxUrl is:"<< url;
    return url;
}

//云转材质url
QString UserInfo::getChangeMaterialUrl(){
    QString url = cgPersonal("/CGPersonal/client-convert-material-quality", 2);
    qDebug()<< "getChangeMaterialUrl is:" << url;
    return url;
}

//公告网址
QString UserInfo::openNoticeUrl(){
    QString url = cgPersonal("/CGPersonal/free-trial-launch-announcement-client", 2);
    qDebug()<< __FUNCTION__ << url;
    return url;
}

QString UserInfo::openMacSet()
{
    QString url = Fn(QString("%1nologin?redirect=/home").arg(prependCenter()));
    qDebug()<< __FUNCTION__ << url;
    return url;
}

QString UserInfo::openSubscribe()
{
    QString url = cgPersonal("/CGPersonal/account/recharge/member");
    qDebug()<< __FUNCTION__ << url;
    return url;
}

QString UserInfo::openDetail()
{
    if(UserInfo::isEns()){
        if (service.contains("dev")) {
        } else if (service.contains("test")) {
             return "https://test-os-www.xrender.com/cgPricing";
        } else {
           return "https://www.xrender.cloud/cgPricing";
        }
    }else{
      return "https://www.xrender.com/v23/price/CgMagicPrice";
    }
}

QString UserInfo::openRegister()
{
    FnInfo info = Fn();
    QString url = "";
    if(UserInfo::isEns()){
        url = QString("%1register").arg(info.urlPrefix);
    }else{
        url = QString("%1register?c=MTpkMTE1ZjFjZmRhY2Y0MTQ2YTE5NjFjNTZiODQxYTkwYw&pt=3").arg(info.urlPrefix);
    }
    qDebug()<< __FUNCTION__ << url;
    return url;
}

QString UserInfo::openDiscord()
{
    return QString("https://discord.com/invite/CsgPsKbkbp");
}


QString UserInfo::openForgetPwd()
{
    FnInfo info = Fn();
    QString url = QString("%1account/reset").arg(info.certifyUrl);
    return url;
}

QString UserInfo::openHelp()
{
    return  UserInfo::isEns() ? "http://docs.xrender.cloud/cg/" : "http://help.xrender.com/xmax/quickstar/CGMagic.html";
}

QString UserInfo::prependCenter()
{
    return "center/";
}

void UserInfo::setSession(const QString &session)
{
    m_session = session;
}

QString UserInfo::getSession()
{
    return m_session;
}

QString UserInfo::dmpPath()
{
    QString path = m_allUserPath + "/dmp";
    QDir().mkpath(path);
    return path;
}

QString UserInfo::tempPath()
{
    return m_tempPath;
}

QString UserInfo::mac()
{
    if (m_mac.isEmpty()) {
        m_mac = XFunc::MAC();
    }

    return m_mac;
}

QString UserInfo::appDataPath()
{
    return m_appDataPath;
}

QString UserInfo::allUserPath()
{
    return m_allUserPath;
}

QString UserInfo::thirdGroup()
{
    if (!m_third.isEmpty())
        return m_third;

    QString exe;
#ifdef Q_OS_WIN
    WCHAR buff[MAX_PATH] = {0};
    GetModuleFileName(NULL, buff, MAX_PATH);
    exe = QString::fromStdWString(buff);
#endif

#ifdef Q_OS_MAC
    exe = MacUtil::exePath();
#endif

    QString appDir = QFileInfo(exe).absolutePath();

    //读取三方信息
    qDebug()<< QFile(appDir + "/XR.ini").exists() << appDir;
    QSettings set(appDir + "/XR.ini", QSettings::IniFormat);
    foreach (QString code, QStringList()<< "GB2312" << "UTF-8") {
        m_third = set.value("info/third").toString();//empty就是自己
        m_lang = set.value("info/lang").toString();
        if (!m_third.isEmpty())
            break;
    }

    if (m_third.isEmpty())
        m_third = "Xcgmagic";

    qDebug()<< __FUNCTION__ << m_third << m_lang;
    return m_third;
}

QString UserInfo::thirdTrayName()
{
    return qApp->applicationDisplayName();
}

void UserInfo::initUser(QString userName, QJsonObject userInfo)
{
    m_userInfo = userInfo;
    m_userName = userName;

    m_userPath = m_appDataPath + QString("/%1").arg(validUserId());
    QDir().mkpath(m_userPath);
    qDebug() << m_userPath;

    QString userCfg = QString("%1/config.ini").arg(m_userPath);
    QFile file(userCfg);
    file.open(QIODevice::ReadWrite);
    file.close();

    QDir().mkpath(QString("%1/%2_%3").arg(m_appDataPath).arg(validUserId()).arg(m_userName));

    QJsonObject po = userInfo.value("permission").toObject();
    m_xgtRole.xgtRoleData(po);
    m_xgtPermission = JsonUtil::jsonObjToStr(po);

    Set::initSetIni();
    MaxSet::initMaxSetIni();
    RenderSet::initRenderSetIni();
}

QJsonObject UserInfo::getUserInfo()
{
    return m_userInfo;
}

void UserInfo::setUserName(QString name)
{
    m_userName = name;
}

QString UserInfo::userName()
{
    return m_userName;
}

QString UserInfo::getUserPath()
{
    return m_userPath;
}

void UserInfo::setUserId(const QString& userid)
{
    m_userId = userid;
}

QString UserInfo::userId()
{
    return m_userId;
}

void UserInfo::setUserIdL(QString userId)
{
    m_userIdL = userId;
}

QString UserInfo::userIdL()
{
    return m_userIdL;
}

QString UserInfo::validUserId()
{
    if (!m_userId.isEmpty())
        return m_userId;
    return m_userIdL;
}

void UserInfo::setParent(bool p)
{
    m_parent = p;
}

bool UserInfo::isParent()
{
    return m_parent;
}

void UserInfo::setUserAuth(bool auth)
{
    m_userAuth = auth;
}

bool UserInfo::isUserAuth()
{
    return m_userAuth;
}

QString UserInfo::reqSsoCode()
{
    QJsonObject single;
    single.insert("clientId", USERINFO->clientId());
    FuncBody f = NET->sync(POST, loginSingleUrl, single, 10);
    QString code = f.j.value("data").toObject().value("data").toObject().value("code").toString();
    USERINFO->setSsoCode(code);
    return code;
}

void UserInfo::setSsoCode(QString code)
{
    qDebug()<< __FUNCTION__ << code;
    m_ssoCode = code;
}

QString UserInfo::ssoCode()
{
    return m_ssoCode;
}

QString UserInfo::clientId()
{
    return "0073b5b3f9b8457c8c0981300ae79595";
}

bool UserInfo::isImc()
{
    return m_userInfo.value("imc").toBool();
}

QString UserInfo::appLang()
{
    if (m_appLang.isEmpty()) {
        m_appLang = RegEdit::US(m_clientSet, "Language").toString();
        if (m_appLang.isEmpty())
        {
            m_appLang = "zh_cn";
        }
    }

    return m_appLang;
}

bool UserInfo::isEns()
{
    return appLangNum() == language_en ? true:false;
}

bool UserInfo::isOS()
{
    return "en_us" == m_lang;
}

UserInfo::Languages UserInfo::appLangNum()
{
    QString lan = Set::changeLan();

    if (lan.isEmpty()) {
        lan = "zh_cn";
    }
    if (lan == "zh_cn") {
        return UserInfo::language_cn;
    } else if (lan == "en_us") {
        return UserInfo::language_en;
    } else {
        return UserInfo::language_cn;
    }
}

bool UserInfo::isAutoRun()
{
    if (m_autoRunSet == NULL) {
        qDebug() << __FUNCTION__ << "m_autoRunSet is NULL";
        return false;
    }

    QString exe = QString("%1.exe").arg(qApp->applicationName());
    return m_autoRunSet->contains(exe);
}

bool UserInfo::AutoRun(bool Auto)
{
    if (m_autoRunSet == NULL) {
        qDebug() << __FUNCTION__ << "m_autoRunSet is NULL";
        return false;
    }

    bool regOk = false;
    QString exe = QString("%1.exe").arg(qApp->applicationName());
    if (Auto) {
        QString dir = qApp->applicationDirPath();
        dir.replace("/", "\\");
        QString val = dir + QString("\\%1 RegAutoRun").arg(exe);
        m_autoRunSet->setValue(exe, val);
        regOk = m_autoRunSet->contains(exe);
    } else {
        m_autoRunSet->remove(exe);
        regOk = !m_autoRunSet->contains(exe);
    }

    qDebug() << __FUNCTION__ << regOk;
    if (!regOk) {
        qDebug() << __FUNCTION__ << "autorun reg write error";
    }

    return regOk;
}

bool UserInfo::isFirstRun()
{
    return readAllIni("", "isFirstRun").toInt() == 0;
}

void UserInfo::setFirstRun(int run)
{
    saveAllIni("", "isFirstRun", run);
}

void UserInfo::saveUserIni(const QString &group, const QString &key, const QVariant &value)
{
    QSettings set(m_userPath + "/config.ini", QSettings::IniFormat);
    set.beginGroup(group);//SetG
    set.setValue(key, value);//"loginSilent", true（静默登录）
    set.endGroup();
}

QVariant UserInfo::readUserIni(const QString &group, const QString &key)
{
    QSettings set(m_userPath + "/config.ini", QSettings::IniFormat);
    return set.value(QString("%1/%2").arg(group).arg(key));
}

bool UserInfo::existUserIni(const QString &group, const QString &key)
{
    QSettings set(m_userPath + "/config.ini", QSettings::IniFormat);
    return set.contains(QString("%1/%2").arg(group).arg(key));
}

void UserInfo::saveAllIni(const QString &group, const QString &key, const QVariant &value)
{
    QSettings set(m_allUserPath + "/config.ini", QSettings::IniFormat);
    set.beginGroup(group);
    set.setValue(key, value);
    set.endGroup();
}

QVariant UserInfo::readAllIni(const QString &group, const QString &key)
{
    QSettings set(m_allUserPath + "/config.ini", QSettings::IniFormat);
    return set.value(QString("%1/%2").arg(group).arg(key));
}

bool UserInfo::existAllIni(const QString &group, const QString &key)
{
    QSettings set(m_allUserPath + "/config.ini", QSettings::IniFormat);
    return set.contains(QString("%1/%2").arg(group).arg(key));
}

void UserInfo::setAccessToken(QByteArray token)
{
    if (m_accToken != token) {
        m_accToken = token;

        QByteArray byte;
        for (int i = 0; i < 16; ++i) {
            QString part = token.mid(i*2,2);
            short s = part.toShort(0,16);
            uchar uc = (uchar)(0x00ff & s);
            byte.append(uc);
        }
        m_accToken16 = byte;
    }
}

QByteArray UserInfo::accessToken()
{
    return m_accToken;
}

QByteArray UserInfo::accessToken16()
{
    return m_accToken16;
}

bool UserInfo::isMaxPath(QString path)
{
    if (path.contains("AppData") && path.contains("Local")) {
        return true;
    }

    //适配路径不对时中间修改过ini
    if (!m_maxSavePath.isEmpty()) {
        QString save = m_maxSavePath;
        return QDir::cleanPath(path).startsWith(save);
    }
    return false;
}

QString UserInfo::maxPath()
{
    return m_maxSavePath;
}

QString UserInfo::clientSet()
{
    return m_clientSet;
}

QJsonObject UserInfo::userCfg()
{
    QJsonObject obj;
    obj.insert("CustomProjectEn", readUserIni("Set", "CustomProjectEn").toInt());
    obj.insert("ProjectManager", readUserIni("Set", "ProjectManager").toString());
    obj.insert("ProjectManager2", readUserIni("Set", "ProjectManager2").toString());
    obj.insert("ResultDir", RenderSet::cacheDir());
    obj.insert("ResultDirType", RenderSet::pushTo() == RenderSet::PushTo::Src ? "FileDir" : "ResultDir");
    obj.insert("ResultDirFormat", RenderSet::channel() ? 1 : 0);
    return obj;
}

QString UserInfo::bs()
{
    return m_bs;
}

QString UserInfo::cmax()
{
    return m_cmax;
}

int UserInfo::vrmode()
{
    return readUserIni("Set", "vrmode").toInt();
}

void UserInfo::setVrmode(int mode)
{
    saveUserIni("Set", "vrmode", mode);
}

RolePermission::RolePermission()
{
    download = 1;
}

void RolePermission::xgtRoleData(QJsonObject obj)
{
    if (!obj.isEmpty()) {
        if (!obj["download"].toBool())
            download = 0;
    }
}
