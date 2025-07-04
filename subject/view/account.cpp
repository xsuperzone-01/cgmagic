#include "account.h"
#include "ui_account.h"
#include "common/session.h"
#include <QDesktopServices>
#include <QTimer>
#include <qmath.h>
#include "config/userinfo.h"
#include "view/set/set.h"

Account::Account(QWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::Account)
{
    ui->setupUi(this);
    {
        selfAdaptionFixedSize();
        selfAdaptionMargins();
        resizeChildrenMargin();
    }

    setCloseOnFocusOut();
    paintDropShadow = false;
    setDropShadow(ui->bodyWidget, 0, 0, 30, QColor(51, 54, 65, 77));
    setClass(ui->change, "textColor");
}


Account::~Account()
{
    delete ui;
}

void Account::setWidgetStatus(bool isEnable){
    this->setEnabled(isEnable);
    // ui->chargeBtn->setEnabled(isEnable);
}

void Account::setCloudTimes(int freeTimes, int nonFreeTimes){
    ui->timesA->setText(QString::number(freeTimes));
    ui->timesB->setText(QString::number(nonFreeTimes));
}

void Account::closeEvent(QCloseEvent *event){
    emit closeWidget();
}

void Account::initAccount(QString name, QJsonObject obj)
{
    ui->name->setText(name);
    int status = obj.value("status").toInt();
    QString endTime = obj.value("endTime").toString().replace("-", ".");
    if(status == 0){
        ui->pluginTime->setText(tr("您当前暂未开通授权哦~"));
        ui->chargeBtn->setText(tr("购买授权"));
        ui->vipTipMore->setText(tr("购买畅享专业版功能权益"));
        ui->vipIcon->hide();
    }else if(status == 1){
        ui->pluginTime->setText(QString("%1到期").arg(endTime));
        ui->vipTipMore->setText(tr("已购买畅享专业版功能权益"));
        ui->vipIcon->show();
        ui->vipIcon->setProperty("Vip", "1");
        ui->vipIcon->setStyle(qApp->style());
    }else{
        ui->pluginTime->setText(tr("您购买的授权已过期~"));
        ui->chargeBtn->setText(tr("授权续费"));
        ui->vipTipMore->setText(tr("购买畅享专业版功能权益"));
        ui->vipIcon->show();
        ui->vipIcon->setProperty("Vip", "2");
        ui->vipIcon->setStyle(qApp->style());
    }

    int renderTime = UserInfo::instance()->renderTime;
    double minutes = renderTime/60.0; // 将秒转换为分钟并保留一位小数并向上取整
    double upRound = std::ceil(minutes * 10)/10.0;
    QString result = QString::number(upRound, 'f', 1);

    QLocale locale = QLocale::system(); // 使用系统区域设置
    QString resultMinutes = locale.toString(result.toFloat(), 'f', 1);
    ui->label->setText(QString("剩余%1%2").arg(resultMinutes).arg(tr("分钟")));

    QString lan = Set::changeLan();
    if(lan == "en_us"){
        ui->horizontalSpacer_9->changeSize(13,20);
        ui->horizontalSpacer_10->changeSize(13,20);
    }

    setFocus();
}

void Account::paintEvent(QPaintEvent *)
{

}

bool Account::needMove(QMouseEvent *e)
{
    return false;
}

void Account::on_change_clicked()
{
    close();
    MsgBox *mb = MsgTool::msgChoose(tr("是否退出当前账号？"), Session::instance()->mainWid());
    mb->setBackgroundMask();
    connect(mb, &MsgBox::accepted, mb, [=]{
        Session::instance()->reLogin(Login::UserReLogin);
    });
}

void Account::on_person_clicked()
{
    close();
    QDesktopServices::openUrl(QUrl(USERINFO->instance()->openPersonalCenter()));
}

void Account::on_ac1_clicked()
{

}

void Account::on_ac2_clicked()
{

}

void Account::on_ac3_clicked()
{

}

void Account::on_ac4_clicked()
{
    close();
    QDesktopServices::openUrl(QUrl(USERINFO->instance()->openDetail()));
}

void Account::on_chargeBtn_clicked()
{
    close();
    emit send_recharge();
}
