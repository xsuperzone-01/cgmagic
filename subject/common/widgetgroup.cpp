#include "widgetgroup.h"

#include <QVariant>
#include <QDebug>
#include <QTimer>

widgetGroup::widgetGroup(QObject* parent):
    QObject(parent),
    m_currWidget(NULL)
{

}

widgetGroup::~widgetGroup()
{

}


void widgetGroup::addWidgets(QList<BaseClickWidget *> widL, QStyle *style, BaseClickWidget *current)
{
    m_widget.clear();
    if (widL.isEmpty())
        return;
    m_widget = widL;
    m_style = style;
    m_currWidget = current;

    for (int i = 0; i < m_widget.size(); i++) { //左侧栏四个窗口clicked信号对应widgetClicked槽函数
        connect(m_widget.at(i), SIGNAL(clicked(BaseClickWidget*)), this, SLOT(widgetClicked(BaseClickWidget*)));
    }

    // 由调用者决定是否点击
    if (NULL == current) {
    } else {
        emit current->clicked(current);
    }
}

//充值点击按钮，左侧窗口不显示状态
void widgetGroup::re_changeWidState(QList<BaseClickWidget*> re_widL){
    qDebug()<<"执行re_addwidgets:";
    m_widget.clear();
    if (re_widL.isEmpty())
        return;
    m_widget = re_widL;
    for (int i = 0; i < m_widget.size(); i++) {
        m_widget.at(i)->setSelected(false);   //循环遍历左侧每一个窗口的Select为false选项
    }
}


void widgetGroup::time_out()
{
    for (int i = 0; i < m_widget.size(); i++) {
        if (m_widget.at(i) && m_widget.at(i)->isEnabled()) {
            emit m_widget.at(i)->clicked(m_widget.at(i));
            break;
        }
    }
}

void widgetGroup::setAllSelected(bool select)
{
    for (int i = 0; i < m_widget.size(); i++) {
        m_widget.at(i)->setSelected(select);
    }

    if (!select)
        m_currWidget = NULL;
}

void widgetGroup::setAllSelectedEnable(bool enable)
{        
    for (int i = 0; i < m_widget.size(); i++) {
        m_widget.at(i)->setSelectEnable(enable);
    }
}

//左侧栏四个窗口clicked信号触发对应的槽函数
void widgetGroup::widgetClicked(BaseClickWidget* wid)
{
    if (!wid->restyle)
        return;

    QMutexLocker locker(&m_mutex);
    if (wid == NULL)
        qDebug() << "6666666666666";
    for (int i = 0; i < m_widget.size(); i++) {
        if (wid == m_widget.at(i)) {
            if (m_currWidget && wid == m_currWidget)
            {
                m_widget.at(i)->setSelected(true);
                m_widget.at(i)->retInfo(false);
            }
            else {
                m_widget.at(i)->setSelected(true);
                m_widget.at(i)->retInfo(false);
                m_currWidget = m_widget.at(i);
            }
        } else {
            m_widget.at(i)->setSelected(false);
        }
        m_widget.at(i)->setStyle(m_style);
    }
}
