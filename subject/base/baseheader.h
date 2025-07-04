#ifndef BASEHEADER_H
#define BASEHEADER_H

#include <QHeaderView>
#include <QPainter>
#include <QMouseEvent>

class BaseHeader : public QHeaderView {
    Q_OBJECT

public:
    explicit BaseHeader(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // BASEHEADER_H
