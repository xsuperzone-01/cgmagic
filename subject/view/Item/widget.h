#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "common/BaseScale.h"

class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);

    void resizeChildrenMargin(QWidget *parent = NULL, QList<Widget *> list = QList<Widget *>());

    DECLARE_RESIZE();
    MARGINS_RESIZE();
signals:

protected:
    void paintEvent(QPaintEvent *event);

};

#endif // WIDGET_H
