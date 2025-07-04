#ifndef USERINFO_H
#define USERINFO_H

#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QTranslator>
#include <QVariant>

#include "common/Singleton.h"

class RolePermission {
public:
    int download;

    RolePermission();
    void xgtRoleData(QJsonObject obj);
};

class UserInfo : public QObject
{
    Q_OBJECT

    PATTERN_SINGLETON_DECLARE(UserInfo);
    struct FnInfo {
        QString urlPrefix;
        QString certifyUrl;
        QString loginUrl;
        QString result;
        QString timestamp;
    };

public:
    void initScale();
    float getScaleW();
    float getScaleH();

    void initService();
    QString Service();
    QString profiles();

    enum Languages {
        language_cn,
        language_en
    };

    FnInfo Fn();
    QString Fn(QString path, QString domain = "");
    QString openPersonalCenter();
    QString openRecharge();
    QString openMacSet();
    QString openSubscribe();
    QString openDetail();
    QString openDiscord();
    QString openRegister();
    QString openForgetPwd();
    QString openHelp();
    QString prependCenter();
    void setSession(const QString& session);
    QString getSession();

    QString dmpPath();
    QString tempPath();

    QString mac();
    QString appDataPath();
    QString allUserPath();
    QString appLang();
    bool isEns();
    bool isOS();
    Languages appLangNum();
    QString thirdGroup();
    QString thirdTrayName();

    void initUser(QString userName, QJsonObject userInfo);
    QJsonObject getUserInfo();
    void setUserName(QString name);
    QString userName();
    QString getUserPath();
    void setUserId(const QString& userid);
    QString userId();
    void setUserIdL(QString userId);
    QString userIdL();
    QString validUserId();
    void setParent(bool p);
    bool isParent();
    void setUserAuth(bool auth);
    bool isUserAuth();
    QString reqSsoCode();
    void setSsoCode(QString code);
    QString ssoCode();
    QString clientId();
    bool isImc();

    bool isAutoRun();
    bool AutoRun(bool Auto);

    bool isFirstRun();
    void setFirstRun(int run);

    void saveUserIni(const QString& group, const QString& key, const QVariant &value);
    QVariant readUserIni(const QString& group, const QString& key);
    bool existUserIni(const QString& group, const QString& key);

    void saveAllIni(const QString& group, const QString& key, const QVariant &value);
    QVariant readAllIni(const QString& group, const QString& key);
    bool existAllIni(const QString& group, const QString& key);

    //从老效果图迁来
    void setAccessToken(QByteArray token);
    QByteArray accessToken();
    QByteArray accessToken16();
    bool isMaxPath(QString path);
    QString maxPath();
    QString clientSet();
    QJsonObject userCfg();
    QString bs();
    QString cmax();
    int vrmode();
    void setVrmode(int mode);

    QString m_xgtPermission;
    RolePermission m_xgtRole;

    //设置机器码+返回机器码
    void setMachineCode(QString machineCode);
    QString returnMachineCode();

    QString openNoticeUrl();  //公告网址
    QString getChangeMaxUrl();  //云转模型网址
    QString getChangeMaterialUrl(); //云转材质网址
    QString getDownloadUrl();
    QString getPersonalUrl(QString url);    //个人中心网址

private:

    bool m_parent;
    QString m_userName;
    QString m_userPath;
    QString m_userId;
    QString m_userIdL;
    QJsonObject m_userInfo;
    bool m_userAuth;
    QString m_ssoCode;

    QString m_mac;
    QString m_appDataPath;
    QString m_allUserPath;
    QString m_tempPath;
    QString m_appLang;
    QString m_clientSet;
    QString m_session;
    QString m_service;
    QString m_third;
    QString m_lang;

    QPointer<QSettings> m_autoRunSet;

    float m_scaleW;
    float m_scaleH;

    //从老效果图迁来
    QByteArray m_accToken;
    QByteArray m_accToken16;
    QString m_maxSavePath;
    QString m_bs;
    QString m_cmax;

    //定义机器码变量
    QString userMachineCode;

public:
    QJsonObject machineInformation;
    int renderTime; //渲染时长
    QJsonObject userStatusObj;   //用户身份状态
};

#define USERINFO UserInfo::instance()

#endif // USERINFO_H
