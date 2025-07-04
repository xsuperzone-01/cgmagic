#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <config/userinfo.h>
#include <common/session.h>
#include <QTranslator>
#include <QRadioButton>
#include <QListView>
#include <QStandardItemModel>
#include "common/trayicon.h"
#include "tool/childprocess.h"
#include "login.h"
#include <QDesktopServices>
#include <QStackedLayout>
#include <QWebEngineView>
#include "version.h"
#include "view/login.h"
#include "common/protocol.h"
#include "tool/network.h"
#include "tool/xfunc.h"
#include "view/account.h"
#include "tool/msgtool.h"
#include "tool/jsonutil.h"
#include "view/firstrun.h"
#include "db/userdao.h"
#include "leftnavitem.h"
#include "common/eventfilter.h"
#include "common/widgetgroup.h"
#include "changeMax/change.h"
#include "changeMax/defaulttablepage.h"
#include "tool/webview.h"
#include "transfer/transfer.h"
#include "view/set/set.h"
#include "view/set/maxset.h"
#include "view/set/renderset.h"
#include "io/pluginlisten.h"
#include "transfer/sessiontimer.h"
#include "db/uploaddao.h"
#include "view/about.h"
#include "view/set/updateset.h"
#include "view/set/trayset.h"
#include "plugin/plugincorrespond.h"
#include <cmath>
#include <QStandardPaths>
#include <QThreadPool>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

MainWindow::MainWindow(BaseWidget *parent)
    : BaseWidget(parent)
    , ui(new Ui::MainWindow),
    activeThreads(0),
    maxThreads(3),
    freeTimes(0),
    nonFreeTimes(0)
{
    ui->setupUi(this);
    {
        selfAdaptionSize();
        selfAdaptionMargins();
        resizeChildrenMargin();

        setMacLayout(ui->headMin);
        setMacLayout(ui->headMax);
        setMacLayout(ui->headClose);
    }

    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    m_autoMax = true;
    connect(this, &BaseWidget::autoMaxChanged, this, &MainWindow::on_headMax_clicked);

    setWindowTitle(qApp->applicationDisplayName());
    QString logo = ":/logo.png";
    ui->mainLogoBtn->setStyleSheet(QString("QPushButton {border-image:url(%1);}").arg(logo));
    ui->mainLogoBtn->setToolTip(CLIENT_VERSION);
    ui->customer->setToolTip(tr("联系客服"));
    ui->help->setToolTip(tr("帮助中心"));
    ui->headset->setToolTip(tr("设置"));
    ui->headMin->setToolTip(tr("最小化"));
    ui->headMax->setToolTip(tr("最大化"));
    ui->headClose->setToolTip(tr("关闭"));

    ui->updatePoint->hide();
    ui->otherWid->show();
    ui->head->show();
    ui->setMax->hide();

    QString lan = Set::changeLan();
    if(lan == "en_us"){
      ui->customer->hide();
      ui->setMax->setDisabled(true);
      ui->setRen->setDisabled(true);
    }

    NeedUpdate();

    connect(ui->headset, &QPushButton::clicked, this, [&](){
        ui->headset->setEnabled(false);
        if(set)
            set->deleteLater();
        set = new Setting(this);
        connect(set, &Setting::needed, this, [=](){
            ui->updatePoint->show();
        });
        connect(set, &Setting::neednot, this, [=](){
            ui->updatePoint->hide();
        });
        set->NeedUpdate();
        set->setSetting();
        ui->headset->setEnabled(true);
    });

    connect(ui->setMax, &QPushButton::clicked, this, [=](){
        if(set)
            set->deleteLater();
        set = new Setting(this);
        set->setMax();
        connect(set->maxSet(), &MaxSet::updateSet, m_changeMax, &Change::updateSet);
    });

    connect(ui->setRen, &QPushButton::clicked, this, [=](){
        if(set)
            set->deleteLater();
        set = new Setting(this);
        set->setRender();
    });

    setClass(ui->headMax, "max");
    setClass(ui->headMin, "min");
    setClass(ui->headClose, "close");
    ui->headMax->setProperty("maximize", "maxmax");
    ui->headMax->setStyle(qApp->style());

    qApp->setQuitOnLastWindowClosed(false);
    openHotKey("", 0);

    setMinBtn(ui->headMin);

    TrayIcon::instance()->setCurWid(this);
    TrayIcon::instance()->setIconOnline("");

    connect(&m_pullMsg, SIGNAL(changeBalance(const QJsonObject)), this, SLOT(changeBalance(const QJsonObject)));
    m_pullMsg.pullTimer();

    setBlurEffectWidget(ui->mainwidget);
    setProperty(this, "shadowBackground", "mainShadowBackground");
    setProperty(this, "shadowRadius", 12);

    setMinimumSize(1056, 752);

    setTitleBar(ui->headWidget);
    setResizeable(true);
    setResizeableAreaWidth(10);

    //获取机器码存在延迟，因此使用多线程操作
    workThread = new QThread;
    machineProfiles = new MachineProfiles;
    machineProfiles->moveToThread(workThread);

    connect(workThread, &QThread::started, machineProfiles, &MachineProfiles::getMachineCode);
    connect(workThread, &QThread::finished, machineProfiles, &MachineProfiles::deleteLater);
    connect(machineProfiles, &MachineProfiles::destroyed, workThread, &QThread::deleteLater);
    workThread->start();

    setCloudInfos();
    account = new Account(this);
    connect(account, &Account::send_renewal, this, &MainWindow::on_recharge_clicked);
    connect(account, &Account::send_recharge, this, &MainWindow::on_recharge_clicked);
    connect(account, &Account::closeWidget, this, [=](){
        this->canMove = true;
    });

    ui->presentLab->installEventFilter(this);
    ui->head->installEventFilter(this);
    activityWidget = new ActivityWidget(this);
    activityWidget->installEventFilter(this);
    account->installEventFilter(this);
    connect(this, &MainWindow::setImageData, activityWidget, &ActivityWidget::setImageData);
    connect(activityWidget, &ActivityWidget::jumpUrlInfo, this, &MainWindow::jumpUrlInfo);
    connect(&hoverMoveTimer, &QTimer::timeout, this, [=](){
        activityWidget->hide();
        account->hide();
    });
    hoverMoveTimer.setInterval(1000);

    ui->bannerWid->hide();
    delMobaoPluginForBeforeVersion();
    getBannerInfo();
    getActivityInfo();

    activityTimer.setInterval(2*60*60*1000);
    connect(&activityTimer, &QTimer::timeout, this, [=](){
        getBannerInfo();
        getActivityInfo();
    });
    activityTimer.start();
    qDebug()<<"成功进入主界面，程序开始操作！！！";


    connect(qApp, &QApplication::aboutToQuit, this, [=](){
        for(auto downloadProcess : downloadProcessLists){
            if(downloadProcess){
                downloadProcess->stopProcess();
            }else{}
        }
    });
    downloadMaxWidget = new DownloadMaxWidget(this);
    QThreadPool::globalInstance()->setMaxThreadCount(maxThreads);  //设置最大线程数为3
    showDownloadWidget();
}

MainWindow::~MainWindow()
{
    qDebug() << __FUNCTION__;
    delete ui;

    if(downloadMaxThread){
        downloadMaxThread->deleteLater();
        downloadMaxThread = nullptr;
    }

    if(!downloadProcessLists.isEmpty()){
        for(auto downloadProcess : downloadProcessLists){
            if(!downloadProcess){
                delete downloadProcess;
                downloadProcess = nullptr;
            }else{}
        }
    }else{
        qDebug()<<"Not active threadPool!";
    }

    QThreadPool::globalInstance()->deleteLater();
}

void MainWindow::setCloudInfos(){
    NET->get(cloudInfos, [=](FuncBody f){
        QJsonObject resultData = f.j.value("data").toObject().value("data").toObject();
        if (!resultData.isEmpty()) {
            freeTimes = resultData.value("total").toInt() - resultData.value("used").toInt();
            nonFreeTimes = resultData.value("payTotal").toInt() - resultData.value("payUsed").toInt();
        } else {
            qDebug() << "查询余额失败!!!" << f.j;
        }
    }, this);
}

//初始化下载界面UI
void MainWindow::showDownloadWidget(){
    downloadMaxThread = new QThread;
    downloadFile = new DownloadInfos;
    downloadFile->moveToThread(downloadMaxThread);
    connect(downloadMaxThread, &QThread::started, downloadFile, &DownloadInfos::readFiles, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
    connect(downloadFile, &DownloadInfos::initEntirelyDownloadWidget, this, &MainWindow::initEntireLyDownloadWidget, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
    connect(this, &MainWindow::updateDownFileStatusRealTime, downloadFile, &DownloadInfos::downFileStatusRealTime, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
    downloadMaxThread->start();
}

QString MainWindow::countStoredFile(const QString dirPath, QString filename) {
    QDir dir(dirPath);
    if(!dir.exists()){
        dir.mkpath(dirPath);
        qDebug()<<"Directory does not exist and create dir:"<<dirPath;
    }else{
        qDebug()<<"Dir is exist!";
    }

    QString baseName = filename.section('.', 0, 0);
    QString extension = filename.section('.', 1, 1);

    QStringList files = dir.entryList(QDir::Files);
    QRegularExpression regex(QString("^%1\\((\\d+)\\)\\.%2$").arg(QRegularExpression::escape(baseName), QRegularExpression::escape(extension)));

    QSet<int> existingIndexes;
    for(const QString& fileName : files){
        QRegularExpressionMatch match = regex.match(fileName);
        if(match.hasMatch()){
            int index = match.captured(1).toInt();
            existingIndexes.insert(index);
        }else{}
    }

    int nextIndex = 1;
    while(existingIndexes.contains(nextIndex)){
        nextIndex++;
    }

    return QString("%1(%2).%3").arg(baseName).arg(nextIndex).arg(extension);
}

//前端用户点击下载任务
void MainWindow::downloadMax(QJsonObject downloadFileInfos){
    qDebug()<<"downloadFileInfos is:"<<downloadFileInfos;
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    downloadFileInfos.insert("clickStartTime", currentTime);
    downloadFileInfos.insert("startTime", currentTime);
    downloadFileInfos.insert("endTime", QString(""));
    downloadFileInfos.insert("userId", UserInfo::instance()->userId());
    downloadFileInfos.insert("userName", UserInfo::instance()->userName());

    QString filename = downloadFileInfos.value("name").toString();
    QString newFilename = countStoredFile(UserInfo::instance()->readAllIni(QString("Set"), QString("cacheDir")).toString() + QString("\\%1").arg(QString("ConvertDownloads")), filename);
    downloadFileInfos.insert("name", newFilename);
    downloadQueue.enqueue(downloadFileInfos);
    startThreadPool();

    //不得已而为之写法!
    for(int i = 0; i < navClickL.size(); i++){
        if(i == 2){
            BaseClickWidget *downloadMaxClickedWidget = navClickL.at(i);
            emit downloadMaxClickedWidget->clicked(downloadMaxClickedWidget);
            break;
        }else{}
    }
    showDownMaxWidget();

    qDebug()<<"downloadQueue is:"<<downloadQueue.size();
    if (!downloadQueue.isEmpty()) {
        foreach (auto queueValue, downloadQueue) {
            DownloadFile::DownloadStatus status = DownloadFile::DownloadStatus::Queued;
            getDownloadStatus(status, queueValue);
        }
    } else {
    }
}

//交互中转
void MainWindow::getDownloadStatus(DownloadFile::DownloadStatus status, QJsonObject file){
    emit updateDownFileStatusRealTime(status, file);
}

//启动线程池
void MainWindow::startThreadPool(){
    for(int i = 0; i < maxThreads; i++){
        if(activeThreads <maxThreads){
            if(!downloadQueue.isEmpty()){
                activeThreads++;
                DownloadProcess* downloadFileProcess = new DownloadProcess;
                QJsonObject taskObj = downloadQueue.dequeue();
                taskObj.insert("storePath", UserInfo::instance()->readAllIni(QString("Set"), QString("cacheDir")).toString() + QString("\\%1").arg(QString("ConvertDownloads")));
                downloadFileProcess->setDownloadTask(taskObj);
                downloadProcessLists.append(downloadFileProcess);
                connect(downloadFileProcess, &DownloadProcess::getDownloadUrlSuccess, this, [=](){
                    QThreadPool::globalInstance()->start(downloadFileProcess);
                });
                connect(downloadFileProcess, &DownloadProcess::downloadStatus, this, &MainWindow::getDownloadStatus, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
                connect(downloadFileProcess, &DownloadProcess::downloadProgressEnd, this, &MainWindow::updateActiveThreads, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
                connect(downloadFileProcess, &DownloadProcess::showFileCleared, this, &MainWindow::showFileClearedTips, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
            }else{
                break;
            }
        }else{
            break;
        }
    }
}

void MainWindow::showFileClearedTips(QString tipContent){
    MsgBox *mb = MsgTool::msgChoose(tipContent, this);
    mb->setTitle(tr("提示"));
    mb->hideOkBtn();
    mb->setBackgroundMask();

    connect(mb, &MsgBox::closed, this, [=](){
        updateActiveThreads();
    });
}

void MainWindow::updateActiveThreads(){
    activeThreads--;
    qDebug()<<"activeThreads is:"<<activeThreads;
    startThreadPool();
}

void MainWindow::initEntireLyDownloadWidget(QList<QJsonObject> fileLists, int type){
    bool isInitReadDb = false;
    if(type == 1){
        for (int i = 0; i < fileLists.size(); i++) {
            QJsonObject obj = fileLists.at(i);
            int status = obj.value("downStatus").toInt();
            if (status == static_cast<int>(DownloadFile::DownloadStatus::Queued)) {
                bool isDelete = true;
                obj.insert("isDelete", isDelete);
                downloadQueue.enqueue(obj);
                obj.insert("isInitRead", isInitReadDb);
            }else{}
        }
    }else{
    }
    startThreadPool();

    if(!downloadMaxWidget){
        qDebug()<<"downloadMaxWidget is nullptr!";
    }else{
        downloadMaxWidget->setDownloadFiles(fileLists);
    }
}

void MainWindow::getBannerInfo(){
    NET->get(bannerActivityUrl, [=](FuncBody f){
        int code = f.j.value("code").toInt();
        if(code == 200){
            QJsonObject resultData = f.j.value("data").toObject().value("data").toObject();
            if(resultData.isEmpty()){
                ui->bannerWid->hide();
            }else{
                qDebug()<<"bannerActivity is:"<<resultData;
                QString content = resultData.value("content").toString();
                bannerObj.insert(QString("inAppJump"), resultData.value("inAppJump").toInt());
                bannerObj.insert(QString("domain"), resultData.value("domain").toString());
                if(!content.isEmpty()){
                    ui->bannerBtn->setText(content);
                    ui->bannerWid->show();
                }else{
                    ui->bannerWid->hide();
                }
            }
        }else{
            ui->bannerWid->hide();
        }

    }, this);
}

void MainWindow::getActivityInfo(){
    activityObj.clear();
    NET->get(presentActivityUrl, [=](FuncBody f){
        int code = f.j.value("code").toInt();
        if(code == 200){
            QJsonArray resultData = f.j.value("data").toObject().value("data").toArray();
            if(resultData.isEmpty()){
                ui->presentLab->hide();
                return;
            }else{
                qDebug()<<"presentActivity is:"<<resultData;
                for(auto it : resultData){
                    QJsonObject detail = it.toObject();
                    activityObj.append(detail);
                }
            }
        }else{
            ui->presentLab->hide();
            qDebug()<<"Not is activity!";
            return;
        }
        getActivityImage(activityObj);
    }, this);
}

void MainWindow::getActivityImage(QList<QJsonObject> activityList){
    imageInfoListObj.clear();
    int urlNum = activityList.size();
    for(int i = 0; i<urlNum; i++){
        QEventLoop loop;
        QTimer::singleShot(500, &loop, &QEventLoop::quit);
        QString imageDownloadUrl = activityList.at(i).value("url").toString();
        QString domain = activityList.at(i).value("domain").toString();
        int inAppJump = activityList.at(i).value("inAppJump").toInt();
        NET->get(imageDownloadUrl, [=](FuncBody f){
            QByteArray imageData = f.b;
            if(imageData.isEmpty()){
                qDebug()<<"Image data is empty!";
            }else{
                QJsonObject imageInfo;
                imageInfo.insert("domain", domain);
                imageInfo.insert("inAppJump", inAppJump);
                imageInfo.insert("imageByte", QString::fromLatin1(imageData.toBase64()));
                imageInfoListObj.append(imageInfo);
            }
        }, this);
        loop.exec();
    }
    emit setImageData(imageInfoListObj);
}

void MainWindow::jumpUrlInfo(QString url, int isInAppJump){
    QString jumpUrl;
    int clientType = 0;
    QString code = UserInfo::instance()->reqSsoCode();
    if(url.contains("https://www.xrender.com") || url.contains("http://dev-xmax.cudatec.com")){
        if(isInAppJump == 0){
            clientType = 1;
            jumpUrl = QString("%1?code=%2&loginType=1&clientType=%3").arg(url).arg(code).arg(clientType);
        }else{
            clientType = 2;
            jumpUrl = QString("%1?code=%2&loginType=1&clientType=%3").arg(url).arg(code).arg(clientType);
        }
    }else{
        qDebug()<<"Other url is:"<<url;
        jumpUrl = url;
    }
    qDebug()<<"jumpUrl is:"<<jumpUrl;

    if(isInAppJump == 0){
        QDesktopServices::openUrl(QUrl(url));
    }else{
        if(noticeWindow){
            return;
        }else{
            showNoticeWidget(url);
        }
    }
}

void MainWindow::delMobaoPluginForBeforeVersion(){
    QString roamingPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    qDebug() << "Roaming path:" << roamingPath;
    if(roamingPath.isEmpty()){
        return;
    }else{
        int index = roamingPath.indexOf("/cgmagic");
        QString pluginTempPath = roamingPath.mid(0, index) + "/Xcgmagic/AllUser/mobao";
        QDir dir(pluginTempPath);
        QStringList dirList = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        if(dirList.isEmpty()){
            return;
        }else{
            QString pluginVersion = readCurrentPluginVersion();
            for(const QString &subdirVersion : dirList){
                if(subdirVersion != pluginVersion){
                    //删除不等于当前版本的插件子目录
                    QDir subdir(dir.absoluteFilePath(subdirVersion));
                    if (subdir.removeRecursively()) {
                        qDebug()<<"Delete directory successfully:"<<subdir.absolutePath();
                    } else {
                        qDebug()<<"Delete directory failed:"<<subdir.absolutePath();
                    }
                }else{
                    qDebug()<<"Currest plugin version equal latest plugin version!";
                }
            }
        }
    }
}

QString MainWindow::readCurrentPluginVersion(){
    QString versionFilePath = QString(qApp->applicationDirPath() + "/mobao/version.txt");
    QFile file(versionFilePath);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)){
        qDebug()<<"Read file failure"<<file.errorString();
    }
    QByteArray pluginVersionByte = file.readAll();
    QString pluginVersion = QString(pluginVersionByte);
    file.close();

    return pluginVersion;
}

void MainWindow::NeedUpdate()
{
    int c = 0;
    int p = 0;
    qDebug()<<"检测是否有更新";
    QPointer<VersionManager> vm = VersionManager::instance();
    vm->check();
    //客户端更新检测
    QJsonObject obj = vm->client();
    QString ncv = obj.value("version").toString();//版本号
    if (CLIENT_VERSION == "" || ncv != CLIENT_VERSION) {
        c = 1;
    }
    //插件更新检测
    QString ver = QString(qApp->applicationDirPath() + "/mobao/version.txt");
    QFile verison(ver);
    if(!verison.open(QIODevice::ReadOnly|QIODevice::Text)){
        qDebug()<<"/mobao/version.txt打开失败";
    }
    QString v = verison.read(10);
    verison.close();
    foreach (QJsonObject plg, vm->plugins) {
        QString npv = plg.value("version").toString();//版本号
        qDebug()<<"版本号为"<<npv;
        QString cpv = v;
        qDebug()<<"当前版本为"<<cpv;
        if ( cpv == "" || npv != cpv) {
            p++;
        }
    }
    qDebug()<<__FUNCTION__<<c<<p;
    if (c + p != 0) {
        qDebug()<<"显示更新提示"<<true;
        ui->updatePoint->show();
    }else {
        ui->updatePoint->hide();
    }
    QTimer::singleShot(60000, this, &MainWindow::NeedUpdate);
}

void MainWindow::downloadPlugin(QString compressPath, QString policyUrl, QString postfix)
{
    QNetworkAccessManager *man = new QNetworkAccessManager(this);
    QFile *wf = new QFile(compressPath, this);
//"41_cgmagic渲染插件/1.0.1/readme.txt" //"41_cgmagic渲染插件/1.0.1/plugin.zip"
    bool ok = wf->open(QFile::WriteOnly|QFile::Truncate);
    qDebug()<<ok<<wf->errorString();
    QNetworkRequest req(QUrl(QString(policyUrl).replace("policy.json", postfix)));//将policy.json替换为name
    QNetworkReply *reply = man->get(req);
    m_replyFile.insert(reply, wf);//插入
    connect(reply, &QNetworkReply::readyRead, this, &MainWindow::writeFile);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::replyFinished);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

}

void MainWindow::writeFile()
{
    if (QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender())) {
        QFile *wf = m_replyFile.value(reply);
        if (wf) {
            qint64 wn = wf->write(reply->readAll());
        }
    }
}

void MainWindow::replyFinished()
{
    if (QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender())) {
        QFile *wf = m_replyFile.value(reply);
        if (wf) {
            wf->close();
            wf->deleteLater();
            QString fn = wf->fileName();
            qDebug()<< __FUNCTION__<<fn;          //UpdateContent::replyFinished "41_cgmagic渲染插件/1.0.1/plugin.zip"
            }
        m_replyFile.remove(reply);
    }
}


void MainWindow::initMainWid()
{
    TrayIcon::instance()->setCurWid(this);

    if (Set::loginHide() || Set::loginSilent())
        this->hide();
    else
        this->show();

    setLeftNavs();

    UPDAO->RallFile();
    DDAO->RallFile();
    SessionTimer::startTimer();
    PluginListen::savePlugins();
    PluginListen::saveCGs();

    checkVip();
    connect(&m_vipTimer, &QTimer::timeout, this, [=]{
        checkVip();
    });

    QString lan = Set::changeLan();
    if(lan == "zh_cn"){
        if (UserInfo::instance()->isFirstRun()) {
            new FirstRun(this);
        }
    }
}

void MainWindow::on_headClose_clicked()
{
    this->close();
}

void MainWindow::on_headMax_clicked()
{
    headMaxHand(ui->headMax);
}

void MainWindow::trayOpenSet()
{
    if (setNav) {
        emit setNav->clicked(setNav);
    }
}

void MainWindow::setLeftNavs()
{
    // 优化点击再加载导致的刷新慢
    if (!m_web) {
        m_web = new WebView(this);
        m_web->setCacheDir(USERINFO->appDataPath() + "/web");
        ui->web->layout()->addWidget(m_web);
        m_web->load(QUrl(USERINFO->bs() + "/bs/view/cg-task-list.html"));
    }

    QStringList leftNavL;
    leftNavL << tr("云转模型") <<tr("云转材质") << tr("下载列表") << tr("任务管理") << tr("上传列表") << tr("下载列表") << tr("商城");
    QVBoxLayout *navLayout = (QVBoxLayout *)ui->leftNav->layout();
    navLayout->setContentsMargins(24, 0, 0, 12);
    BaseClickWidget *currentNav = NULL;

    int length = leftNavL.size() +3; //左侧选项总数
    for (int i = 0; i < length; i++) {
        switch (i) {
        case 0:{
            ui->titleMax->setFixedHeight(35);
            navLayout->insertWidget(0 , ui->titleMax);
            break;
        }
        case 1:{
            LeftNavItem *item = new LeftNavItem(ui->leftNav);
            currentNav = item;
            item->setFixedHeight(36);
            navLayout->insertWidget(1 , item);
            item->initLeftNavItem(leftNavL.at(i-1), i);
            // connect(item, &LeftNavItem::clicked, this, [=]{
            //     QString lan = Set::changeLan();
            //     if(lan == "en_us"){
            //         ui->stackedWidget->setCurrentIndex(9);
            //         if(!m_defaultTablePage){
            //              m_defaultTablePage = new DefaultTablePage(this);
            //              ui->defalut->layout()->addWidget(m_defaultTablePage);
            //         }
            //     }else{
            //         ui->stackedWidget->setCurrentIndex(0);
            //         if (!m_changeMax) {
            //             m_changeMax = new Change(this);
            //             connect(this, &MainWindow::resizeMainWindow, m_changeMax, &Change::resizeTableItemWidth, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
            //             ui->changeMax->layout()->addWidget(m_changeMax);
            //             m_changeMax->init();
            //         }
            //     }
            // });

            connect(item, &LeftNavItem::clicked, this, &MainWindow::jumpChangeMaxWeb, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
            navClickL << item;
            break;
        }
        case 2:{
            LeftNavItem *item = new LeftNavItem(ui->leftNav);
            item->setFixedHeight(36);
            navLayout->insertWidget(i , item);
            item->initLeftNavItem(leftNavL.at(i-1), i);
            connect(item, &LeftNavItem::clicked, this, &MainWindow::jumpChangeMaterialWeb, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
            m_renderNavs << item;
            navClickL << item; //从上至下一次执行添加
            break;
        }
        case 3:{
            LeftNavItem *item = new LeftNavItem(ui->leftNav);
            item->setFixedHeight(36);
            navLayout->insertWidget(i , item);
            item->initLeftNavItem(leftNavL.at(i-1), i);
            connect(item, &LeftNavItem::clicked, this, &MainWindow::showDownMaxWidget, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
            m_renderNavs << item;
            navClickL << item; //从上至下一次执行添加
            break;
        }
        case 4:{
            ui->titleRen->setFixedHeight(30);
            navLayout->insertWidget(i , ui->titleRen);
            break;
        }
        case 5:{
            LeftNavItem *item = new LeftNavItem(ui->leftNav);
            item->setFixedHeight(36);
            navLayout->insertWidget(i , item);
            item->initLeftNavItem(leftNavL.at(i-2), i);
            connect(item, &LeftNavItem::clicked, this, [=]{
                QString lan = Set::changeLan();
                if(lan == "en_us"){
                    ui->stackedWidget->setCurrentWidget(ui->defalut);
                    if(!m_defaultTablePage){
                         m_defaultTablePage = new DefaultTablePage(this);
                         ui->defalut->layout()->addWidget(m_defaultTablePage);
                    }
                }else{
                    ui->stackedWidget->setCurrentWidget(ui->web);
                    if (m_web)
                        m_web->reloadIfError();
                }
            });
            m_renderNavs << item;
            navClickL << item;
            break;
        }
        case 6:{
            LeftNavItem *item = new LeftNavItem(ui->leftNav);
            item->setFixedHeight(36);
            navLayout->insertWidget(i , item);
            item->initLeftNavItem(leftNavL.at(i-2), i);
            connect(item, &LeftNavItem::clicked, this, [=]{
                QString lan = Set::changeLan();
                if(lan == "en_us"){
                    ui->stackedWidget->setCurrentWidget(ui->defalut);
                    if(!m_defaultTablePage){
                         m_defaultTablePage = new DefaultTablePage(this);
                         ui->defalut->layout()->addWidget(m_defaultTablePage);
                    }
                }else{
                    ui->stackedWidget->setCurrentWidget(ui->upload);
                    if (!m_upload) {
                        m_upload = new Transfer(this);
                        connect(this, &MainWindow::resizeMainWindow, m_upload, &Transfer::resizeUpTableItemWidth, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
                        m_upload->hideDelBatchBtn(true);
                        ui->upload->layout()->addWidget(m_upload);
                        m_upload->initUpload();
                    } else {
                        m_upload->hideDelBatchBtn(true);
                        m_upload->refresh();
                    }
                }
            });
            m_renderNavs << item;
            navClickL << item;
            break;
        }
        case 7:{
            LeftNavItem *item = new LeftNavItem(ui->leftNav);
            item->setFixedHeight(36);
            navLayout->insertWidget(i , item);
            item->initLeftNavItem(leftNavL.at(i-2), i);
            connect(item, &LeftNavItem::clicked, this, [=]{
                QString lan = Set::changeLan();
                if(lan == "en_us"){
                    ui->stackedWidget->setCurrentWidget(ui->defalut);
                    if(!m_defaultTablePage){
                         m_defaultTablePage = new DefaultTablePage(this);
                         ui->defalut->layout()->addWidget(m_defaultTablePage);
                    }
                }else{
                    ui->stackedWidget->setCurrentWidget(ui->download);
                    if (!m_download) {
                        m_download = new Transfer(this);
                        connect(this, &MainWindow::resizeMainWindow, m_download, &Transfer::resizeDownTableItemWidth, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
                        m_download->hideDelBatchBtn(false);
                        m_download->initDownload();
                        ui->download->layout()->addWidget(m_download);
                    }
                }
            });
            m_renderNavs << item;
            navClickL << item; //从上至下一次执行添加
            break;
        }
        case 8:{
            ui->titlePerson->setFixedHeight(30);
            navLayout->insertWidget(i , ui->titlePerson);
            break;
        }
        case 9:
            LeftNavItem *item = new LeftNavItem(ui->leftNav);
            item->setFixedHeight(36);
            navLayout->insertWidget(i , item);
            item->initLeftNavItem(leftNavL.at(i-3), i);
            connect(item, &LeftNavItem::clicked, this, &MainWindow::on_recharge_clicked, Qt::ConnectionType(Qt::UniqueConnection | Qt::AutoConnection));
            m_renderNavs << item;
            navClickL << item; //从上至下一次执行添加
            break;
        }
    }
    navGroup = new widgetGroup(this);  //暂时提升至类成员变量，与充值按钮进行公用
    navGroup->addWidgets(navClickL, qApp->style(), currentNav); //navClickL包含4个窗口
}

//需要在主窗口完全初始化完成并显示才有效果--主界面不是在构造函数中show()方法的
void MainWindow::moveThirdWidget(QPointer<QWidget> ownWidget, QPointer<ActivityWidget> thirdWidget){
    QPoint presentPos = ownWidget->mapToGlobal(QPoint(0, 0));
    int realX = presentPos.x() + ownWidget->width()/2 - thirdWidget->width()/2;
    int realY = presentPos.y() + ownWidget->height();
    presentPos.setX(realX);
    presentPos.setY(realY);
    thirdWidget->move(presentPos);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->presentLab && event->type() == QEvent::Enter){
        moveThirdWidget(ui->presentLab, activityWidget);
        activityWidget->show();
        hoverMoveTimer.stop();
    }else if(watched == ui->presentLab && event->type() == QEvent::Leave){
        hoverMoveTimer.start();
    }else if(watched == activityWidget && event->type() == QEvent::Enter){
        hoverMoveTimer.stop();
    }else if(watched == activityWidget && event->type() == QEvent::Leave){
        hoverMoveTimer.start();
    }else if(watched == ui->head && event->type() == QEvent::Enter){
        checkVip(true);
        hoverMoveTimer.stop();
    }else if(watched == ui->head && event->type() == QEvent::Leave){
        hoverMoveTimer.start();
    }else if(watched == account && event->type() == QEvent::Enter){
        hoverMoveTimer.stop();
    }else if(watched == account && event->type() == QEvent::Leave){
        hoverMoveTimer.start();
    }
    return BaseWidget::eventFilter(watched, event);
}

// 解决 Qt界面最小后恢复界面不能刷新，出现假死的情况 ？？？
void MainWindow::showEvent(QShowEvent *event)
{
    setAttribute(Qt::WA_Mapped);
    BaseWidget::showEvent(event);
}

void MainWindow::changeBalance(const QJsonObject obj)
{
    int balance = obj.value("amount").toInt();
    UserInfo::instance()->renderTime = balance;
}

QString MainWindow::getMoneyToolTipContent(){
    int renderTime = UserInfo::instance()->renderTime;
    int minuteForInteger = renderTime / 60.0;
    int secondForModular = renderTime % 60;
    QString tip = QString("%1分钟%2秒").arg(minuteForInteger).arg(qAbs(secondForModular));
    return tip;
}

void MainWindow::on_mainLogoBtn_clicked()
{
    if (m_aboutNav) {
        emit m_aboutNav->clicked(m_aboutNav.data());
    }
}

void MainWindow::refreshOrder()
{
    if (m_web) {
        m_web->page()->runJavaScript("autoRefresh()");
    }
}

QString MainWindow::money()
{
    int renderTime = UserInfo::instance()->renderTime;
    double minutes = renderTime / 60.0; // 将秒转换为分钟并保留一位小数并向上取整
    double upRound = std::ceil(minutes * 10) / 10.0;
    QString result = QString::number(upRound, 'f', 1);
    return result;
}

int MainWindow::downloadConfirm(QString src, QString dest)
{
    MsgBox *mb = MsgTool::msgChoose("", this);
    mb->setTitle(tr("重命名文件"));
    mb->hideOkBtn();
    mb->setBackgroundMask();

    QPushButton *cover = new QPushButton(tr("覆盖"), mb);
    cover->setFixedSize(88, 32);
    BaseWidget::setClass(cover, "noBtn");

    QPushButton *rename = new QPushButton(tr("重命名"), mb);
    rename->setFixedSize(88, 32);
    BaseWidget::setClass(rename, "okBtn");
    BaseWidget::setDropShadow(rename, 0, 7, 15, QColor(19, 125, 211, 102));

    mb->prependButton(rename);
    mb->prependButton(cover);

    mb->setInfo(tr("已有文件存在于本地，请选择"));
    mb->setFixedHeight(240);

    int ret = RenderSet::Ignore;
    QEventLoop loop;
    connect(cover, &QPushButton::clicked, this, [=, &loop, &ret]{
        ret = RenderSet::Cover;
        loop.quit();
    });
    connect(rename, &QPushButton::clicked, this, [=, &loop, &ret]{
        ret = RenderSet::Rename;
        loop.quit();
    });
    connect(mb, &MsgBox::closed, this, [=, &loop, &ret]{
        loop.quit();
    });
    loop.exec();
    return ret;
}

QFrame *MainWindow::leftNavLine()
{
    QFrame *line = new QFrame(ui->leftNav);
    line->setLineWidth(1);
    line->setFrameStyle(QFrame::HLine|QFrame::Plain);
    return line;
}

QLabel *MainWindow::leftNavSpace(int height)
{
    BaseLabel *l = new BaseLabel(ui->leftNav);
    l->setFixedHeight(height);
    return l;
}

void MainWindow::on_customer_clicked()
{
    m_webTool.contactCustomer();
}

void MainWindow::on_help_clicked()
{
    QDesktopServices::openUrl(USERINFO->openHelp());
}

void MainWindow::checkVip(bool showAccount)
{
    NET->get(vipUrl, [=](FuncBody f){
        QJsonObject data = f.j.value("data").toObject();
        int code = data.value("code").toInt();
        if (1 != code) {
            QTimer::singleShot(15000, this, [=]{
                checkVip(showAccount);
            });
            return;
        }

        data = data.value("data").toObject();
        int status = data.value("status").toInt();
        QString endTime = data.value("endTime").toString();
        UserInfo::instance()->userStatusObj = data;

        if(status == 1){
            BaseWidget::setProperty(ui->head, "type", "1");
            ui->head->setFixedHeight(40);
        }else if(status == 2){
            BaseWidget::setProperty(ui->head, "type", "2"); //过期
            ui->head->setFixedHeight(40);
        }else{
            BaseWidget::setProperty(ui->head, "type", "0");  //未开通
            ui->head->setFixedHeight(32);
        }
        qDebug()<<"status is:"<<status;

        if(status == 1){   //1表示vip用户，2表示nonVip用户
            isUserVip = 1;

            NET->get(universalClientUrl, [=](FuncBody f) {
                QJsonObject data = f.j.value("data").toObject();
                if (data.value("code").toInt() == 1) {
                    QString empowerNumber =data.value("data").toObject().value("empowerNumber").toString();
                    QString freeNumber =data.value("data").toObject().value("freeNumber").toString();
                    QString hideNumber =data.value("data").toObject().value("hideNumber").toString();

                    if(!vipModelList.isEmpty()){
                        vipModelList.clear();
                    }
                    vipModelList<<empowerNumber<<freeNumber<<hideNumber;
                    PluginCorrespond::getInstance()->updateViplPlugins(vipModelList);
                } else {
                    qDebug()<<"None vip content!";
                }
            }, this);
        }else{
            isUserVip = 2;

            NET->get(freeUserForAuthorizeModelUrl, [=](FuncBody f) {
                QJsonObject data = f.j.value("data").toObject();
                if (data.value("code").toInt() == 1) {
                    freeTime = data.value("data").toObject().value("durationAes").toString();
                    NET->get(universalClientUrl, [=](FuncBody f) {
                        QJsonObject data = f.j.value("data").toObject();
                        if (data.value("code").toInt() == 1) {
                            QString empowerNumber =data.value("data").toObject().value("empowerNumber").toString();
                            QString freeNumber =data.value("data").toObject().value("freeNumber").toString();
                            QString hideNumber =data.value("data").toObject().value("hideNumber").toString();

                            if(!nonVipModelList.isEmpty()){
                                nonVipModelList.clear();
                            }
                            nonVipModelList<<empowerNumber<<freeNumber<<hideNumber;
                            PluginCorrespond::getInstance()->updateNonViplPlugins(freeTime, nonVipModelList);
                        } else {
                            qDebug()<<"None nonVip content!";
                        }
                    }, this);
                }
            }, this);
        }


        if (1 != status) {
            m_vipTimer.start(30000);
        } else {
            qint64 s = QDateTime::currentMSecsSinceEpoch();
            qint64 e = QDateTime::fromString(endTime, "yyyy-MM-dd HH:mm:ss").toMSecsSinceEpoch();
            m_vipTimer.start(qMin(qAbs(e - s), qint64(30000)));
        }

        if(showAccount){
            showAccountWidget(UserInfo::instance()->userStatusObj);
        }else{
            qDebug()<<"Not showAccountWidget!";
        }
    }, this);
}

void MainWindow::showAccountWidget(QJsonObject userInfo){
    account->initAccount(UserInfo::instance()->userName(), userInfo);
    QPoint pos = ui->head->mapToGlobal(QPoint(0, 0));
    pos.setX(pos.x() - account->width() / 2);
    pos.setY(pos.y() + ui->head->height() + 10);
    account->move(pos);
    this->canMove = false;


    NET->get(cloudInfos, [=](FuncBody f){
        QJsonObject resultData = f.j.value("data").toObject().value("data").toObject();
        if (!resultData.isEmpty()) {
            freeTimes = resultData.value("total").toInt() - resultData.value("used").toInt();
            nonFreeTimes = resultData.value("payTotal").toInt() - resultData.value("payUsed").toInt();
        } else {
            qDebug() << "查询余额失败!!!" << f.j;
        }

        account->setCloudTimes(freeTimes, nonFreeTimes);
        account->show();
    }, this);
}

void MainWindow::showRenderNavs()
{
    if (m_renderNavs.length() == 0)
        return;

    bool hide = m_renderNavs.at(0)->isHidden();
    foreach (QPointer<LeftNavItem> item, m_renderNavs) {
        if (item) {
            if (hide)
                item->show();
            else
                item->hide();
        }
    }
}

void MainWindow::showSetNavs()
{
    if (m_setNavs.length() == 0)
        return;

    bool hide = ui->setNav->isHidden();
    ui->setNav->setVisible(hide);
    foreach (QPointer<LeftNavItem> item, m_setNavs) {
        if (item) {
            if (hide)
                item->hide();
            else
                item->show();
        }
    }
}


//点击充值页面不相关页面的隐藏和选择高亮的不提示
void MainWindow::hideWindow_closeHighLight()
{
    ui->upload->setVisible(false);
    ui->defalut->setVisible(false);
    ui->changeMax->setVisible(false);
    ui->download->setVisible(false);
    navGroup->re_changeWidState(navClickL);
}

//接收信号失败，只能使用这种方法
void MainWindow::main_refresh_web(){
    qDebug()<<"444444444444";
    on_recharge_clicked();
}

//内嵌网页（刷新触发+按钮触发）
void MainWindow::on_recharge_clicked()
{
    qDebug()<<"222222222";
    ui->stackedWidget->setCurrentWidget(ui->reWidget);  //指定需要显示窗口widget
    hideWindow_closeHighLight();  //关闭高亮

    if(!recharge_web){
        recharge_web = new WebView(this);
        recharge_url = UserInfo::instance()->openRecharge();
        recharge_web->load(QUrl(recharge_url));
        ui->reWidget->layout()->addWidget(recharge_web);   //此时ui->reWidget->layout()->count()数量变为1
    }else{ //recharge_web存在
        recharge_url = UserInfo::instance()->openRecharge();
        recharge_web->load(QUrl(recharge_url));
    }
}

//展示Max文件下载列表窗口
void MainWindow::showDownMaxWidget(){
    ui->stackedWidget->setCurrentWidget(ui->downloadMaxList);
    if(ui->downloadMaxList->layout()->count() < 1){
        ui->downloadMaxList->layout()->addWidget(downloadMaxWidget);
    }else{

    }
    downloadMaxWidget->show();
}

//跳转云转模型网站
void MainWindow::jumpChangeMaxWeb(){
    ui->stackedWidget->setCurrentWidget(ui->webChangeMax);  //指定需要显示窗口widget
    hideWindow_closeHighLight();  //关闭高亮

    if(!changeMaxWeb){
        changeMaxWeb = new WebView(this);
        changeMaxUrl = UserInfo::instance()->getChangeMaxUrl();
        changeMaxWeb->load(QUrl(changeMaxUrl));
        ui->webChangeMax->layout()->addWidget(changeMaxWeb);
    }else{
        changeMaxUrl = UserInfo::instance()->getChangeMaxUrl();
        changeMaxWeb->load(QUrl(changeMaxUrl));
    }
}

//跳转云转材质网站
void MainWindow::jumpChangeMaterialWeb(){
    ui->stackedWidget->setCurrentWidget(ui->webChangeMaterial);  //指定需要显示窗口widget
    hideWindow_closeHighLight();  //关闭高亮

    if(!changeMaterialWeb){
        changeMaterialWeb = new WebView(this);
        changeMaterialUrl = UserInfo::instance()->getChangeMaterialUrl();
        changeMaterialWeb->load(QUrl(changeMaterialUrl));
        ui->webChangeMaterial->layout()->addWidget(changeMaterialWeb);
    }else{
        changeMaterialUrl = UserInfo::instance()->getChangeMaterialUrl();
        changeMaterialWeb->load(QUrl(changeMaterialUrl));
    }
}

//用户是否为Vip
int MainWindow::isUserIdVip(){
    return isUserVip;
}

void MainWindow::on_bannerBtn_clicked()
{
    if(ui->bannerWid->isVisible()){
        QString bannerUrl = bannerObj.value("domain").toString();
        if(bannerObj.value("inAppJump").toInt() == 0){
            QDesktopServices::openUrl(QUrl(bannerUrl));
        }else{
            if(noticeWindow){
                return;
            }else{
                QString jumpBannerUrl;
                int clientType = 0;
                QString code = UserInfo::instance()->reqSsoCode();

                if(bannerUrl.contains("https://www.xrender.com") || bannerUrl.contains("http://dev-xmax.cudatec.com")){
                    if(bannerObj.value("inAppJump").toInt() == 0){
                        clientType = 1;
                        jumpBannerUrl = QString("%1?code=%2&loginType=1&clientType=%3").arg(bannerUrl).arg(code).arg(clientType);
                    }else{
                        clientType = 2;
                        jumpBannerUrl = QString("%1?code=%2&loginType=1&clientType=%3").arg(bannerUrl).arg(code).arg(clientType);
                    }
                }else{
                    qDebug()<<"Other bannerUrl is:"<<bannerUrl;
                    jumpBannerUrl = bannerUrl;
                }
                qDebug()<<"jumpBannerUrl is:"<<jumpBannerUrl;
                showNoticeWidget(jumpBannerUrl);
            }
        }
    }else{
        qDebug()<<"Not click!";
    }
}

void MainWindow::showNoticeWidget(QString url){
    //应用内跳转
    noticeWindow = NoticeWidget::getInstance();
    noticeWindow->setWindowTitle(tr("公告"));
    noticeWindow->addWebPage(url);
    int moveForX = (this->width()-noticeWindow->width())/2;
    int moveForY = (this->height()-noticeWindow->height())/2;

    int realX = this->pos().x() + moveForX;
    int realY = this->pos().y() + moveForY;
    noticeWindow->showWindow();
    noticeWindow->move(realX, realY);
}


//这三个事件在切换双屏时才会出现触发--前提
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    QPoint localPos = ui->widget->mapFromGlobal(event->globalPos());  //
    qDebug()<<"ui->widget->childAt(localPos) is:"<<ui->widget->childAt(localPos)<<localPos;
    if(!ui->widget->childAt(localPos)){
        isMouseInTitleBar = true;
    }else{
        isMouseInTitleBar = false;
    }

    if(event->button() == Qt::LeftButton){
        reltvPos = event->pos();
    }else{}
}

void MainWindow::mouseMoveEvent(QMouseEvent *event){
    if(isMouseInTitleBar){
        move(event->globalPos() - reltvPos);
    }else{}
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event){
    isMouseInTitleBar = false;
}

void MainWindow::resizeEvent(QResizeEvent *event){
    double ratio = event->size().width() / 1056.0;
    emit resizeMainWindow(ratio);
}

void MainWindow::setMainWindowEnable(bool isEnable){
    for(auto item :navClickL){
        item->setEnabled(isEnable);
    }

    ui->presentLab->setEnabled(isEnable);
    ui->bannerBtn->setEnabled(isEnable);

    if(!account){
    }else{
        account->setWidgetStatus(isEnable);
    }
}

void MainWindow::jumpPersonWeb(QString personalUrl){
    QString jumpPersonalUrl = UserInfo::instance()->getPersonalUrl(personalUrl);
    QDesktopServices::openUrl(QUrl(jumpPersonalUrl));
}

void MainWindow::jumpChargeRuleWeb(){
    QString chargeRuleUrl = QString("https://www.xrender.com/v23/autoRenewal");
    QDesktopServices::openUrl(QUrl(chargeRuleUrl));
}
