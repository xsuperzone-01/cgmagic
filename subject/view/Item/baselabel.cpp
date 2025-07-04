#include "baselabel.h"
#include <QDebug>

#include <QApplication>
#include "common/basewidget.h"

BaseLabel::BaseLabel(QWidget *parent) :
    QLabel(parent),
    m_hover(false),
    m_mouse_press(false),
    textInteraction(false),
    textClick(false)
{

}

DEFINE_RESIZE(BaseLabel, QLabel);

void BaseLabel::bindTextInteraction()
{
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    textInteraction = true;
    this->installEventFilter(this);
}

void BaseLabel::setText(const QString &text)
{
    if (textInteraction) {
        BaseWidget::setLabelText(this, text);
    } else {
        QLabel::setText(text);
    }
}

bool BaseLabel::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this) {
        if (event->type() == QEvent::Enter) {
            if (!m_hover) {
                m_hover = true;
                emit pressHover(true);
                emit enter();
            }
        } else if (event->type() == QEvent::Leave) {
            if (m_hover) {
                emit pressHover(false);
                m_hover = false;
                emit leave();
            }
            if (textInteraction && textClick) {
                textClick = false;
                BaseWidget::setLabelText(this, toolTip());
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (textInteraction) {
                setText2(toolTip());
                textClick = true;
            }
        }
    }

    return QLabel::eventFilter(obj, event);
}

void BaseLabel::setText2(const QString &text)
{
    QLabel::setText(text);
}



