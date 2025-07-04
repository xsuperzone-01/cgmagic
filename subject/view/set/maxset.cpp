#include "maxset.h"
#include "ui_maxset.h"

#include <QDir>
#include "common/basewidget.h"
#include "set.h"

#define SetG "MaxSet"

MaxSet::MaxSet(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MaxSet)
{
    ui->setupUi(this);

    connect(ui->autoPush, &QCheckBox::clicked, this, [=](bool checked){
        setAutoPush(checked);
        emit updateSet();
    });
    connect(ui->downloadOpenDir, &QCheckBox::clicked, this, [=](bool checked){
        setAutoOpenDir(checked);
        emit updateSet();
    });
    foreach (QRadioButton *rb, QList<QRadioButton *>()<< ui->pushToCache << ui->pushToSrc) {
        connect(rb, &QRadioButton::clicked, this, [=](bool checked){
            setPushTo(PushTo(rb->property("pushTo").toInt()));
            emit updateSet();
        });
    }

    connect(ui->note_2, &QPushButton::clicked, this, [=](){
        if(ui->widget_6->isVisible()){
            ui->widget_6->hide();
            BaseWidget::setProperty(ui->note_2, "type", "down");
        }else {
            ui->widget_6->show();
            BaseWidget::setProperty(ui->note_2, "type", "up");
        }
    });

}

MaxSet::~MaxSet()
{
    delete ui;
}

void MaxSet::initMaxSet()
{
    ui->autoPush->setChecked(autoPush());
    ui->downloadOpenDir->setChecked(autoOpenDir());

    ui->pushToCache->setProperty("pushTo", PushTo::Cache);
    ui->pushToSrc->setProperty("pushTo", PushTo::Src);
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
}

void MaxSet::initMaxSetIni()
{
    if (!USERINFO->existUserIni(SetG, "autoPush")) {
        setAutoPush(true);
    }

    if (!USERINFO->existUserIni(SetG, "autoOpenDir")) {
        setAutoOpenDir(true);
    }

    if (!USERINFO->existUserIni(SetG, "pushTo")) {
        setPushTo(PushTo::Cache);
    }

    QString dir = cacheDir();
    if (!QDir(dir).exists()) {
        QDir().mkpath(dir);
    }
}

bool MaxSet::autoPush()
{
    return USERINFO->readUserIni(SetG, "autoPush").toBool();
}

void MaxSet::setAutoPush(bool v)
{
    USERINFO->saveUserIni(SetG, "autoPush", v);
}

bool MaxSet::autoOpenDir()
{
    return USERINFO->readUserIni(SetG, "autoOpenDir").toBool();
}

void MaxSet::setAutoOpenDir(bool v)
{
    USERINFO->saveUserIni(SetG, "autoOpenDir", v);
}

MaxSet::PushTo MaxSet::pushTo()
{
    return PushTo(USERINFO->readUserIni(SetG, "pushTo").toInt());
}

void MaxSet::setPushTo(MaxSet::PushTo v)
{
    USERINFO->saveUserIni(SetG, "pushTo", v);
}

QString MaxSet::cacheDir()
{
    return Set::cacheDir() + "/ConvertDownloads";
}
