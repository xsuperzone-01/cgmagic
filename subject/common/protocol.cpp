#include "protocol.h"

#include <QIntValidator>
#include <QTcpServer>
#include "tool/jsonutil.h"
#define tr QObject::tr

QByteArray appname = "desk";
QByteArray sign = XFunc::Md5("6pLdE8yT").toUtf8();

int svcGinPort = 46166;
QString localSvc = QString("http://127.0.0.1:%1").arg(svcGinPort);
int noticePort = 47165;

QMap<CodeSection, QMap<QString, QString>> codeMap;

/*
    code 具体码 or json
    先拿代码转义的，没有则拿接口返回的
*/
QString codeView(QString code, QString m)
{
    QString msg;
    QJsonObject obj = JsonUtil::jsonStrToObj(code);
    if (!obj.isEmpty()) {
        QJsonObject ret = obj.value("data").toObject();
        code = ret.value("resultCode").toString();
        msg = ret.value("message").toString();
        if (msg.isEmpty())
            msg = ret.value("resultMsg").toString();
    }

    code = m + code;
    QString s;

    if (code == "sublogin20001002") {
        s = QObject::tr("账号或密码不正确，请重新输入");
    } else if (code == "accountError") {
        s = QObject::tr("账号或密码不正确，请重新输入");
    } else if(code == "20000001") {
        s = QObject::tr("未查找到用户");
    } else if(code == "00201003") {
        s = QObject::tr("账号或密码不正确，请重新输入");
    } else if (code == "00202006") {
        s = QObject::tr("手机号或密码不正确，请重新输入");
    } else if (code == "00203009") {
        s = QObject::tr("邮箱或密码不正确，请重新输入");
    } else if (code == "00206019" || code == "sublogin20001004") {
        s = QObject::tr("当前用户已禁止登录");
    } else if (code == "10403") {
        s = QObject::tr("登录过期");
    } else if (code == "12403") {
        s = QObject::tr("您的账号已在其他设备登录，如不是您本人操作，请及时修改密码");
    } else if (code == "13403") {
        s = QObject::tr("密码已重置，请重新登录！");
    } else if (code == "14403") {
        s = QObject::tr("上级账号已修改，请重新登录…");
    } else if (code == "15403") {
        s = QObject::tr("账号信息有变动，请重新检查登录…");
    } else {
        if (!msg.isEmpty()) {
            s = msg;
        } else {
            if (code.isEmpty()) {
                s = QObject::tr("未知错误");
            } else {
                s = QObject::tr("未知错误，") + QString("code:%1").arg(code);
            }
        }
    }

    qDebug() << __FUNCTION__ << code << s;
    return s;
}

bool IsValidLetter(const QString &str) {
    QRegularExpression regx("^[A-Za-z]+$");
    QRegularExpressionValidator regs(regx, 0);
    QString str1 = str;
    int pos = 0;
    QValidator::State res = regs.validate(str1, pos);
    if (QValidator::Acceptable == res) {
        return true;
    }
    else {
        return false;
    }
}

bool IsValidNumber(const QString &num) {
    QRegularExpression regx("^[0-9]+$");
    QRegularExpressionValidator regs(regx, 0);
    QString str = num;
    int pos = 0;
    QValidator::State res = regs.validate(str, pos);
    if (QValidator::Acceptable == res) {
        return true;
    }
    else {
        return false;
    }
}

bool IsCharacter(const QString & charac)
{
    QRegularExpression regx("((?=[\x21-\x7e]+)[^A-Za-z0-9])*$");
    QRegularExpressionValidator regs(regx, 0);
    QString str = charac;
    int pos = 0;
    QValidator::State res = regs.validate(str, pos);
    if (QValidator::Acceptable == res) {
        return true;
    }
    else {
        return false;
    }
}

//    /^(?!^\d+$)(?!^[A-Za-z]+$)(?!^[^A-Za-z0-9]+$)(?!^.*[\u4E00-\u9FA5].*$)^\S{4,20}$/
bool IsValidPwd(const QString &pwd)
{

    if (pwd.isEmpty() || pwd.length() < 4 || pwd.length() > 20) {
        return false;
    }
    if (!IsValidLetter(pwd) && !IsValidNumber(pwd) && !IsCharacter(pwd)) {
        return true;
    }
    return false;
}

int randomPort(int port)
{
    QTcpServer ts;
    bool ok = ts.listen(QHostAddress::LocalHost);
    if (ok) {
        port = ts.serverPort();
        ts.close();
    } else {
        qDebug()<< __FUNCTION__ << ts.serverError() << ts.errorString();
    }
    return port;
}

void changeAppname(bool parent)
{
    if (parent) {
        appname = "desk";
        sign = XFunc::Md5("6pLdE8yT").toUtf8(); //51a2c562b6ca3325cc5bcc72d50e26ae
    } else {
        appname = "desk_xdemo";
        sign = XFunc::Md5("3yUxTKbv").toUtf8(); //5bc1ae0a8ac984e58e33b7a16983c796
    }
}

/*
    Default可以存公用的
*/
void setCodeMap()
{
    {
        QMap<QString, QString> cm;
        codeMap[Default] = cm;
    }
    {
        QMap<QString, QString> cm;
        cm["100"] = QObject::tr("用户名填写错误，请重新填写");
        cm["101"] = QObject::tr("手机号填写错误，请重新填写");
        cm["102"] = QObject::tr("邮箱填写错误，请重新填写");
        cm["103"] = QObject::tr("密码填写错误，请重新填写");
        cm["104"] = QObject::tr("当前账号被冻结，请联系客服");
        cm["10"] = QObject::tr("账号或密码不正确，请重新填写");
        codeMap[LoginCenter] = cm;
    }
    {
        QMap<QString, QString> cm;
        cm["20004001"] = QObject::tr("操作成功");
        cm["20002023"] = QObject::tr("该套餐为限购套餐，当前已达限购上限");
        codeMap[NodeAction] = cm;
    }
}

/*
    先去s找，再去Default找
*/
QString codeView(QString resp, CodeSection s)
{
    if (codeMap.isEmpty()) {
        setCodeMap();
    }

    QString code = codeS(resp);
    QString ret = codeMap.value(s).value(code);
    if (ret.isEmpty()) {
        ret = codeMap.value(Default).value(code);
        if (ret.isEmpty()) {
            QString msg = codeMsg(resp);
            if (!msg.isEmpty()) {
                ret = msg;
            } else {
                ret = resp;
            }
        }
    }

    qDebug()<< __FUNCTION__ << ret;
    return ret;
}

int codeI(QString resp)
{
    return codeS(resp).toInt();
}

QString codeS(QString resp)
{
    QString code;
    QJsonObject obj = JsonUtil::jsonStrToObj(resp);
    if (!obj.isEmpty()) {
        QJsonObject data = obj.value("data").toObject();
        code = data.value("resultCode").toString();
        if (code.isEmpty())
            code = QString::number(data.value("code").toInt());
    }
    return code;
}

QString codeMsg(QString resp)
{
    QString msg;
    QJsonObject obj = JsonUtil::jsonStrToObj(resp);
    if (!obj.isEmpty()) {
        QJsonObject data = obj.value("data").toObject();
        msg = data.value("message").toString();
        if (msg.isEmpty())
            msg = data.value("resultMsg").toString();
    }
    return msg;
}
