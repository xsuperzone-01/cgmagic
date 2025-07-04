#include "progressitem.h"

#include <QApplication>
#include <QProgressBar>
#include <QPainter>

ProgressItem::ProgressItem(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void ProgressItem::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    bool show = true;
    if (index.data(Qt::UserRole + 2).isValid()) {
        show = index.data(Qt::UserRole + 2).toBool();
    }
    if (show) {
        int progress = index.data(Qt::UserRole).toInt();
        QString text = index.data(Qt::UserRole + 1).toString();

        QStyleOptionProgressBar pbo;
        pbo.rect = QRect(option.rect.left()+(option.rect.width()-100)/2, option.rect.top()+(option.rect.height()-14)/2, 100, 14);
        pbo.minimum = 0;
        pbo.maximum = 100;
        pbo.progress = progress;

        QProgressBar pb;
        qApp->style()->drawControl(QStyle::CE_ProgressBar,
                                           &pbo, painter, &pb);

        painter->setPen(QColor(text.length() < 5 && progress > 50 ? "#fff" : "#000"));
        if (text == "")
            painter->drawText(option.rect, QString("%1%").arg(progress), QTextOption(Qt::AlignCenter));
        else
            painter->drawText(option.rect, QString("%1").arg(text), QTextOption(Qt::AlignCenter));

    }
}
