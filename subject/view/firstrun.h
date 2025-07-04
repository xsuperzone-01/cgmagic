#ifndef FIRSTRUN_H
#define FIRSTRUN_H

#include "common/basewidget.h"

namespace Ui {
class FirstRun;
}

class FirstRun : public BaseWidget
{
    Q_OBJECT

public:
    explicit FirstRun(QWidget *parent = nullptr);
    ~FirstRun();

    bool needMove(QMouseEvent *e);

private:
    void loadImage();
    void setText(QString a, QString b, QString c, int i, int j, int k, int l);
    void showPaint();

private:
    Ui::FirstRun *ui;

    int m_index;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

};

#endif // FIRSTRUN_H
