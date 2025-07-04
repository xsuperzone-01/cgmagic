#include "updatecontent.h"
#include "ui_updatecontent.h"
#include "common/basewidget.h"
#include "view/softwareupdate.h"
#include "versions/versionmanager.h"
#include <QJsonArray>
#include "updateitem.h"
#include "version.h"
#include "tool/network.h"
#include "tool/jsonutil.h"
#include "quazip/JlCompress.h"
#include "versions/versionmanager.h"
#include "tool/childprocess.h"
#include "tool/msgtool.h"
#include "tool/xfunc.h"
#include "config/userinfo.h"
#include "updateset.h"
#include "view/pluginmanager.h"
#include <QDir>
#include <QStandardPaths>

UpdateContent::UpdateContent(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UpdateContent),
    m_client(false),
    m_completeSize(0),
    m_totalSize(0)
{
    ui->setupUi(this);
    connect(ui->up, &QPushButton::clicked, this, [=](){
        if(ui->widget->isVisible()){
            ui->widget->hide();
            ui->label_3->setText(tr("更新内容说明"));
            BaseWidget::setProperty(ui->up, "type", "down");
        }else {
            ui->widget->show();
            ui->label_3->setText(tr("更新内容："));
            BaseWidget::setProperty(ui->up, "type", "up");
        }
    });
    ui->widget->hide();
    ui->label_3->setText(tr("更新内容说明"));
    BaseWidget::setProperty(ui->up, "type", "down");
}


void UpdateContent::Allunclicked()
{
    ui->update->setEnabled(false);
}

void UpdateContent::Allclicked()
{
    ui->update->setEnabled(true);
}

void UpdateContent::Bottomtype()
{
    BaseWidget::setProperty(ui->backWid, "type", "1");
}

UpdateContent::~UpdateContent()
{
    foreach (QPointer<QFile> wf, m_replyFile.values()) {
        if (wf) {
            wf->close();
        }
    }
    delete ui;
}

void UpdateContent::setUpdateContentShow(bool isContentShow){
    if(isContentShow){
        //插件更新内容是显示的
        ui->widget_2->show();
        ui->widget_3->show();
        ui->horizontalWidget_2->show();
    }else{
        //客户端更新内容不显示
        ui->widget_2->hide();
        ui->widget_3->hide();
        ui->horizontalWidget_2->hide();
    }
}

void UpdateContent::initUpdateClient()
{
    connect(ui->update, &QPushButton::clicked, this, [=](){
        VersionManager::instance()->popup();
    });
}

void UpdateContent::initUpdateModule()
{
    int softId = m_obj.value("softwareId").toInt();
    version = m_obj.value("version").toString();
    path = USERINFO->allUserPath() + "/mobao/" + version;
    policyUrl = m_obj.value("policyUrl").toString();
    NET->get(policyUrl, [=](FuncBody f) {
        QJsonArray files = f.j.value("files").toArray();
        if (files.isEmpty()) {
            ui->label_4->setText(tr("无法获取文件"));
            return;
        }
        QString appDirPath = qApp->applicationDirPath();
        QString dir = appDirPath + "/" +QString("%1_%2/%3").arg(softId).arg(m_obj.value("softwareTag").toString()).arg(version);
        qDebug()<<dir;//41_cgmagic渲染插件/1.0.1
        bool success = QDir().mkpath(dir);
        qDebug()<<"文件夹创建"<<success;



        foreach (QJsonValue v, files) {
            QJsonObject f = v.toObject();
            QString name = f.value("path").toString();

            if(name == "readme.txt"){
                continue;
            }
            postfix = name;
            compressPath = QString (dir + "/" + name);
            if(compareHash(f)){
                ChangeStatus(ContinueInstall);
            }
            else {
                ChangeStatus(UpdatePlugin);
            }

        }
    }, this);
    connect(ui->update, &QPushButton::clicked, this, [=](){
        update_clicked();
    });
}

void UpdateContent::update(QJsonObject obj)
{
    m_obj = obj;
    m_client = obj.value("isClient").toBool();//是否为客户端
    QString nv = obj.value("version").toString();//版本号
    QString cv = obj.value("currentVersion").toString();//当前版本号,本地
    QString mt = VersionManager::instance()->clientTime(obj);//时间
    int s = obj.value("size").toInt();
    QString size = XFunc::getSizeString(s);
    ui->label_8->setText(tr("%1").arg(obj.value("isClient").toBool() ? tr("客户端更新") : tr("插件更新")));
    ui->label_4->setText(size);

    qDebug()<<"版本号："<<nv<<"当前版本号："<<cv;
    QString m_nv = QString("v" + nv);
    QString m_cv = QString("v" + cv);
    ui->label_7->setText(m_cv);
    ui->label_5->setText(m_nv);
    if (nv != cv && nv != "") {
        QString d = obj.value("detail").toString();

        BaseWidget::setLineHeight(ui->detail, 20);
        ui->detail->setText(d);

        int c = d.count("\r\n");
        c++;
        ui->detail->setFixedHeight(300);//+ 14
        int textHeight = c * 20;
        if(textHeight < ui->detail->height()){
            ui->detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }else{
            ui->detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        }
        emit updated(true);
    }
    else {
        ui->horizontalWidget->hide();
        ui->horizontalWidget_2->hide();
        ui->horizontalWidget_3->hide();
        ui->label_4->setText(tr("当前为最新版本"));
        emit updated(false);
    }

}

void UpdateContent::downloadProgress(qint64 complete, qint64 total)
{
    m_completeSize += complete;
    m_totalSize += total;
    if(m_completeSize == 0 && m_totalSize == 0)
    BaseWidget::setProperty(ui->update, "tab", "1");
    int pro = 0;
    int p = 0;
    if (m_totalSize != 0) {
        pro = m_completeSize * 30 / m_totalSize - 1;
        ui->update->setStyleSheet(QString("UpdateContent #update[tab='1'] {border-image: url(:/updateBtn/updating%1.png);}")
                                  .arg(pro));
        p = m_completeSize / m_totalSize;
    }
}

void UpdateContent::timeProgress()
{
    //时间开始
    if(count){
        count->deleteLater();
    }
    count = new QTimer(this);
    BaseWidget::setProperty(ui->update, "tab", "1");
    pro = 0;
    //每33/34ms进行一次计数，0~29循环
    connect(count, &QTimer::timeout, this, [=]{
        ui->update->setStyleSheet(QString("UpdateContent #update[tab='1'] {border-image: url(:/updateBtn/updating%1.png);}")
                                  .arg(pro));
        pro++;
        if (pro == 30){
            pro = 0;
        }
    });
    if (pro == 0) {
        count->start(33);
    }

}

void UpdateContent::update_clicked()
{
    emit btnEnable();

    if (m_client) {
        ui->update->setEnabled(false);
        VersionManager::instance()->popup();
        return;
    }
    m_completeSize = 0;
    m_totalSize = 0 ;
    downloadProgress(0, 0);

    if(updateStatus == ContinueInstall){
        ui->update->setText(tr("安装中..."));
        updateInstall();
    } else if(updateStatus == UpdatePlugin){
        ui->update->setText(tr("下载中..."));
        download();
    } else if(updateStatus == ContinueUpdate){
        ui->update->setText(tr("更新中..."));
        qDebug()<<"点击继续更新";
        updateNext();
    } else if(updateStatus == UpdateError){
        ui->update->setText(tr("更新中..."));
        updateNext();
    }

}

void UpdateContent::ChangeStatus(UpdateStatus u)
{
    QString text;

    switch (u) {
    case ContinueInstall:
        text = tr("继续安装");
        break;
    case UpdatePlugin:
        text = tr("更新插件");
        break;
    case ContinueUpdate:
        text = tr("继续更新");
        break;
    case UpdateError:
        text = tr("更新失败");
        break;
    }
    ui->update->setText(text);
    updateStatus = u;

}

bool UpdateContent::compareHash(QJsonObject f)
{
    QString ha = f.value("md5").toString();
    qDebug()<<compressPath;
    QString ms = XFunc::fileHash(compressPath);
    qDebug()<<ms;
    if(ha == ms){//本地文件哈希和下载的哈希比对-------------把这部分取出来，如果对比哈希相同显示为继续更新（反馈bool），点击下载/直接安装
        qDebug()<<"哈希比对相同";
        return true;
    }else {
        return false;
}
}

void UpdateContent::download()
{
    QNetworkAccessManager *man = new QNetworkAccessManager(this);
    QFile *wf = new QFile(compressPath, this);
    wf->setPermissions(QFileDevice::WriteOwner | QFileDevice::ReadOwner);   //增加文件可由文件的所有者读写权限
    bool ok = wf->open(QFile::WriteOnly|QFile::Truncate);
    qDebug()<<"File isOpened successfully:"<<wf->isOpen();
    qDebug()<<ok<<wf->errorString()<<"压缩文件错误";
    QNetworkRequest req(QUrl(QString(policyUrl).replace("policy.json", postfix)));//将policy.json替换为name
    QNetworkReply *reply = man->get(req);
    m_replyFile.insert(reply, wf);//插入
    connect(reply, &QNetworkReply::readyRead, this, &UpdateContent::writeFile);
    connect(reply, &QNetworkReply::downloadProgress, this, &UpdateContent::downloadProgress);
    connect(reply, &QNetworkReply::finished, this, &UpdateContent::replyFinished);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void UpdateContent::writeFile()
{
    if (QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender())) {
        QFile *wf = m_replyFile.value(reply);
        if (wf) {
            qint64 wn = wf->write(reply->readAll());
        }
    }
}

void UpdateContent::replyFinished()
{
    qDebug()<<"下载完成！！！！！！！！";
    if (count) {
    count->stop();
    count->deleteLater();
    }
    ui->update->setStyleSheet(QString("UpdateContent #update[tab='1'] {border-image: url(:/updateBtn/updating0.png);}"));
    ui->update->setText(tr("安装中..."));
    if (QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender())) {
        QFile *wf = m_replyFile.value(reply);
        if (wf) {
            wf->close();
            wf->deleteLater();
            QString fn = wf->fileName();
            qDebug()<< __FUNCTION__<<compressPath;          //UpdateContent::replyFinished "41_cgmagic渲染插件/1.0.1/plugin.zip"
            if (fn == compressPath && fn.contains("plugin.zip")) {
                updateInstall();
                }
            }
        m_replyFile.remove(reply);
    }
}


void UpdateContent::updateInstall()
{
    qDebug()<<path;
    QDir save(path);
    if(!save.exists()) {
        save.mkpath(path);
    }
    JlCompress::extractDir(compressPath, path);//解压
    qDebug()<<"从"<<compressPath<<"解压到"<<path;
    //TODO 安装
    qDebug()<<"解压完成继续安装";
    updateNext();
}

void UpdateContent::retry()
{
    MsgBox *mb = MsgTool::msgChoose(tr("请先关闭max，再安装插件。"));
    QPushButton *okBtn = mb->findChild<QPushButton*>("okBtn");
    QPushButton *noBtn = mb->findChild<QPushButton*>("noBtn");
    okBtn->hide();
    noBtn->setText(tr("确认"));
    mb->setBackgroundMask();
    connect(mb, &MsgBox::close, this, [=](){
        return;
    });
}

void UpdateContent::updateNext()
{
    if(ChildProcess::existProcess("3dsmax.exe")) {           //ChildProcess::existProcess "信息: 没有运行的任务匹配指定标准。\r\n"
        retry();
        ChangeStatus(ContinueUpdate);
        emit btnabled();
    } else {
        QString plugPath = QString(qApp->applicationDirPath() + "/mobao");
        qDebug()<< __FUNCTION__<<plugPath;//存在当前运行文件夹/mobao  "D:/xsuperqzone/xdemo/install_ui/cgmagic/build-client-Desktop_Qt_5_14_2_MSVC2017_32bit-Release/release" aaaaa
        bool copy = XFunc::veryCopy(path, plugPath);//将文件从缓存目录下移至运行目录
        qDebug()<<"复制是否成功"<<copy;
        //如果复制失败就从下载地解压一个过来
        if (!copy) {
            qDebug()<<"从"<<compressPath<<"解压到"<<plugPath;
            JlCompress::extractDir(compressPath, plugPath);
        }
        updateSave();
    }
}

void UpdateContent::updateSave()
{
    PluginManager pm;
    if(!pm.comparePlugins()){
        if(pm.repairPlugin()){
            if(pm.comparePlugins()){
                updateFinished();
            }else {
                updateFail();
            }
        }else {
            updateFail();
        }
    }else{
        updateFinished();
    }
}

void UpdateContent::versionSave()//暂时没用
{
    QString appDirPath = qApp->applicationDirPath();
    QString d1 = QString(appDirPath + "/mobao/version.txt");
    QFile ver(d1);
    if(!ver.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text)){
        qDebug()<<"文件打开失败";
        updateFail();
        return;
    }
    ver.write(version.toUtf8());
    ver.close();
}


void UpdateContent::updateFinished()
{

    ui->update->setText(tr("更新完成"));
    emit updateModule();//更新状态---刷新界面通过update检测。
    emit btnabled();//按钮解开
}

void UpdateContent::updateFail()
{
    ChangeStatus(UpdateError);
    ui->update->setStyleSheet("UpdateContent #update {background: rgba(255, 255, 255, 0.10);border-radius: 4px;}");
    emit btnabled();//按钮解开
}
