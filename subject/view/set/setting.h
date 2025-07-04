#ifndef SETTING_H
#define SETTING_H

#include <QWidget>
#include "common/basewidget.h"
#include "common/pullmsg.h"
#include <QJsonArray>

class LeftNavItem;
class About;
class Set;
class MaxSet;
class RenderSet;
class UpdateSet;
class EnvSet;

namespace Ui {
class Setting;
}

class Setting : public BaseWidget
{
    Q_OBJECT

public:
    explicit Setting(QWidget *parent = nullptr);
    ~Setting();

    void setSetting();
    void setRender();
    void setMax();

    void NeedUpdate();

    void closeSetting();

    QPointer<MaxSet> maxSet();

signals:
    void needed();
    void neednot();


protected:
    bool event(QEvent *event);


private slots:
    void on_headClose_clicked();

private:
    Ui::Setting *ui;

    QPointer<LeftNavItem> setItem;
    QPointer<LeftNavItem> updateItem;
    QPointer<LeftNavItem> aboutItem;

//    QList<QPointer<LeftNavItem>> m_renderNavs;
//    QList<QPointer<LeftNavItem>> m_setNavs;

    QPointer<Set> m_set;
    QPointer<UpdateSet> m_update;
    QPointer<About> m_about;

    QPointer<RenderSet> m_render;
    QPointer<EnvSet> m_env;

    QPointer<MaxSet> m_max;

    int c = 0;
    int p = 0;
    bool n = false;

    void updateSetting();
};

#endif // SETTING_H
