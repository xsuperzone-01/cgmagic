#ifndef BASEITEMDELEGATE_H
#define BASEITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPushButton>
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>

class NoFocusDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    NoFocusDelegate(QObject *parent = 0) : QStyledItemDelegate(parent) {}
    ~NoFocusDelegate(){}

protected:
    virtual void initStyleOption(QStyleOptionViewItem *option,
                                const QModelIndex &index) const {
        QStyledItemDelegate::initStyleOption(option, index);
        if (option->state & QStyle::State_HasFocus) {
            option->state = option->state ^ QStyle::State_HasFocus;
        }
    }
};

class BaseItemDelegate : public NoFocusDelegate
{
    Q_OBJECT
public:
    BaseItemDelegate(QObject *parent = 0);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const;
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index);

    void drawCEButton(QPainter *painter, const QStyleOptionViewItem &option) const;

    virtual QList<QStyleOptionButton> rectOpt(QRect oR) const = 0;

    QList<QStyleOptionButton> sameBtn(QRect oR) const;

    void enableBtn() const;
signals:
    void rePos() const;
    void baseItemHand(int idx, QModelIndex index);
private slots:
    void remPos();
protected:
    QPoint m_mousePos;
    QList<QPushButton*> m_btnL;
    QString m_style;
};

#endif // BASEITEMDELEGATE_H
