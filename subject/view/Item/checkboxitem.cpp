#include "checkboxitem.h"

#include <QApplication>
#include <QPainter>
#include <QCheckBox>
#include <QMouseEvent>

CheckBoxItem::CheckBoxItem(QObject *parent) :
    NoFocusDelegate(parent)
{
}

void CheckBoxItem::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);
        QString ps = ":/uncheck.png";

        if (option.state & QStyle::State_MouseOver) {
            ps = ":/uncheckHover.png";
            if (option.state & QStyle::State_Selected) {
                ps = ":/check.png";
            }
        }
        if (option.state & QStyle::State_Selected)
            ps = ":/check.png";

        qApp->style()->drawItemPixmap(painter, option.rect, Qt::AlignCenter, QPixmap(ps).scaled(14, 14, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

bool CheckBoxItem::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
