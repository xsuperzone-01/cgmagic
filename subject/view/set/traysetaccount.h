#ifndef TRAYSETACCOUNT_H
#define TRAYSETACCOUNT_H

#include <QWidget>

namespace Ui {
class TraySetAccount;
}

class TraySetAccount : public QWidget
{
    Q_OBJECT

public:
    explicit TraySetAccount(QWidget *parent = nullptr);
    ~TraySetAccount();

    void initTraySetAccount();

private:
    Ui::TraySetAccount *ui;
};

#endif // TRAYSETACCOUNT_H
