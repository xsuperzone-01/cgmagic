#ifndef BASECLICKWIDGET_H
#define BASECLICKWIDGET_H

#include "view/Item/widget.h"

class BaseClickWidget : public Widget
{
    Q_OBJECT
public:
    explicit BaseClickWidget(QWidget *parent = 0);
    ~BaseClickWidget();

    virtual void setSelected(bool selected);

    virtual void setSelectEnable(bool enable);

    virtual void retInfo(bool isEmpty);

    bool restyle;

signals:
    void clicked(BaseClickWidget*);
    void ret(const QJsonObject);

protected:
    void mousePressEvent(QMouseEvent *ev);

    void mouseReleaseEvent(QMouseEvent *ev);

    QJsonObject jsonObject;

private:
    bool mousePressed;
    bool m_selected;
};

#endif // BASECLICKWIDGET_H
