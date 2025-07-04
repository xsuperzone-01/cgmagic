#include "baseheader.h"
#include <QStyleOptionButton>
#include <QStyle>

BaseHeader::BaseHeader(Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent){
}

void BaseHeader::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const {
    QHeaderView::paintSection(painter, rect, logicalIndex);
}

void BaseHeader::mousePressEvent(QMouseEvent *event) {
    QHeaderView::mousePressEvent(event);
}
