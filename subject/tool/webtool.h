#ifndef WEBTOOL_H
#define WEBTOOL_H

#include <QObject>
#include "common/basewidget.h"
#include "transferMax/downloadmaxset.h"

class WebTool : public QObject
{
    Q_OBJECT
public:
    explicit WebTool(QObject *parent = 0);

    Q_INVOKABLE int appLang();
    Q_INVOKABLE QString accessToken();
    Q_INVOKABLE void openLocalDir(QString relativePath);
    Q_INVOKABLE void preview(QString json);
    Q_INVOKABLE void clipboard(QString text);
    Q_INVOKABLE void shareResult(QString json);
    Q_INVOKABLE void downloadResult(QString jsonArr);
    Q_INVOKABLE void downloadLog(QString log);
    Q_INVOKABLE void contactCustomer();
    Q_INVOKABLE void contactCustomerSea();
    Q_INVOKABLE void reLogin();
    Q_INVOKABLE void reloadErrorPage();
    Q_INVOKABLE void adCount(int id);
    Q_INVOKABLE QString loadUserInfos();
    Q_INVOKABLE void openUrl(QString url);

    Q_INVOKABLE void delUp(QString order);
    Q_INVOKABLE QString sysMac();

    Q_INVOKABLE QString permission();

    Q_INVOKABLE void clickLeftNavigation(int id);
    Q_INVOKABLE QString leftNavigations();

    Q_INVOKABLE void showMessageSuccess(QString msg);
    Q_INVOKABLE void showMessageWarning(QString msg);
    Q_INVOKABLE void showMessageInfo(QString msg);
    Q_INVOKABLE QWidget* showMessageError(QString msg);

    Q_INVOKABLE void refresh_web();
    Q_INVOKABLE void jumpChargePage();
    Q_INVOKABLE void updatePluginStatusForChargeSuccess();
    Q_INVOKABLE void jumpPersonalUrl(QString personalUrl);

    Q_INVOKABLE void getUserDownloadMax(QJsonObject downInfos);
    Q_INVOKABLE void setMainWindowStatus(bool isEnable);
    Q_INVOKABLE void jumpAutoChargeRuleWeb();

public slots:
    void browserOpen(QString url);

    void closeWidget();
Q_SIGNALS:
    void closeWidgetSig();
    void send_flash_web();

//signals:
//    void send_flash_web();

private:
    QWidget* showMessage(int t, QString msg);
};

#endif // WEBTOOL_H
