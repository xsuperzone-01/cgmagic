#ifndef CHECKBOXITEM_H
#define CHECKBOXITEM_H

#include "baseitemdelegate.h"

class CheckBoxItem : public NoFocusDelegate
{
    Q_OBJECT
public:
    CheckBoxItem(QObject *parent = 0);
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const;
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index);
};

#endif // CHECKBOXITEM_H
