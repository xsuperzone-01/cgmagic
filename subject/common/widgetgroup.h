#ifndef WIDGETGROUP_H
#define WIDGETGROUP_H

#include <QObject>
#include <QPointer>
#include <QStyle>
#include "common/baseclickwidget.h"
#include <QMutex>

class widgetGroup : public QObject
{
    Q_OBJECT
public:
    explicit widgetGroup(QObject* parent = 0);
    ~widgetGroup();

    void addWidgets(QList<BaseClickWidget*> widL, QStyle* style, BaseClickWidget *current = NULL);

    void setAllSelected(bool select);

    void setAllSelectedEnable(bool enable);

private slots:
    void widgetClicked(BaseClickWidget* curr_clickWid);
    void time_out();


private:
    QList<BaseClickWidget*> m_widget;
    QStyle* m_style;
    QPointer<BaseClickWidget> m_currWidget;
    QList<BaseClickWidget*> re_widget;
    QMutex m_mutex;

public:
    void re_changeWidState(QList<BaseClickWidget*> re_widL);

};

#endif // WIDGETGROUP_H
