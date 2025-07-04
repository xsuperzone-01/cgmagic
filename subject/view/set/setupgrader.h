#ifndef SETUPGRADER_H
#define SETUPGRADER_H

#include <QWidget>

namespace Ui {
class SetUpgrader;
}

class SetUpgrader : public QWidget
{
    Q_OBJECT

public:
    explicit SetUpgrader(QWidget *parent = nullptr);
    ~SetUpgrader();

    void initSetUpgrader();

private slots:
    void on_upgrade_clicked();

private:
    Ui::SetUpgrader *ui;
};

#endif // SETUPGRADER_H
