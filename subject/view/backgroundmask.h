#ifndef BACKGROUNDMASK_H
#define BACKGROUNDMASK_H

#include "common/basewidget.h"

namespace Ui {
class BackgroundMask;
}

class BackgroundMask : public BaseWidget
{
    Q_OBJECT

public:
    explicit BackgroundMask(QWidget *parent = nullptr);
    ~BackgroundMask();

    void setContentWidget(BaseWidget *wid);
    QWidget *background();

protected:
    void resizeEvent(QResizeEvent *event);
    void moveEvent(QMoveEvent *event);

private:
    void keepOneMask();
    void setSize(QWidget *parent);
    void setBlurMask();

private:
    Ui::BackgroundMask *ui;
    int firstMove;
    QPointer<BaseWidget> content;
    static QList<QPointer<BackgroundMask>> maskL;

};

#endif // BACKGROUNDMASK_H
