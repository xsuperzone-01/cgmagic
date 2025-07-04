#pragma execution_character_set("utf-8")
#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include <QNetworkAccessManager>
#include <QButtonGroup>
#include <QMap>

#include "../basewidget.h"

namespace Ui {
class Widget;
}

class Widget : public BaseWidget
{
    Q_OBJECT

public:
    explicit Widget(BaseWidget *parent = 0);
    ~Widget();

protected:
    void closeEvent(QCloseEvent *e);

private:
    void uninstall();
    void pro(int p);

    void addShadow(QWidget *w);

    void hideChecked();

    QString name();

    void readCilentExeVersion();  //读取客户端exe的版本号（非读取配置文件）

private slots:
    void on_minBtn_clicked();

    void on_closeBtn_clicked();

    void on_cancel_clicked();

//    void on_next_clicked();

    void finished();

    void on_finish_clicked();

    void on_opinionText_textChanged();

    void postReason();
    void replyFinished(QNetworkReply* reply);

    void getClientVersion();  //获取客户端版本号
    void getSqlFile();    //获取数据库文件

    void getEnvironmentJsonPath();   //从本地Json文件获取Url

    void getUrlFromJson(QString jsonPath); //从Json获取指定地段的网址

public slots:
    void on_next_clicked();

public:
    void setAppExit();  //根据是否静默卸载的状态实现程序是否退出exit(0);

    void setSlientUninstallStatus(bool slientUninstall);  //设置静默卸载的状态，true表示静默卸载，false表示非静默卸载

private:
    Ui::Widget *ui;

    bool m_clear;
    QNetworkAccessManager *m_net;

    QMap<int, QString> deleteReasonFromIndex;   //根据索引添加卸载原因
    QString deleteReason; //删除原因
    QString otherTextFromDelete;  //删除的其他意见
    int clientType;   //定义客户端类型(1：官方版；2：渠道版)
    QString clientVersion;  //客户端版本号
    QString userName;   //最后一次登录账号；

    QString url;  //请求地址

    bool isSlientUninstall = false;
};

#endif // WIDGET_H
