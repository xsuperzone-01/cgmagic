#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "common/basewidget.h"
#include <QCloseEvent>

namespace Ui {
class Account;
}

class Account : public BaseWidget
{
    Q_OBJECT

public:
    explicit Account(QWidget *parent = nullptr);
    ~Account();

    void initAccount(QString name, QJsonObject obj);

    bool needMove(QMouseEvent *e);

    void setCloudTimes(int freeTimes, int nonFreeTimes);

    void setWidgetStatus(bool isEnable);

protected:
    void paintEvent(QPaintEvent *);
    void closeEvent(QCloseEvent *event);

private slots:
     void on_change_clicked();

     void on_person_clicked();

     void on_ac1_clicked();

     void on_ac2_clicked();

     void on_ac3_clicked();

     void on_ac4_clicked();

     void on_chargeBtn_clicked();

signals:
     void send_renewal();
     void send_recharge();
     void closeWidget();    //账户窗口关闭

private:
    Ui::Account *ui;
};

#endif // ACCOUNT_H
