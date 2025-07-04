#include "headview.h"

#include <QLabel>
#include "tool/xfunc.h"

HeadView::HeadView(Qt::Orientation orientation, QWidget *parent) :
    QHeaderView(orientation, parent),
    m_curIdx(-1),
    m_curSort(0),
    m_selectState(0),
    m_sortArrow(false)
{
    connect(this, &QHeaderView::sectionClicked, [=](int idx){
        if (!m_headMap.contains(idx))
            return;

        if (HeadView::SELECTALL == m_typeMap.value(idx)) {//select all
            if (-1 == m_selectState)
                return;
            m_selectState = 0 == m_selectState ? 1 : 0;
            emit signalSelectAll(1 == m_selectState ? true : false);
            setSelectAll(m_selectState);
        } else {
            int befor = m_headMap.value(idx);

            QList<int> upList = m_headMap.keys();
            for (int i = 0; i < upList.length(); ++i) {
                m_headMap.insert(upList.at(i), 0);
            }

            int after = 0 == befor ? 1 : 0 - befor;
            m_headMap.insert(idx, after);

            m_curIdx = idx;
            m_curSort = 1 == after ? 0 : 1;
            emit headerClicked(m_curIdx, m_curSort);

            for (int i = 0; i < upList.length(); ++i) {
                updateSection(upList.at(i));
            }
        }
    });
}

HeadView::~HeadView()
{

}

void HeadView::setHeadType(QMap<int, HeadView::HEADTYPE> typeMap)
{
    setSectionsClickable(true);
    QList<int> sortList = typeMap.keys();
    for (int i = 0; i < sortList.length(); ++i) {
        m_headMap.insert(sortList.at(i), 0);
    }
    m_typeMap = typeMap;

    this->updateSection(0);
}

void HeadView::setSelectAll(int state)
{
    qDebug()<<__FUNCTION__<<state;
    m_selectState = state;
    this->updateSection(0);
}

int HeadView::selectAllState()
{
    return m_selectState;
}

int HeadView::getCurrentOrderIndex()
{
    return m_curIdx;
}

int HeadView::getCurrentOrder()
{
    return m_curSort;
}

void HeadView::resetHead()
{
    QList<int> sortList = m_headMap.keys();
    for (int i = 0; i < sortList.length(); ++i) {
        m_headMap.insert(sortList.at(i), 0);
        this->updateSection(sortList.at(i));
    }
}

void HeadView::setHeadLab(QStringList labL)
{
    m_labL = labL;
}

bool HeadView::checkIdx(int idx)
{
    if (m_typeMap.contains(idx) && m_typeMap.value(idx) == SELECTALL)
        return true;
    return false;
}

QRect HeadView::iconRect(const QRect &rect, int idx) const
{
    QRect ret;
    if (!m_labL.isEmpty() && m_labL.length() > idx) {
        QString lab = m_labL.at(idx);
        QLabel label(lab);
        int w = label.sizeHint().width();
        ret = QRect(rect.x() + rect.width()/2 + w/2 + 10, rect.y() + (rect.height() - 10) / 2, 10, 10);
    } else
        ret = QRect(rect.x() + rect.width() / 3 * 2, rect.y() + 15, 10, 10);

    return ret;
}

void HeadView::setSortArrow(bool sort)
{
    m_sortArrow = sort;
}

void HeadView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    QHeaderView::paintSection(painter,rect,logicalIndex);
    painter->restore();

    QRect iRect = iconRect(rect, logicalIndex);
    if (!m_headMap.isEmpty()) {
        QString pixName;
        if (m_typeMap.contains(logicalIndex) && HeadView::SELECTALL == m_typeMap.value(logicalIndex)) {
            iRect = rect;
            if (0 == m_selectState)
                pixName = QString(":/uncheck.png");
            else if (1 == m_selectState)
                pixName = QString(":/check.png");
            else if (-1 == m_selectState)
                pixName = QString(":/uncheckDisabled.png");
        } else {
        }
        if (m_typeMap.contains(logicalIndex)) {
            if (HeadView::HELP == m_typeMap.value(logicalIndex)) {
                pixName = ":/rerentip.png";
                m_helpRect = iRect;
            }
        }
        if(!pixName.isEmpty())
        this->style()->drawItemPixmap(painter, iRect, Qt::AlignCenter, QPixmap(pixName).scaled(14, 14, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }

    if (m_sortArrow && this->sortIndicatorSection() == logicalIndex) {
        QString pix = this->sortIndicatorOrder() == Qt::AscendingOrder ? ":/arrowU2.png" : ":/arrowD2.png";
        this->style()->drawItemPixmap(painter, iRect, Qt::AlignCenter, QPixmap(pix));
    }
}

void HeadView::mouseMoveEvent(QMouseEvent *e)
{
    if (m_helpRect.isValid()) {
        emit helpHover(m_helpRect.contains(e->pos()));
    }

    return QHeaderView::mouseMoveEvent(e);
}
