#include "traysetaccount.h"
#include "ui_traysetaccount.h"

#include "config/userinfo.h"
#include "common/session.h"

TraySetAccount::TraySetAccount(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TraySetAccount)
{
    ui->setupUi(this);

    ui->name->bindTextInteraction();
}

TraySetAccount::~TraySetAccount()
{
    delete ui;
}

void TraySetAccount::initTraySetAccount()
{
    ui->name->setText(USERINFO->userName());
    ui->money->setText(Session::instance()->mainWid()->money());

    ui->money->setToolTip(Session::instance()->mainWid()->getMoneyToolTipContent());
}
