#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "tool/xfunc.h"
#include "config/userinfo.h"

// 软件单例端口
static int SinglePort = 47166;

enum CodeSection {
    Default,
    LoginCenter,
    NodeAction,
};

extern QMap<CodeSection, QMap<QString, QString>> codeMap;
QString codeView(QString code, QString m = "");
QString codeView(QString resp, CodeSection s);
int codeI(QString resp);
QString codeS(QString resp);
QString codeMsg(QString resp);
void setCodeMap();

bool IsValidPwd(const QString &pwd);

int randomPort(int port);
void changeAppname(bool parent);

extern QByteArray appname;
extern QByteArray sign;
static const QByteArray productType = "3";

static QString service = USERINFO->instance()->Service();

static QString UC = "/uc";
static QString AC = "/infra-account-center";
static QString CG_CONSOLE = "/cgmagic-console";

//TODO
//static QString service = "http://10.23.100.138";
//static QString UC = ":8850";
//static QString XDEMO_CORE = ":8920";
//static QString XDEMO_CONFIG = ":8922";
//static QString AC = ":8850";

//账号中心 POST
static QString accountCenterUrl = service + "/infra-account-center/oauth/authorize";
//static QString accountCenterUrl = service + "/oauth/authorize";

//新的登录
static QString loginAuthUrl = service + CG_CONSOLE + "/user/login";
//static QString loginAuthUrl = "http://10.23.100.195:13000/user/login";

//单点授权
static QString loginSingleUrl = service + CG_CONSOLE + "/user/authorize";

//获取账号信息
static QString loginUserInfoUrl = service + UC + "/validation/xneofss";


/*余额查询      GET */
//static QString balanceUrl = service + "/xneouc/money";

static QString balanceUrl = service + AC + "/wallet/amount?walletType=1&productType=3&productWalletType=32";

/* 会员有效期 GET*/
static QString vipUrl = service + AC + "/vip/validity/period";

/*实名认证*/
static QString userAuthUrl = service + "/infra-account-center/user/certify/info";

/* GET 查询全局可见和该用户指定可见的模式 */
static QString renderModeUrl = service + CG_CONSOLE + "/render/query/user/mode";

/* GET 客户端不能使用模块编号 */
static QString pluginLicUrl = service + CG_CONSOLE + "/module/select";

//更新接口
static QString ginPort = "48163";

// 获取更新内容  GET
static QString listUpdateUrl = "http://127.0.0.1:" + ginPort + "/softupgrade/force";

// 更新       PUT
static QString updateUrl = "http://127.0.0.1:" + ginPort + "/softupgrade/force";

//更新状态      GET
static QString updateStatusUrl = "http://127.0.0.1:" + ginPort + "/softupgrade/updatestatus";

//更新程序退出    POST
static QString updateExitUrl = "http://127.0.0.1:" + ginPort + "/softupgrade/exitApp";

// goCmd put
static QString goCmdUrl = "http://127.0.0.1:" + ginPort + "/softupgrade/cmd";

//Get--官方客户端（登录）--包含免费模块、授权模块和隐藏模块（无论Vip状态到期还是未到期）
static QString universalClientUrl = service + CG_CONSOLE + "/module/select/new";

//无授权用户使用授权模块的可用时长
static QString freeUserForAuthorizeModelUrl = service + CG_CONSOLE + "/module/time/free";

//GET--固定文本内容的地址
static QString advertiseTitleUrl = service + CG_CONSOLE + "/system/parameter/query";

//GET-客户端横幅信息
static QString bannerActivityUrl = service + CG_CONSOLE + "/banner/detail";

//GET-客户端活动列表
static QString presentActivityUrl = service + CG_CONSOLE + "/activity/list";

//云工具次数
static QString cloudInfos = service + CG_CONSOLE + "/task/conversion/count";

extern int svcGinPort;
//netdiskSvc.exe的服务
extern QString localSvc;
//底层通知前端的udp端口
extern int noticePort;

class Verify_Lock {
public:
    bool succ;
    QString pwd;

    Verify_Lock() {
        succ = 0;
        pwd = QString();
    }
};


#endif // PROTOCOL_H
