
#include "renderset.h"
#include "ui_renderset.h"

#include <QDir>
#include "set.h"
#include "common/basewidget.h"
#include "tool/network.h"
#include "tool/jsonutil.h"
#include "common/buttongroup.h"
#include <QTimer>

#define SetG "RenderSet"
#define PushToK "pushTo"
#define SameFileK "sameFile"

bool RenderSet::m_autoPush;
bool RenderSet::m_autoOpenDir;
RenderSet::SameFile RenderSet::m_sameFile;
bool RenderSet::m_psd;
bool RenderSet::m_channel;
bool RenderSet::m_prefix;

RenderSet::RenderSet(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RenderSet)
{
    ui->setupUi(this);

    ui->loginType->hide();

    QList<QPushButton *> btnL;
    btnL << ui->render << ui->plugin;
    foreach (QPushButton *btn, btnL) {
        BaseWidget::setClass(btn, "switchLogin");
    }
    QList<QFrame *> lineL;
    lineL << ui->renderLine << ui->pluginLine;
    foreach (QFrame *line, lineL) {
        BaseWidget::setClass(line, "switchLine");
    }
    ButtonGroup *bg = new ButtonGroup(this);
    connect(bg, &ButtonGroup::btnIdClick, [=](int id){
        foreach (QFrame *line, lineL) {
            BaseWidget::setProperty(line, "tab", "");
        }
        BaseWidget::setProperty(lineL.at(id), "tab", "1");
    });
    bg->addButtons(btnL, qApp->style(), ui->stackedWidget);
    if(QAbstractButton *button = bg->button(0)) {
        emit bg->buttonClicked(button);
    }else{
        qDebug()<<"Button with ID 0 does not exist!";
    }

    ui->env->clearEnvSet();
    ui->env->initEnvSet(3280, 3);

    connect(ui->autoPush, &QCheckBox::clicked, this, [=](bool checked){
        setAutoPush(checked);
    });
    connect(ui->downloadOpenDir, &QCheckBox::clicked, this, [=](bool checked){
        setAutoOpenDir(checked);
    });
    foreach (QRadioButton *rb, QList<QRadioButton *>()<< ui->pushToCache << ui->pushToSrc) {
        connect(rb, &QRadioButton::clicked, this, [=](bool checked){
            setPushTo(PushTo(rb->property(PushToK).toInt()));
        });
    }
    foreach (QRadioButton *rb, QList<QRadioButton *>()<< ui->confirm << ui->cover << ui->rename) {
        connect(rb, &QRadioButton::clicked, this, [=](bool checked){
            setSameFile(SameFile(rb->property(SameFileK).toInt()));
        });
    }
    connect(ui->psd, &QCheckBox::clicked, this, [=](bool checked){
        setPsd(checked);
    });
    connect(ui->channel, &QCheckBox::clicked, this, [=](bool checked){
        setChannel(checked);
    });
    connect(ui->prefix, &QCheckBox::clicked, this, [=](bool checked){
        setPrefix(checked);
    });
    foreach (QRadioButton *rb, QList<QRadioButton *>()<< ui->warn << ui->warnCancel) {
        connect(rb, &QRadioButton::clicked, this, &RenderSet::setWarn, Qt::UniqueConnection);
    }
    ui->warnTime->setEnabled(false);
    connect(ui->warnTime, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RenderSet::setWarn, Qt::UniqueConnection);

//    ui->pushTip->setToolTip(tr("结果文件下载路径将按所配置类型下载，如当前为文件内设置路径，但本机无对应路径则默认推送至缓存路径"));
//    ui->renameTip->setToolTip(tr("如客户端检测到文件重名则按所配置进行处理\r\n"
//                                 "询问：下载时将弹窗提示 覆盖/重命名/取消下载\r\n"
//                                 "覆盖：下载时将覆盖重名文件\r\n"
//                                 "重命名：下载时自动重命名重名文件"));
//    ui->psdTip->setToolTip(tr("提交插件内需勾选合成PSD通道"));
//    ui->channelTip->setToolTip(tr("Corona渲染如使用了渲染元素会自动创建文件夹，启用后将不在本地创建文件夹"));
//    ui->prefixTip->setToolTip(tr("将对应相机的结果文件名称前添加前缀"));

    connect(ui->note, &QPushButton::clicked, this, [=](){
        if(ui->widget_11->isVisible()){
            ui->widget_11->hide();
            BaseWidget::setProperty(ui->note, "type", "down");
        }else {
            ui->widget_11->show();
            BaseWidget::setProperty(ui->note, "type", "up");
        }
    });
    connect(ui->note_2, &QPushButton::clicked, this, [=](){
        if(ui->widget_14->isVisible()){
            ui->widget_14->hide();
            BaseWidget::setProperty(ui->note_2, "type", "down");
        }else {
            ui->widget_14->show();
            BaseWidget::setProperty(ui->note_2, "type", "up");
        }
    });
    connect(ui->note_3, &QPushButton::clicked, this, [=](){
        if(ui->widget_23->isVisible()){
            ui->widget_23->hide();
            BaseWidget::setProperty(ui->note_3, "type", "down");
        }else {
            ui->widget_23->show();
            BaseWidget::setProperty(ui->note_3, "type", "up");
        }
    });
    connect(ui->note_4, &QPushButton::clicked, this, [=](){
        if(ui->widget_24->isVisible()){
            ui->widget_24->hide();
            BaseWidget::setProperty(ui->note_4, "type", "down");
        }else {
            ui->widget_24->show();
            BaseWidget::setProperty(ui->note_4, "type", "up");
        }
    });

}

RenderSet::~RenderSet()
{
    delete ui;
}

void RenderSet::initRenderSet()
{
    ui->autoPush->setChecked(autoPush());
    ui->downloadOpenDir->setChecked(autoOpenDir());

    ui->pushToCache->setProperty(PushToK, PushTo::Cache);
    ui->pushToSrc->setProperty(PushToK, PushTo::Src);
    switch (pushTo()) {
    case PushTo::Cache:
        ui->pushToCache->setChecked(true);
        break;
    case PushTo::Src:
        ui->pushToSrc->setChecked(true);
        break;
    default:
        break;
    }

    ui->confirm->setProperty(SameFileK, SameFile::Confirm);
    ui->cover->setProperty(SameFileK, SameFile::Cover);
    ui->rename->setProperty(SameFileK, SameFile::Rename);
    switch (sameFile()) {
    case SameFile::Confirm:
        ui->confirm->setChecked(true);
        break;
    case SameFile::Cover:
        ui->cover->setChecked(true);
        break;
    case SameFile::Rename:
        ui->rename->setChecked(true);
        break;
    default:
        break;
    }

    ui->psd->setChecked(psd());
    ui->channel->setChecked(channel());
    ui->prefix->setChecked(prefix());

    NET->xrget("/bs/user/sets", [=](FuncBody f){
        int status = f.j.value("status").toInt();
        if (status == 18300000) {
            int timeout = f.j["timeout"].toInt();
            int kill = f.j["killTimeout"].toInt();
            ui->warnTime->setValue((double)timeout / 3600.0);
            if (kill == 0)
                ui->warn->setChecked(true);
            else
                ui->warnCancel->setChecked(true);
            ui->warnTime->setEnabled(true);
        }
    }, this);

//    QTimer::singleShot(300, this, [=](){
//            ui->warnTime->setEnabled(true);
//    });
}

void RenderSet::initRenderSetIni()
{
    if (!USERINFO->existUserIni(SetG, "autoPush")) {
        setAutoPush(true);
    }
    m_autoPush = USERINFO->readUserIni(SetG, "autoPush").toBool();

    m_autoOpenDir = USERINFO->readUserIni(SetG, "autoOpenDir").toBool();

    if (!USERINFO->existUserIni(SetG, PushToK)) {
        setPushTo(PushTo::Cache);
    }

    if (!USERINFO->existUserIni(SetG, SameFileK)) {
        setSameFile(SameFile::Confirm);
    }
    m_sameFile = SameFile(USERINFO->readUserIni(SetG, SameFileK).toInt());

    m_psd = USERINFO->readUserIni(SetG, "psd").toBool();
    m_channel = USERINFO->readUserIni(SetG, "channel").toBool();
    m_prefix = USERINFO->readUserIni(SetG, "prefix").toBool();

    QString dir = cacheDir();
    if (!QDir(dir).exists()) {
        QDir().mkpath(dir);
    }
}

bool RenderSet::autoPush()
{
    return m_autoPush;
}

void RenderSet::setAutoPush(bool v)
{
    m_autoPush = v;
    USERINFO->saveUserIni(SetG, "autoPush", v);
}

bool RenderSet::autoOpenDir()
{
    return m_autoOpenDir;
}

void RenderSet::setAutoOpenDir(bool v)
{
    m_autoOpenDir = v;
    USERINFO->saveUserIni(SetG, "autoOpenDir", v);
}

RenderSet::PushTo RenderSet::pushTo()
{
    return PushTo(USERINFO->readUserIni(SetG, PushToK).toInt());
}

void RenderSet::setPushTo(RenderSet::PushTo v)
{
    USERINFO->saveUserIni(SetG, PushToK, v);
}

QString RenderSet::cacheDir()
{
    return Set::cacheDir() + "\\RenderDownloads";
}

RenderSet::SameFile RenderSet::sameFile()
{
    return m_sameFile;
}

void RenderSet::setSameFile(RenderSet::SameFile v)
{
    m_sameFile = v;
    USERINFO->saveUserIni(SetG, SameFileK, v);
}

bool RenderSet::psd()
{
    return m_psd;
}

void RenderSet::setPsd(bool v)
{
    m_psd = v;
    USERINFO->saveUserIni(SetG, "psd", v);
}

bool RenderSet::channel()
{
    return m_channel;
}

void RenderSet::setChannel(bool v)
{
    m_channel = v;
    USERINFO->saveUserIni(SetG, "channel", v);
}

bool RenderSet::prefix()
{
    return m_prefix;
}

void RenderSet::setPrefix(bool v)
{
    m_prefix = v;
    USERINFO->saveUserIni(SetG, "prefix", v);
}

void RenderSet::setWarn()
{
    qDebug()<<sender()<<ui->warnCancel->isChecked();
    if(!ui->warnTime->isEnabled()){
        return;
    }
    //限制
    QString ts = QString::number(ui->warnTime->value(), 'f', 1);
    if (!ts.endsWith(".0") && !ts.endsWith(".5")) {
        ui->warnTime->setValue(double(int(ui->warnTime->value() + 0.5f)));
        return;
    }


    int time = ui->warnTime->value() * 3600;
    int kill = 0;
    if (ui->warnCancel->isChecked())
        kill = 1;

    QJsonObject obj;
    obj.insert("timeout", time);
    obj.insert("killTimeout", kill);
//            obj.insert("alertDisable", alertDisable);
    NET->xrput(QString("/bs/user/sets/edit"), JsonUtil::jsonObjToByte(obj), [=](FuncBody f){
    }, this);

//    ui->warnTime->setEnabled(true);

//    QTimer::singleShot(300, this, [=](){
//ui->warnTime->setEnabled(true);
//    });
}

void QAbstractSpinBox::wheelEvent(QWheelEvent *e)
{
}

void RenderSet::on_warnTime_valueChanged(const QString &arg1)
{
    if (ui->warnTime->value() == 0.5) {
        BaseWidget::setProperty(ui->warnTime, "type", "0");
    } else if (ui->warnTime->value() == 24.0){
        BaseWidget::setProperty(ui->warnTime, "type", "2");
    } else {
        BaseWidget::setProperty(ui->warnTime, "type", "1");
    }
}
