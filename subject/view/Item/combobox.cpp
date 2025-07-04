#include "combobox.h"

#include <QListView>
#include <QDebug>
#include <QApplication>
#include <QGraphicsEffect>
#include "common/basewidget.h"
#include <QScrollBar>
#include <QTimer>

ComboBox::ComboBox(QWidget *parent) : QComboBox(parent)
{
    setView(new QListView());
    view()->window()->setWindowFlags(Qt::Popup|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint);
    view()->window()->setAttribute(Qt::WA_TranslucentBackground);
}

void ComboBox::wheelEvent(QWheelEvent *e)
{

}

void ComboBox::showPopup()
{
    QComboBox::showPopup();
    ComboBox::setPadding(this);
    this->setStyleSheet(qApp->styleSheet());
    return;

    emit getLists();
    QComboBox::showPopup();
    //避免下拉框出现空白条目
    this->setStyleSheet(qApp->styleSheet());

    //mac下默认下拉会遮挡ComboBox
#ifdef Q_OS_MAC
    QPoint pos = this->mapToGlobal(QPoint(0, 0));
    view()->window()->move(pos.x(), pos.y() + this->height());
    view()->window()->setFixedWidth(this->width());
#endif
}

void ComboBox::hidePopup()
{
    this->setGraphicsEffect(NULL);
    QComboBox::hidePopup();
    return;
}

/*
    清空除第一行
*/
void ComboBox::clearData()
{
    blockSignals(true);
    while (count() > 1) {
        removeItem(count() - 1);
    }
    blockSignals(false);
}

void ComboBox::setPadding(QComboBox *cb)
{
    if (cb->model()->rowCount() > cb->maxVisibleItems()) {
        BaseWidget::setProperty(cb, "itemViewPaddingRight", true);
    }
}
