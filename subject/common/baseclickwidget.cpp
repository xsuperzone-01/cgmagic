#include "baseclickwidget.h"

BaseClickWidget::BaseClickWidget(QWidget *parent):
    Widget(parent),
    restyle(true)
{

}

BaseClickWidget::~BaseClickWidget()
{

}

void BaseClickWidget::setSelected(bool selected)
{

}

void BaseClickWidget::setSelectEnable(bool enable)
{

}

void BaseClickWidget::retInfo(bool retInfo)
{
    if (retInfo)
        emit ret(QJsonObject());
    else
        emit ret(jsonObject);
}

void BaseClickWidget::mousePressEvent(QMouseEvent *ev)
{
    mousePressed = true;
}

void BaseClickWidget::mouseReleaseEvent(QMouseEvent *ev)
{
    if (mousePressed) {
        emit clicked(this);
        mousePressed = false;
    }
}

