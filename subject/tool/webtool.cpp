#include "webtool.h"

#include <QPropertyAnimation>
#include "tool/xfunc.h"
#include "common/session.h"
#include "config/userinfo.h"
#include "view/preview.h"
#include "common/trayicon.h"
#include <QFileDialog>
#include <QClipboard>
#include "db/userdao.h"
#include "tool/jsonutil.h"
#include "tool/network.h"
#include "view/set/renderset.h"
#include "view/set/set.h"
#include "../windows/noticewidget.h"

WebTool::WebTool(QObject *parent) :
    QObject(parent)
{
}

int WebTool::appLang()
{
    int lang = USERINFO->appLangNum();
    qD __FUNCTION__ << lang;
    return lang;
}

QString WebTool::accessToken()
{
    QString ret = USERINFO->accessToken();
    qD __FUNCTION__ << ret;
    return ret;
}

void WebTool::openLocalDir(QString relativePath)
{
    qD __FUNCTION__ << relativePath;
    QString dirDb = DDAO->ResultDir(relativePath);
    if (!dirDb.isEmpty()) {
        QDir dir(dirDb);
        if (dir.exists()) {
            XFunc::openDir(dirDb);
            return;
        } else {
            QString dirPath = RenderSet::cacheDir() + "\\" + relativePath;
            QString info = QString(tr("(%1)路径不存在,已帮您下载至(%2),是否打开?")).arg(dirDb).arg(dirPath);
            if(MsgTool::msgChooseLoop(info, Session::instance()->mainWid()) == MsgBox::msgOk) {
                QDir dir(dirPath);
                if (!dir.exists())
                    dir.mkpath(dirPath);
                XFunc::openDir(dirPath);
            } else {
                return;
            }
        }
    }

    QString dirPath = RenderSet::cacheDir() + "\\" + relativePath;
    QDir dir(dirPath);
    if (!dir.exists())
        dir.mkpath(dirPath);
    XFunc::openDir(dirPath);
}

void WebTool::preview(QString json)
{
    qD __FUNCTION__<<json;
    QJsonObject obj = JsonUtil::jsonStrToObj(json);
    if (obj.isEmpty())
        return;

    QPointer<MainWindow> pm = Session::instance()->mainWid();
    QPointer<BaseWidget> pv = new Preview(pm);
    Preview* pr = (Preview*)pv.data();
    pr->reqView(obj.value("id").toVariant().toInt(), obj.value("target").toString());
}

void WebTool::clipboard(QString text)
{
    qD __FUNCTION__ << text;
    QClipboard* clip = qApp->clipboard();
    clip->setText(text);

    showMessageSuccess(tr("复制成功"));
}

void WebTool::shareResult(QString json)
{
    qD __FUNCTION__ << json;
    QJsonObject obj = JsonUtil::jsonStrToObj(json);
    if (obj.isEmpty())
        return;

    QString para = "url=xrurl&title=xrtitle&summary=xrsummary&pics=xrpics&site=xrsite&style=201&width=32&height=32";

    para.replace("xrurl", XFunc::encodeURI(obj["url"].toString()));//原图链接
    para.replace("xrtitle", obj["title"].toString());//标题
    para.replace("xrsite", obj["title"].toString());//分享来源
    para.replace("xrpics", XFunc::encodeURI(obj["pics"].toString()));//缩略图链接
    para.replace("xrsummary", obj["desc"].toString());//摘要

    QString shareqq = "http://connect.qq.com/widget/shareqq/index.html?";
    qD shareqq + para;
    QDesktopServices::openUrl(QUrl(shareqq + para));
}

void WebTool::downloadResult(QString jsonArr)
{
    qD __FUNCTION__ << jsonArr;
    QJsonArray arr = JsonUtil::jsonStrToArr(jsonArr);
    for (int i = 0; i < arr.size(); ++i) {
        QINVOKE(Session::instance()->m_downHand, "missionResult",
                Q_ARG(int, arr.at(i).toInt()),
                Q_ARG(bool, false),
                Q_ARG(bool, arr.size() > 1));
    }
}

void WebTool::downloadLog(QString log)
{
    qD __FUNCTION__ << log;
    QJsonObject obj = JsonUtil::jsonStrToObj(log);
    if (obj.isEmpty())
        return;

    QString fn = obj["fileName"].toString();
    QString save = QFileDialog::getSaveFileName(Session::instance()->mainWid(), tr("保存"), RenderSet::cacheDir() + "\\" + fn, tr("LOG 文件(*.log)"));
    if (!save.isEmpty()) {
        NetDown* nd = new NetDown(this);
        QNetworkReply* reply = nd->get(obj["url"].toString(), save);
        connect(reply, &QNetworkReply::finished, this, [=]{
            QProcess process;
            process.startDetached("explorer", QStringList() << QString("/select,") << QString("%1").arg(QDir::toNativeSeparators(save)));
        });
    }
}

void WebTool::contactCustomerSea()
{
    QString name = USERINFO->userName().toUtf8().toPercentEncoding();
    QString url = QString("https://xrender.sobot.com/chat/pc/v2/index.html?sysnum=89500c5a3af540f0a3cce6f19591b570&uname=%1&locale=en").arg(name);
    QDesktopServices::openUrl(QUrl(url));
}

void WebTool::contactCustomer()
{
    // QString url = "https://ykf-webchat.yuntongxun.com/wapchat.html?accessId=19b2cb70-deef-11eb-a98f-999e7122859f&fromUrl=http://www.xrender.com/&urlTitle=CGMAGIC客户端&language=ZHCN&clientId=%1&otherParams={\"nickName\":\"%2\"}";
    QString url = "https://support.zanqicloud.com/cgmagic?clientid=%1&nickname=%2";
    QDesktopServices::openUrl(QUrl(url.arg(USERINFO->userId()).arg(USERINFO->userName())));
}

void WebTool::reLogin()
{
}

void WebTool::reloadErrorPage()
{
}

void WebTool::adCount(int id)
{
    qD __FUNCTION__ << id;
    NET->xrget(QString("/bs/user/advertisement?id=%1").arg(id), [=](FuncBody f) {

    }, this);
}

//此方法目前只给网页出现登录页时调用
QString WebTool::loadUserInfos()
{
    return "";
}

void WebTool::openUrl(QString url)
{
    qD __FUNCTION__ << url;
    QDesktopServices::openUrl(QUrl(url));
}

//任务管理取消任务时调客户端删除上传
void WebTool::delUp(QString order)
{
    qD __FUNCTION__ << order;
    QINVOKE(Session::instance()->m_upHand, "delUp",
            Q_ARG(QString, order));
}

QString WebTool::sysMac()
{
    QString mac = XFunc::MAC(true);
    qD __FUNCTION__ << mac;
    return mac;
}

// cg暂无
QString WebTool::permission()
{
    QString p = USERINFO->m_xgtPermission;
    if (p == "") {
        p = "{}";
    }
    qD __FUNCTION__ << p;
    return p;
}

void WebTool::clickLeftNavigation(int id)
{
}

QString WebTool::leftNavigations()
{
    return "";
}

void WebTool::showMessageSuccess(QString msg)
{
    showMessage(0, msg);
}

void WebTool::showMessageWarning(QString msg)
{
    showMessage(1, msg);
}

void WebTool::showMessageInfo(QString msg)
{
    showMessage(2, msg);
}

QWidget* WebTool::showMessageError(QString msg)
{
    return showMessage(3, msg);
}

void WebTool::browserOpen(QString url)
{
    qD __FUNCTION__ << url;
    QDesktopServices::openUrl(QUrl(url));
}

void WebTool::closeWidget()
{
    qD __FUNCTION__;
    emit this->closeWidgetSig();
}

QWidget* WebTool::showMessage(int t, QString msg)
{
    QWidget *mainWid = Session::instance()->CurWid();
    if (!mainWid)
        return mainWid;

    qDebug()<< __FUNCTION__ << t << msg;

    QWidget *wid = new QWidget(mainWid);
    BaseWidget::setClass(wid, "topMessage");
    BaseWidget::setProperty(wid, "type", QString::number(t));
    wid->setAttribute(Qt::WA_DeleteOnClose);
    wid->setFixedSize(qMin(int((double)mainWid->width() * 0.4), 580), 40);
    int x = (mainWid->width() - wid->width()) / 2;
    wid->move(x, 0);
    wid->show();

    QHBoxLayout *lay = new QHBoxLayout(wid);
    lay->setContentsMargins(QMargins(16, 0, 16, 0));
    lay->setSpacing(8);

    QLabel *lab = new QLabel(wid);
    lab->setObjectName("icon");
    lab->setFixedSize(16, 16);
    lay->addWidget(lab);

    QLabel *text = new QLabel(wid);
    text->setObjectName("text");
    text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    text->setText(msg);
    lay->addWidget(text);

    QPushButton *close = new QPushButton(wid);
    close->setObjectName("close");
    close->setFixedSize(14, 14);
    lay->addWidget(close);
    connect(close, &QPushButton::clicked, wid, &QWidget::close);

    wid->setLayout(lay);

    QTimer::singleShot(3000, wid, &QWidget::close);

    QPropertyAnimation *pa = new QPropertyAnimation(wid, "pos");
    pa->setStartValue(QPoint(x, 0));
    pa->setEndValue(QPoint(x, 30));
    pa->setDuration(200);
    pa->start();

    return wid;
}

void WebTool::refresh_web(){
    qDebug()<<"发送刷新网页的信号！";
    QPointer<MainWindow> pm = Session::instance()->mainWid();
    pm->main_refresh_web();


}

void WebTool::jumpChargePage(){
    QPointer<NoticeWidget> noticeWindow = NoticeWidget::getInstance();
    qDebug()<<"Jump chargePage and memory is"<<noticeWindow;
    noticeWindow->on_closeBtn_clicked();

    QPointer<MainWindow> mainWindow = Session::instance()->mainWid();
    mainWindow->main_refresh_web();

}

//内嵌网页充值成功后，实时刷新插件状态
void WebTool::updatePluginStatusForChargeSuccess(){
    QPointer<MainWindow> mainWindow = Session::instance()->mainWid();
    mainWindow->checkVip();
    qDebug()<<"Update plugin status real timely!";
}

//用户下载Max（云转模型、云转材质）
void WebTool::getUserDownloadMax(QJsonObject downInfos){
    QPointer<MainWindow> mainWindow = Session::instance()->mainWid();
    mainWindow->downloadMax(downInfos);
    qDebug()<<"User download max!";
}

void WebTool::setMainWindowStatus(bool isEnable){
    QPointer<MainWindow> mainWindow = Session::instance()->mainWid();
    mainWindow->setMainWindowEnable(isEnable);
    qDebug()<<"Set MainWindow enable is:"<<isEnable;
}

//跳转个人中心
void WebTool::jumpPersonalUrl(QString personalUrl){
    QPointer<MainWindow> mainWindow = Session::instance()->mainWid();
    mainWindow->jumpPersonWeb(personalUrl);
    qDebug()<<"Jump personal web url is:"<<personalUrl;
}

//跳转自动续费规则
void WebTool::jumpAutoChargeRuleWeb(){
    QPointer<MainWindow> mainWindow = Session::instance()->mainWid();
    mainWindow->jumpChargeRuleWeb();
    qDebug()<<"Jump chargeRule web url is:";
}

