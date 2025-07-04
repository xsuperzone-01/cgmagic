#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "common/basewidget.h"
#include "common/pullmsg.h"
#include <QJsonArray>
#include "tool/msgtool.h"
#include "transfer/transfer.h"
#include "tool/webtool.h"
#include "set/setting.h"
#include <QNetworkReply>
#include <QWebEngineView>
#include <QUrl>
#include "leftnavitem.h"
#include "common/widgetgroup.h"
#include "config/userinfo.h"
#include "account.h"
#include "../windows/noticewidget.h"
#include "tool/machineprofiles.h"
#include <QPointer>
#include "windows/activitywidget.h"
#include "transferMax/downloadinfos.h"
#include <QQueue>
#include "windows/downloadmaxwidget.h"
#include "transferMax/downloadprocess.h"
class LeftNavItem;
class MsgBox;
class Change;
class WebView;
class Transfer;
class About;
class Set;
class MaxSet;
class RenderSet;
class UpdateSet;
class DefaultTablePage;

namespace Ui {
class MainWindow;
}

class MainWindow : public BaseWidget
{
    Q_OBJECT

public:
    explicit MainWindow(BaseWidget *parent = 0);
    ~MainWindow();

    void initMainWid();

    void trayOpenSet();

    void setLeftNavs();

    void downloadPlugin(QString compressPath, QString policyUrl, QString postfix);

    MsgTool m_msgTool;
    WebTool m_webTool;

    QString money();

    Q_INVOKABLE int downloadConfirm(QString src, QString dest);

    void downloadMax(QJsonObject downloadFileInfos);

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void showEvent(QShowEvent *event);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QFrame *leftNavLine();
    QLabel *leftNavSpace(int height);
    void startThreadPool();

signals:
    void machineCounted(QJsonObject obj);
    void setImageData(QList<QJsonObject>);
    void resizeMainWindow(double ratio);
    void updateDownFileStatusRealTime(DownloadFile::DownloadStatus status, QJsonObject file);

public slots:
    void refreshOrder();
    void NeedUpdate();
    void checkVip(bool showAccount = false);
    void jumpUrlInfo(QString url, int isInAppJump);

private slots:
    void on_headClose_clicked();

    void on_headMax_clicked();

    void changeBalance(const QJsonObject obj);

    void on_mainLogoBtn_clicked();

    void on_customer_clicked();

    void on_help_clicked();
    void showRenderNavs();
    void showSetNavs();

    void on_recharge_clicked();

    void writeFile();
    void replyFinished();
    void on_bannerBtn_clicked();
    void jumpChangeMaxWeb();   //跳转云转模型网站
    void jumpChangeMaterialWeb();   //跳转云转材质网站
    void initEntireLyDownloadWidget(QList<QJsonObject> fileLists, int type);
    void getDownloadStatus(DownloadFile::DownloadStatus status, QJsonObject file);
    void showDownMaxWidget();   //展示Max文件下载列表
    void updateActiveThreads();
    void showFileClearedTips(QString tipContent);//展示结果文件已经过期的结果

private:
    Ui::MainWindow *ui;

    PullMsg m_pullMsg;
    QTimer m_vipTimer;

    QPointer<LeftNavItem> userNav;
    QPointer<LeftNavItem> setNav;
    QPointer<LeftNavItem> m_aboutNav;

    QList<QPointer<LeftNavItem>> m_renderNavs;
    QList<QPointer<LeftNavItem>> m_setNavs;
    QPointer<Change> m_changeMax;
    QPointer<WebView> m_web;
    QPointer<Transfer> m_upload;
    QPointer<Transfer> m_download;
    QPointer<UpdateSet> m_updateSet;
    QPointer<About> m_about;
    QPointer<Set> m_set;
    QPointer<MaxSet> m_maxSet;
    QPointer<RenderSet> m_renderSet;
    QPointer<DefaultTablePage> m_defaultTablePage;

    QPointer<Setting> set;
    QMap<QPointer<QNetworkReply>, QPointer<QFile>> m_replyFile;

private:
    QPointer<WebView> recharge_web;
    QString recharge_url;
    QList<BaseClickWidget *> navClickL;
    widgetGroup *navGroup;
    QPointer<WebView> changeMaxWeb;
    QString changeMaxUrl;
    QPointer<WebView> changeMaterialWeb;
    QString changeMaterialUrl;

private:
    void hideWindow_closeHighLight();
    void delMobaoPluginForBeforeVersion();  //删除老版本mobao插件
    QString readCurrentPluginVersion();     //读取插件当前版本
    void showAccountWidget(QJsonObject userInfo);               //展示用户账户界面
    void getBannerInfo();                                       //获取横幅信息
    void getActivityInfo();                                     //获取活动列表（包含网址、是否应用内跳转、图片信息）
    void getActivityImage(QList<QJsonObject> activityList);     //根据活动列表下载指定图片
    void moveThirdWidget(QPointer<QWidget> ownWidget, QPointer<ActivityWidget> thirdWidget);//获取窗口的位置一般是全局位置
    void showNoticeWidget(QString url);
    void showDownloadWidget();
    void setCloudInfos();
    QString countStoredFile(const QString dirPath, QString filename);

public:
    void main_refresh_web();
    QString getMoneyToolTipContent();   //获取Money的提示内容o'i'net
    int isUserIdVip();                 //用户是否为Vip
    void setMainWindowEnable(bool isEnable);
    void jumpPersonWeb(QString personalUrl);
    void jumpChargeRuleWeb();

public:
    int isUserVip = 0;  //初始化为0，1表示授权、2表示无授权
    QString freeTime;
    QStringList vipModelList;
    QStringList nonVipModelList;
    QPointer<NoticeWidget> noticeWindow;

private:
    QPointer<MachineProfiles> machineProfiles;
    QPointer<QThread> workThread;
    QPointer<Account> account;

    QPointer<ActivityWidget> activityWidget;
    QJsonObject bannerObj;                  //存储横幅内容信息
    QList<QJsonObject> activityObj;         //活动列表
    QList<QJsonObject> imageInfoListObj;    //图片显示需要的信息
    QTimer hoverMoveTimer;                  //弹窗悬浮移动交互定时器
    QTimer activityTimer;                   //活动列表的定时刷新

    bool isMouseInTitleBar = false;
    QPoint reltvPos;

    QList<QPointer<DownloadProcess>> downloadProcessLists;
    QPointer<DownloadMaxWidget> downloadMaxWidget;
    QPointer<QThread> downloadMaxThread;    //下载云转模型、云转材质子线程
    QPointer<DownloadInfos> downloadFile;   //下载文件的信息
    QString downloadFileUrl;
    QQueue<QJsonObject> downloadQueue;      //Max下载队列
    QJsonObject tmp;
    qint64 activeThreads;   //工作线程数
    qint64 maxThreads;  //线程池最大线程数

    qint64 freeTimes;
    qint64 nonFreeTimes;
};
#endif // MAINWINDOW_H
