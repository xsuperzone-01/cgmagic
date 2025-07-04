#include "widget.h"

#include <QPainter>
#include <qstyleoption.h>
#include "config/userinfo.h"

Widget::Widget(QWidget *parent) : QWidget(parent)
{

}

//margin统一处理
void Widget::resizeChildrenMargin(QWidget *parent, QList<Widget *> list)
{
    if (parent == NULL) {
        parent = this;
    }
    if (list.isEmpty()) {
        list = parent->findChildren<Widget *>();
    }

    foreach (Widget *wid, list) {
        wid->selfAdaptionMargins();
        resizeChildrenMargin(wid);
    }
}

DEFINE_RESIZE(Widget, QWidget);
MARGINS_RESIZE_NAME(Widget);

void Widget::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}
