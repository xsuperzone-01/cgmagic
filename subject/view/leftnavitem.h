#ifndef LEFTNAVITEM_H
#define LEFTNAVITEM_H

#include <QWidget>
#include <QPushButton>
#include "common/baseclickwidget.h"

#include "view/Item/baselabel.h"
class LeftSelectEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit LeftSelectEventFilter(QObject *parent = nullptr);

    void bindEvent(QWidget *wid1, BaseLabel *lab1);

protected:
    bool eventFilter(QObject* obj, QEvent *event);

private:
    QPointer<QWidget> w1;
    QPointer<BaseLabel> l1;
};

class WidgetPressedEventFilter : public QObject
{
public:
    explicit WidgetPressedEventFilter(QObject *parent = nullptr);

    void bindEvent(QWidget *wid1);
    void bindEvent(QWidget *wid1, QList<QWidget *> pressedL);

protected:
    bool eventFilter(QObject* obj, QEvent *event);

private:
    QPointer<QWidget> w1;
    QList<QPointer<QWidget>> pL;
};

namespace Ui {
class LeftNavItem;
}

class LeftNavItem : public BaseClickWidget
{
    Q_OBJECT

public:
    explicit LeftNavItem(QWidget *parent = nullptr);
    ~LeftNavItem();

    void initLeftNavItem(QString text, int type = -1, bool update = false);
    void showDown(bool show = true);

    void hideSelect();
    void hideIcon();
    void setIconType(QString type);
    void setLeftMargin(int left);

    void setSelected(bool selected);
    void setleftStyle(int a);

    QPushButton *downBtn();

    void setAllClass(QString c, QVariant v);

signals:
    void hovered();

public slots:
    void leave();

protected:
    bool event(QEvent *event);

private:
    Ui::LeftNavItem *ui;
    int i = 0;
    QPointer<QTimer> count;
};

#endif // LEFTNAVITEM_H
