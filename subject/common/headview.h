#ifndef HEADVIEW_H
#define HEADVIEW_H

#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>

class HeadView : public QHeaderView
{
    Q_OBJECT

public:
    enum HEADTYPE{
        SELECTALL,
        SORT,
        HELP
    };
public:
    explicit HeadView(Qt::Orientation orientation, QWidget *parent = 0);
    ~HeadView();

    void setHeadType(QMap<int, HEADTYPE> typeMap);
    void setSelectAll(int state);
    int selectAllState();
    int getCurrentOrderIndex();
    int getCurrentOrder();
    void resetHead();

    void setHeadLab(QStringList labL);
    bool checkIdx(int idx);

    QRect iconRect(const QRect& rect, int idx) const;

    void setSortArrow(bool sort);
signals:
    void headerClicked(int index, int orderby);
    void signalSelectAll(bool isAll);
    void helpHover(bool hover);
public slots:

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
    void mouseMoveEvent(QMouseEvent *e);
private:
    QMap<int/*idx*/, int/*state*/> m_headMap;
    QMap<int/*idx*/, HEADTYPE> m_typeMap;

    QList<int> m_sortList;
    int m_curIdx;
    int m_curSort;

    int m_selectState;//-1 disable, 0 noselect, 1 select

    QStringList m_labL;
    mutable QRect m_helpRect;

    bool m_sortArrow;
};

#endif // HEADVIEW_H
