#include "setupgrader.h"
#include "ui_setupgrader.h"

#include "../../version.h"
#include "common/basewidget.h"
#include "versions/versionmanager.h"

SetUpgrader::SetUpgrader(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SetUpgrader)
{
    ui->setupUi(this);

    BaseWidget::setClass(ui->bodyWidget, "dialogBlock");
    BaseWidget::setDropShadow(ui->bodyWidget, 0, 1, 4, QColor(0, 0, 0, 10));

    BaseWidget::setClass(ui->upgrade, "btn1");
    ui->detailWid->hide();
}

SetUpgrader::~SetUpgrader()
{
    delete ui;
}

void SetUpgrader::initSetUpgrader()
{
    ui->version->setText(QString("V%1").arg(CLIENT_VERSION));

    QPointer<VersionManager> vm = VersionManager::instance();
    QPointer<QObject> ptr = this;
    vm->check();
    if (!ptr || !vm)
        return;

    QString newVersion = vm->newVersion();
    if (newVersion.isEmpty() || newVersion == CLIENT_VERSION) {
        ui->newVersion->setText(tr("您目前使用的已经是最新版本。"));
    } else {
        ui->newVersion->setText(tr("检测到新版本V%1，是否需要升级到最新版本？").arg(newVersion));
        ui->detailWid->show();
        ui->textEdit->setText(vm->detail());
    }
}

void SetUpgrader::on_upgrade_clicked()
{
    VersionManager::instance()->popup();
}
