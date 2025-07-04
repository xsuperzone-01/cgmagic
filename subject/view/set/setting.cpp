#include "setting.h"
#include "ui_setting.h"
#include <QDebug>
#include "view/leftnavitem.h"
#include "view/set/set.h"
#include "view/set/maxset.h"
#include "view/set/renderset.h"
#include "view/set/updateset.h"
#include "view/about.h"
#include "common/widgetgroup.h"
#include "envset.h"
#include "versions/versionmanager.h"
#include "version.h"

Setting::Setting(QWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::Setting)
{
    ui->setupUi(this);
    {
        selfAdaptionSize();
        selfAdaptionMargins();
        resizeChildrenMargin();
        setMacLayout(ui->headClose);
    }

    setClass(ui->headClose, "close");

    raise();
    activateWindow();

    c = 0;
    p = 0;
    n = false;

}

Setting::~Setting()
{
    delete ui;
}

void Setting::NeedUpdate()
{
    QPointer<VersionManager> vm = VersionManager::instance();
    vm->check();
    {
        QJsonObject obj = vm->client();
        QString ncv = obj.value("version").toString();//版本号
        QString ccv = CLIENT_VERSION;
        if ( ccv == "" || ncv != ccv) {
            c = 1;
        }
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
    }
    qDebug()<<__FUNCTION__<<c<<p;
    if (c + p != 0) {
        n = true;
        qDebug()<<"显示更新提示"<<n;
        emit needed();
    }else {
        emit neednot();
    }
}

void Setting::setSetting()
{
    modaldel();
    setBackgroundMask();
    show();


    QStringList leftItem;
    QString lan = Set::changeLan();
    if(lan == "en_us"){
        ui->headTitle->setText(tr("通用设置英文"));
        leftItem << tr("通用设置") <<tr("软件更新");
    }else{
        ui->headTitle->setText(tr("通用设置"));
        leftItem << tr("通用设置") <<tr("软件更新") << tr("关于");
    }

    QVBoxLayout *navLayout = (QVBoxLayout *)ui->setList->layout();
    QList<BaseClickWidget *> navClickL;
    BaseClickWidget *currentNav = NULL;

    for (int i = 0; i < leftItem.length(); i++) {
        LeftNavItem *item = new LeftNavItem(ui->setList);
        item->setFixedHeight(36);
        item->hideIcon();
        navLayout->addWidget(item);
        if (i == 1) {
            item->initLeftNavItem(leftItem.at(i), -1, n);
            connect(this, &Setting::neednot, this, [=](){
                item->initLeftNavItem(leftItem.at(1), -1, n);
            });
        }else {
            item->initLeftNavItem(leftItem.at(i), -1);//-1代表不需要插入icon
        }
        switch (i) {
        case 0:
            connect(item, &LeftNavItem::clicked, this, [=]{
                ui->stackedWidget->setCurrentIndex(0);
                if (!m_set) {
                    m_set = new Set(this);//把主程序中的字搞没了
                    ui->set->layout()->addWidget(m_set);
                    m_set->initSet();//主界面字和这句无关
                }
            });
            currentNav = item;
            break;
        case 1:
            connect(item, &LeftNavItem::clicked, this, [=]{
                ui->stackedWidget->setCurrentIndex(1);
                if (!m_update) {
                    m_update = new UpdateSet(this);
                    ui->update->layout()->addWidget(m_update);
                    m_update->initUpdateSet(c, p);
                    connect(m_update, &UpdateSet::haveUpdated, this, [=](){
                        c = 0;
                        p = 0;
                        n = false;
                        NeedUpdate();
                    });//这部分为更新完成，后面需要接收到未更新完成
                    connect(this, &Setting::needed, this, [=](){
                        item->initLeftNavItem(leftItem.at(1), -1, n);
                        qDebug()<<"需要更新"<<n<<"客户端"<<c<<"插件"<<p;
                        m_update->initUpdateSet(c, p);
                    });
                }
                m_update->clearModule();
            });
            break;
        case 2:
            connect(item, &LeftNavItem::clicked, this, [=]{
                ui->stackedWidget->setCurrentIndex(2);
                if (!m_about) {
                    m_about = new About(this);
                    ui->about->layout()->addWidget(m_about);
                }
                m_about->initAbout();
            });
            break;
        default:
            break;
        }
        navClickL << item;
    }
    widgetGroup *navGroup = new widgetGroup(this);
    navGroup->addWidgets(navClickL, qApp->style(), currentNav);
}

void Setting::updateSetting()
{

}

void Setting::setRender()
{
    modaldel();
    setBackgroundMask();
    show();

    ui->headTitle->setText(tr("云渲染设置"));
    QStringList leftItem;
    leftItem << tr("云渲染设置") <<tr("插件设置");
    QVBoxLayout *navLayout = (QVBoxLayout *)ui->setList->layout();
    QList<BaseClickWidget *> navClickL;
    BaseClickWidget *currentNav = NULL;

    for (int i = 0; i < leftItem.length(); i++) {
        LeftNavItem *item = new LeftNavItem(ui->setList);
        item->setFixedHeight(36);
        item->hideIcon();
        navLayout->addWidget(item);
        item->initLeftNavItem(leftItem.at(i), -1);//-1代表不需要插入icon

        switch (i) {
        case 0:
            connect(item, &LeftNavItem::clicked, this, [=](){
                ui->stackedWidget->setCurrentIndex(3);
                if (!m_render) {
                    m_render = new RenderSet(this);
                    ui->render->layout()->addWidget(m_render);
                    m_render->initRenderSet();
                }

            });
            currentNav = item;
            break;
        case 1:
            connect(item, &LeftNavItem::clicked, this, [=](){
                ui->stackedWidget->setCurrentIndex(4);
                if (!m_env) {
                    m_env = new EnvSet(this);
                    ui->envnew->layout()->addWidget(m_env);
                }
                m_env->initEnvSet(3280, 3);
            });
            break;
        default:
            break;
        }
        navClickL << item;
    }
    widgetGroup *navGroup = new widgetGroup(this);
    navGroup->addWidgets(navClickL, qApp->style(), currentNav);

}

void Setting::setMax()
{
    modaldel();
    setBackgroundMask();

    show();
    ui->headTitle->setText(tr("云转模设置"));
    QStringList leftItem;
    leftItem << tr("云转模设置");
    QVBoxLayout *navLayout = (QVBoxLayout *)ui->setList->layout();
    QList<BaseClickWidget *> navClickL;
    BaseClickWidget *currentNav = NULL;

    LeftNavItem *item = new LeftNavItem(ui->setList);
    item->setFixedHeight(36);
    item->hideIcon();
    navLayout->addWidget(item);
    item->initLeftNavItem(leftItem.at(0), -1);//-1代表不需要插入icon

    connect(item, &LeftNavItem::clicked, this, [=]{
        ui->stackedWidget->setCurrentIndex(5);
        if (!m_max) {
            m_max = new MaxSet(this);
            ui->max->layout()->addWidget(m_max);
            m_max->initMaxSet();
        }
    });
    currentNav = item;
    navClickL << item;

    widgetGroup *navGroup = new widgetGroup(this);
    navGroup->addWidgets(navClickL, qApp->style(), currentNav);

}

QPointer<MaxSet> Setting::maxSet()
{
    return m_max;
}

void Setting::on_headClose_clicked()
{
    this->close();
}

void Setting::closeSetting()
{
    on_headClose_clicked();
}


bool Setting::event(QEvent *event)
{
    return QWidget::event(event);
}
