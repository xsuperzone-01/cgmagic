#include "baseitemdelegate.h"

#include <QToolTip>

BaseItemDelegate::BaseItemDelegate(QObject *parent) :
    NoFocusDelegate(parent)
{
    m_mousePos = QPoint(0, 0);
    m_style = "QPushButton{border:0px;background-repeat:notrepeat;background-position:center;background-color:transparent;}";

    connect(this, SIGNAL(rePos()), this, SLOT(remPos()));
}

void BaseItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

}

bool BaseItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    bool rePaint = false;

    QMouseEvent *me = static_cast<QMouseEvent*>(event);
    m_mousePos = me->pos();
    QApplication::restoreOverrideCursor();

    QList<QStyleOptionButton> optL = rectOpt(option.rect);

    for (int i = 0; i < optL.length(); ++i) {
        QRect rect = optL.at(i).rect;
        if (!rect.contains(m_mousePos))
            continue;

        rePaint = true;
        if (me->type() == QEvent::MouseMove) {
            QString text = m_btnL.at(i)->toolTip();
            if (!text.isEmpty())
                QToolTip::showText(me->globalPos(), text);
        } else
            QToolTip::hideText();

        if (m_btnL.at(i)->isEnabled()) {
            if (me->type() == QEvent::MouseMove) {
                qApp->setOverrideCursor(Qt::PointingHandCursor);
            }
            if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
                emit baseItemHand(i, index);
            }
        }
    }
    return rePaint;
}

void BaseItemDelegate::drawCEButton(QPainter *painter, const QStyleOptionViewItem &option) const
{
    QList<QStyleOptionButton> optL = rectOpt(option.rect);
    for (int i = 0; i < optL.length(); ++i) {
        QStyleOptionButton opt = optL.at(i);
        if (opt.rect.isEmpty())
            continue;
        if (m_btnL.at(i)->isEnabled())
            opt.state |= QStyle::State_Enabled;
        if (opt.rect.contains(m_mousePos))
            opt.state |= QStyle::State_MouseOver;
        opt.text = m_btnL.at(i)->text();
        qApp->style()->drawControl(QStyle::CE_PushButton, &opt, painter, m_btnL.at(i));
    }
}

QList<QStyleOptionButton> BaseItemDelegate::sameBtn(QRect oR) const
{
    int num = 0;
    foreach (QPushButton* btn, m_btnL) {
        if (btn->property("bshow").toInt())
            num++;
    }

    int wh = 25;
    int space = 10;
    QSize all = QSize(num * wh + (num - 1) * space, wh);

    int x = oR.left() + (oR.width() - all.width()) / 2;
    int y = oR.top() + (oR.height() - all.height()) / 2;

    QList<QStyleOptionButton> optL;
    for (int i = 0; i < m_btnL.length(); ++i) {
        QStyleOptionButton opt;
        if (m_btnL.at(i)->property("bshow").toInt()) {
            opt.rect = QRect(x, y, wh, wh);
            x += wh + space;
        }

        optL << opt;
    }

    return optL;
}

void BaseItemDelegate::enableBtn() const
{
    foreach (QPushButton* btn, m_btnL) {
        btn->setEnabled(true);
        btn->setStyleSheet(m_style);
        btn->setProperty("bshow", "1");
    }
}

void BaseItemDelegate::remPos()
{
    m_mousePos.setX(0);
    m_mousePos.setY(0);
}
