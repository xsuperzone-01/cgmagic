#include "leftnavitem.h"
#include "ui_leftnavitem.h"

#include "common/basewidget.h"
#include <QTimer>
#include <QDebug>
#include <QStyle>

LeftNavItem::LeftNavItem(QWidget *parent) :
    BaseClickWidget(parent),
    ui(new Ui::LeftNavItem)
{
    ui->setupUi(this);
    {
        selfAdaptionFixedHeight();
        resizeChildrenMargin();
    }

    ui->widget->setAttribute(Qt::WA_Hover);
    BaseWidget::setClass(ui->select, "leftSelect");
    ui->down->hide();
    ui->update->hide();
    setAllClass("tab", "0");
}

LeftNavItem::~LeftNavItem()
{
    delete ui;
}

void LeftNavItem::initLeftNavItem(QString text, int type, bool update)
{
    ui->text->setText(text);


    if (type > -1)
        BaseWidget::setProperty(ui->icon, "type", QString::number(type));

    LeftSelectEventFilter *ef = new LeftSelectEventFilter(this);
    ef->bindEvent(this, ui->select);
    WidgetPressedEventFilter *ef2 = new WidgetPressedEventFilter(this);
    ef2->bindEvent(ui->widget);

    if (update) {
        ui->update->show();
    }else {
        ui->update->hide();
    }

}

void LeftNavItem::showDown(bool show)
{
    if (!show) {
        ui->down->hide();
        return;
    }

    ui->down->show();
    if (ui->down->property("type").isNull()) {
        connect(ui->down, &QPushButton::clicked, this, [=]{
            QString t = ui->down->property("type").toString();
            if ("up" == t)
                t = "down";
            else
                t = "up";
            BaseWidget::setProperty(ui->down, "type", t);
        });
    }

    BaseWidget::setProperty(ui->down, "type", "up");
}

void LeftNavItem::hideSelect()
{
    ui->select->hide();
}

void LeftNavItem::hideIcon()
{
    ui->icon->hide();
}

void LeftNavItem::setIconType(QString type)
{
    BaseWidget::setProperty(ui->icon, "type", type);
}

void LeftNavItem::setLeftMargin(int left)
{
    QLayout *lay = ui->widget_2->layout();
    QMargins m = lay->contentsMargins();
    m.setLeft(left);
    lay->setContentsMargins(m);
}

void LeftNavItem::setSelected(bool selected)
{
    if (!restyle)
        return;
    if(count){
        count->deleteLater();
    }
    QString tab = selected ? "1" : "0";
    i = 0;
    if(selected == true){
        count = new QTimer(this);
        setAllClass("tab", tab);//tab=1
        int a = ui->icon->property("type").toString().toInt();
        setleftStyle(a);
        connect(count, &QTimer::timeout, this, [=](){
            int a = ui->icon->property("type").toString().toInt();
            setleftStyle(a);
            i++;
            if(i == 24){
                count->deleteLater();
            }
        });
        if(i == 0){
        count->start(42);
        }
    }else{
        setAllClass("tab", tab);//tab=0
    }

}

void LeftNavItem::setleftStyle(int a)
{
    QString b;
    switch (a) {
    case 1:
        b = "changeMax";
        break;
    case 2:
        b = "changeMaterial";
        break;
    case 3:
        b = "download";     //下载云转材质
        break;
    case 5:
        b = "manage";
        break;
    case 6:
        b = "upload";
        break;
    case 7:
        b = "download";
        break;
    case 9:
        b = "shopcenter";
        break;
    default:
        break;
    }
    ui->icon->setStyleSheet(QString("LeftNavItem #icon[type='%1'][tab='1'] {border-image: url(:/leftItem/%2 (%3).png);}")
                            .arg(a).arg(b).arg(i));
}

QPushButton *LeftNavItem::downBtn()
{
    return ui->down;
}

void LeftNavItem::setAllClass(QString c, QVariant v)
{
    BaseWidget::setProperty(ui->widget, c, v);
    if (!ui->select->isHidden())
        BaseWidget::setProperty(ui->select, c, v);
    BaseWidget::setProperty(ui->text, c, v);
    BaseWidget::setProperty(ui->icon, c, v);
}

void LeftNavItem::leave()
{
    if (isEnabled()) {
        setAllClass("hover", false);
    }
}

bool LeftNavItem::event(QEvent *event)
{
    if (event->type() == QEvent::Enter) {
        if (isEnabled()) {
            setAllClass("hover", true);
        }
        emit hovered();
    } else if (event->type() == QEvent::Leave) {
        leave();
    }

    return BaseClickWidget::event(event);
}

LeftSelectEventFilter::LeftSelectEventFilter(QObject *parent)
{

}

void LeftSelectEventFilter::bindEvent(QWidget *wid1, BaseLabel *lab1)
{
    w1 = wid1;
    l1 = lab1;
    wid1->installEventFilter(this);
}

bool LeftSelectEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == w1) {
        if (event->type() == QEvent::MouseButtonPress) {
            l1->setFixedHeight(10);
            BaseWidget::setProperty(l1, "tab", "1");//左边那根线
        } else if (event->type() == QEvent::MouseButtonRelease) {
            l1->setFixedHeight(20);
        }
    }
    return QObject::eventFilter(obj, event);
}

WidgetPressedEventFilter::WidgetPressedEventFilter(QObject *parent)
{

}

void WidgetPressedEventFilter::bindEvent(QWidget *wid1)
{
    w1 = wid1;
    wid1->installEventFilter(this);
}

void WidgetPressedEventFilter::bindEvent(QWidget *wid1, QList<QWidget *> pressedL)
{
    w1 = wid1;
    wid1->installEventFilter(this);
    foreach (QWidget *wid, pressedL) {
        wid->installEventFilter(this);
        pL << wid;
    }
}

bool WidgetPressedEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == w1 || pL.contains((QWidget *)obj)) {
        if (event->type() == QEvent::MouseButtonPress) {
            BaseWidget::setProperty(w1, "pressed", "1");//窗口按压效果，已废弃。
        } else if (event->type() == QEvent::MouseButtonRelease) {
            BaseWidget::setProperty(w1, "pressed", "0");
        }
    }
    return QObject::eventFilter(obj, event);
}
