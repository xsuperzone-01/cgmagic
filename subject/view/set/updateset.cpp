#include "updateset.h"
#include "ui_updateset.h"

#include "version.h"
#include "common/flowlayout.h"
#include "common/eventfilter.h"
#include "set.h"
#include "setting.h"
#include <QDebug>

UpdateSet::UpdateSet(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UpdateSet)
{
    ui->setupUi(this);
    ui->itemWid->hide();
    ui->updateAll->hide();
    ui->logo_2->hide();
    ui->logo_3->hide();
    BaseWidget::setProperty(ui->clientLine, "tab", "1");
    BaseWidget::setProperty(ui->moduleLine, "tab", "");
    BaseWidget::setProperty(ui->client, "tab", "1");
    BaseWidget::setProperty(ui->module, "tab", "");
}

UpdateSet::~UpdateSet()
{
    delete ui;
}

void UpdateSet::initUpdateSet(int c, int p)
{
    classSet(c, p);//设置基础样式

    fileRead();

    on_client_clicked();
    on_module_clicked();//安装点击槽

    moduleClear();
    clientClear();

    if(ui->module->property("tab") == "1") {
        moduleShow();
    }else {
        clientShow();
        moduleAdd();
        ui->logo_2->show();
        ui->logo_3->show();
    }

    ui->updateAll->setEnabled(false);
}

void UpdateSet::classSet(int c, int p)
{

    if (c != 0)
        ui->client->setText(tr("客户端更新(1)"));
    if (p != 0)
        ui->module->setText(QString(tr("模块更新(%1)")).arg(p));
    if (p + c != 0)
        ui->title->setText(QString(tr("更新和下载(%1)")).arg(p + c));
    ui->widget_2->hide();
    num_p = p;
    num_t = p + c;

    removePlugins();
    QPointer<UpdateSet> ptr = this;
    QPointer<VersionManager> vm = VersionManager::instance();
    vm->check();
    if (!ptr)
        return;

    BaseWidget::setClass(ui->updateAll, "okBtn");

    QList<QPushButton *> btnL;
    btnL << ui->client <<ui->module ;
    foreach (QPushButton *btn, btnL) {
        BaseWidget::setClass(btn, "switchLogin");
    }

    QList<QFrame *> lineL;
    lineL << ui->clientLine << ui->moduleLine;
    foreach (QFrame *line, lineL) {
        BaseWidget::setClass(line, "updateLine");
    }
}

void UpdateSet::on_client_clicked()
{
    connect(ui->client, &QPushButton::clicked, this, [=](){
        BaseWidget::setProperty(ui->clientLine, "tab", "1");
        BaseWidget::setProperty(ui->moduleLine, "tab", "");
        BaseWidget::setProperty(ui->client, "tab", "1");
        BaseWidget::setProperty(ui->module, "tab", "");
        ui->updateAll->hide();
        ui->logo_2->show();
        moduleClear();
        clientShow();
    });
}

void UpdateSet::clientClear()
{
    if(m_client){
        ui->UpdateContent->layout()->removeWidget(m_client);
        m_client->hide();
        m_client.clear();
    }
}

void UpdateSet::clientShow()
{
    QPointer<VersionManager> vm = VersionManager::instance();
    QJsonObject obj = vm->client();
    obj.insert("isClient", true);
    obj.insert("currentVersion", CLIENT_VERSION);
    QVBoxLayout* ly = qobject_cast<QVBoxLayout*>(ui->UpdateContent->layout());
    if(!m_client) {
        m_client = new UpdateContent(this);
        m_client->update(obj);
        m_client->initUpdateClient();
    }
    m_client->setUpdateContentShow(false);
    m_client->setContentsMargins(0, 12, 0, 0);
    ly->insertWidget(0, m_client);
    m_client->Bottomtype();
    m_client->show();
}

void UpdateSet::on_module_clicked()
{
    connect(ui->module, &QPushButton::clicked, this, [=](){
        BaseWidget::setProperty(ui->moduleLine, "tab", "1");
        BaseWidget::setProperty(ui->clientLine, "tab", "");
        BaseWidget::setProperty(ui->client, "tab", "");
        BaseWidget::setProperty(ui->module, "tab", "1");
        ui->logo_2->hide();
        clientClear();
        moduleShow();
    });
}

void UpdateSet::moduleClear()
{
    if(!m_module.isEmpty()){
        int length = m_module.length();
        for (int i = 0; i<length; i++) {
            ui->UpdateContent->layout()->removeWidget(m_module.at(i));
            m_module.at(i)->hide();
        }
        m_module.clear();
    }
}

void UpdateSet::moduleShow()
{
    if(m_module.isEmpty()) {
        moduleAdd();
    }
    insertLay();
}

void UpdateSet::moduleAdd()
{
    QPointer<VersionManager> vm = VersionManager::instance();
    foreach (QJsonObject plg, vm->plugins) {
        plg.insert("currentVersion", v);//插入当前版本号
        UpdateContent *item = new UpdateContent(this);
        connect(item, &UpdateContent::updateModule, this, &UpdateSet::updated);//信号捆绑
        item->update(plg);
        item->initUpdateModule();
        item->setContentsMargins(0, 12, 0, 0);
        if (plg.value("versionType") == "force"){
            item->setProperty("force", true);
        }
        else {
            item->setProperty("force", false);
        }
        item->update(plg);
        m_module.append(item);
    }
    insertLay();
}

void UpdateSet::insertLay()
{
    QVBoxLayout* ly = qobject_cast<QVBoxLayout*>(ui->UpdateContent->layout());
    QList<QPointer<UpdateContent>> m;
    qDebug()<<"插件数量："<<m_module.length();
    for (int i = 0;i < m_module.length(); i++) {
        ly->insertWidget(1, m_module.at(i));
        if(Set::updateStableVersion()){//Set界面更新稳定版本按下
            if (m_module.at(i)->property("force") == true){
                m_module.at(i)->show();
                m<<m_module.at(i);
            }else {
                m_module.at(i)->hide();
            }
        }else {
            m_module.at(i)->show();
            m_module.at(0)->Bottomtype();
        }
        connect(m_module.at(i), &UpdateContent::btnEnable, this, [=](){
            for (int i = 0;i < m_module.length(); i++)
                m_module.at(i)->Allunclicked();
        });
        connect(m_module.at(i), &UpdateContent::btnabled, this, [=](){
            for (int i = 0;i < m_module.length(); i++)
                m_module.at(i)->Allclicked();
        });
    }
    for (int j = 0;j <m.length();j++) {
        if(j == 0){
            m.first()->Bottomtype();
        }
    }
}

void UpdateSet::fileRead()
{
    QString ver = QString(qApp->applicationDirPath() + "/mobao/version.txt");
    QFile verison(ver);
    if(!verison.open(QIODevice::ReadOnly|QIODevice::Text)){
        qDebug()<<"/mobao/version.txt打开失败";
    }
    v = verison.read(10);
    verison.close();
}

void UpdateSet::updated()
{
    update();
    num_p--;
    num_t--;
    qDebug()<<"模块未更新个数"<<num_p;
    qDebug()<<"总计还有多少未更新"<<num_t;
    if (num_p != 0) {
        ui->module->setText(QString(tr("模块更新(%1)")).arg(num_p));
        ui->title->setText(QString(tr("更新和下载(%1)")).arg(num_t));
    } else {
        ui->module->setText(tr("模块更新"));
        if (num_t != 0) {
            ui->title->setText(QString(tr("更新和下载(%1)")).arg(num_t));
        } else {
            ui->title->setText(tr("更新和下载"));
            emit haveUpdated();
        }
    }
}

void UpdateSet::update()
{
    if(!m_module.isEmpty()){
        for (int i = 0; i<m_module.length(); i++) {
            ui->UpdateContent->layout()->removeWidget(m_module.at(i));
            m_module.at(i)->hide();
            m_module.at(i)->deleteLater();
        }
    }
    m_module.clear();
    fileRead();
    moduleAdd();
}
void UpdateSet::clearModule()
{
    if(ui->module->property("tab") == "1"){
        ui->client->clicked();
        ui->module->clicked();
    }
    if(ui->module->property("tab") == "0"){
        ui->module->clicked();
        ui->client->clicked();
    }
}
void UpdateSet::enabled(bool o)
{
    ui->updateAll->setEnabled(o);
}
void UpdateSet::removePlugins()
{
    QHBoxLayout *lay = (QHBoxLayout *)ui->bottomWid->layout();
    foreach (UpdateItem *item, findChildren<UpdateItem *>()) {
        if (item == ui->UpdateContent)
            continue;
        lay->removeWidget(item);
        item->deleteLater();
    }
}
