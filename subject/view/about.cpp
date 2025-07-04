#include "about.h"
#include "ui_about.h"

#include <QDateTime>
#include <QDebug>
#include <QToolTip>
#include "version.h"
#include "common/basewidget.h"
#include "versions/versionmanager.h"
#include "view/set/set.h"
#include "QDesktopServices"

About::About(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);

    QString lan = Set::changeLan();
    if (lan.isEmpty()) {
        lan = "zh_cn";
    }
    if (lan == "en_us") {
        ui->copyright->hide();
    }
    qDebug()<<__func__<<lan;
}

About::~About()
{
    delete ui;
}

void About::initAbout()
{
    ui->copyright->setText(tr("江苏赞奇科技股份有限公司 版权所有"));
    ui->copyrightEn->setText("Copyright © 2017-2024 XSUPERZONE All Rights Reserved.");

    VersionManager *vm = VersionManager::instance();
    QString mt = QString(" (%2)").arg(vm->clientTime(vm->client()));
    if (vm->newVersion() != CLIENT_VERSION)
        mt = "";
    ui->version->setText(tr("客户端版本  %1%2").arg(CLIENT_VERSION).arg(mt));

    connect(ui->mainUrl, &QPushButton::clicked, this, [=](){
        QDesktopServices::openUrl(QUrl("https://www.xrender.com/v23/product/CgMagicProduct"));
    });
    connect(ui->docUrl, &QPushButton::clicked, this, [=](){
        QDesktopServices::openUrl(QUrl("http://help.xrender.com/xmax/quickstar"));
    });
    ui->wxUrl->setText(tr("微信公众号"));
}
