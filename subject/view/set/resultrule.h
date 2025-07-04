#ifndef RESULTRULE_H
#define RESULTRULE_H

#include "common/basewidget.h"

#include <QButtonGroup>

namespace Ui {
class ResultRule;
}

class ResultRule : public BaseWidget
{
    Q_OBJECT

public:
    enum {
        project = 0,
        submitTime,
        orderNum
    };

    explicit ResultRule(QWidget *parent = 0);
    ~ResultRule();

    void initResultRule(QString rule);
signals:
    void resultRule(QString rule);
private slots:
    void on_submit_clicked();
    void checkChanged(QAbstractButton* cb);
private:
    Ui::ResultRule *ui;

    QButtonGroup m_bg;
    QStringList m_selCheck;
};

#endif // RESULTRULE_H
