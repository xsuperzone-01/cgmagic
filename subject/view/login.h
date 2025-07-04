#ifndef LOGIN_H
#define LOGIN_H

#include "common/basewidget.h"
#include <QJsonObject>
#include "tool/machineprofiles.h"
#include <QPointer>
#include <QNetworkAccessManager>

class ButtonGroup;

namespace Ui {
class Login;
}

class Login : public BaseWidget
{
    Q_OBJECT

public:
    explicit Login(BaseWidget *parent = nullptr);
    ~Login();

    enum LoginCode {
        UserLogin = 0,
        UserReLogin,
        UpdateAutoLogin,
    };

    void initLogin(int relogin = 0, QJsonObject user = QJsonObject()); // 0-登录, 1-重登, 2-更新自动登录
    void setSsoCode(QString code);
    bool isSsoLogin();

    QJsonObject machineInfo();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void assumeParent();
    void btnEnable(bool enable);

    bool IsValidPhoneNumber(const QString &phoneNum);

    bool IsValidEmail(const QString &email);

    int authType();

    void showNameErr(QString err, bool show);
    void showPwdErr(QString err, bool show);

    void showMacErr();
    void showRemoteLoginErr();

    void addShadow(QWidget *w);

    void changeEnShow();

    void areaCode();

    void saveClientVersionToSettings();     //保存客户端版本至配置文件
    QString getUuid(QString type);          //生成UUUID
    QString getComputerVersion();           //获取电脑版本号
    void startLoginBuryPoint(QString url);  //开启埋点
    void getBuryPointUrl(int loginStatus);                 //请求具体的埋点地址
    void getSessionIdAndPvidForDifferentEvent(int loginStatus, QString &sessionId, QString &sessionIdStamp, int &pvid);   //针对登录事件、登陆点击事件生成不同sessionid和pvid
    void delPreviousFiles();    //删除QT5相关文件
    QString getFileVersion(const QString& filePath);
    bool isExistQt6File(const QString &dir);      //仅仅针对Qt5->Qt6版本大跨度升级（仅一次）

public slots:
    void onSingleConnected();


private slots:
    void on_headClose_clicked();

    void on_forgetBtn_clicked();

    void on_loginBtn_clicked();

    void on_registerBtn_clicked();

    void pressedlogin();

    void textChanged(const QString &text);

    void on_autoLoginC_clicked(bool checked);

    void on_remPassC_clicked(bool checked);

    void on_headMin_clicked();

    void on_wx_clicked();

    void on_wxBack_clicked();

    void on_en_link_clicked();

    void focusOutNoText(bool noText);
    bool nameFocusOut();

    void nameError();
    void passwordError();


    void userPress();
    void phonePress();
    void emailPress();
    void loginBuryPointReply(QNetworkReply *reply);

private:
    Ui::Login *ui;

    ButtonGroup *bg;

    QString m_ssoCode;
    bool m_parent;

    QString clientVersion;    //客户端版本
    QPointer<QThread> workThread;
    QPointer<MachineProfiles> machineProfiles;
    QString ipUrl;
    QPointer<QNetworkAccessManager> manager;
    QString ip;
    QString ipcityname;

    QPoint mousePoint;
    bool mousePress;
};
#endif // LOGIN_H
