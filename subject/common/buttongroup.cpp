#include "buttongroup.h"

#include <QVariant>
#include <QDebug>
ButtonGroup::ButtonGroup(QObject *parent) :
    QButtonGroup(parent)
{
    this->setExclusive(true);
    m_clickedId = -1;
}

void ButtonGroup::addButtons(QList<QPushButton *> buttonL, QStyle *style, QStackedWidget *stack)
{
    for (int i = 0; i < buttonL.length(); ++i) {
        this->addButton(buttonL.at(i), i);
    }
    void (QButtonGroup::*btnClick)(QAbstractButton *) = &QButtonGroup::buttonClicked;
    connect(this, btnClick, [=](QAbstractButton *button){
        int id = this->id(button);
        m_clickedId = id;
        QList<QAbstractButton*> btnL = this->buttons();
        for (int i = 0; i < btnL.length(); ++i) {
            QVariant var = btnL.at(i) != this->button(id) ? "0" : "1";
            btnL.at(i)->setProperty("tab", var);
            btnL.at(i)->setStyle(style);
        }
        if (stack)
            stack->setCurrentIndex(id);
        emit btnIdClick(id);
    });
}

int ButtonGroup::clickedId()
{
    return m_clickedId;
}
