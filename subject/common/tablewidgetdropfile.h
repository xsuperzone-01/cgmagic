#ifndef TABLEWIDGETDROPFILE_H
#define TABLEWIDGETDROPFILE_H

#include <QTableWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include "common/session.h"
#include <QDir>
#include <QFileInfoList>
#include <QDebug>
#include <QUrl>
#include <QtGui>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QTime>
#include <QHeaderView>
#include <QScrollBar>
#include <QApplication>

#include "view/Item/baseitemdelegate.h"

#include "headview.h"
class LeftSpace : public NoFocusDelegate
{
    Q_OBJECT
public:
    LeftSpace(){}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
protected:
    virtual void initStyleOption(QStyleOptionViewItem *option,
                                const QModelIndex &index) const;
};

class MidIcon : public NoFocusDelegate
{
    Q_OBJECT
public:
    MidIcon(){}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
protected:
    virtual void initStyleOption(QStyleOptionViewItem *option,
                                const QModelIndex &index) const;
};

#include <QDrag>
#include <QMimeData>
#include "view/pagination.h"
#include "common/debounce.h"

class TableWidgetDropFile : public QTableWidget
{
    Q_OBJECT
public:
    explicit TableWidgetDropFile(QWidget *parent = 0);
    ~TableWidgetDropFile();

    void setRowColor(int row, int type);
    int insertOneRow();
    void removeOneRow(int row, bool notice = false);
    void removeAllRow();
    void enableAutoColor();
    void enableCellEnter();
    void setHeadSize(QStringList lab, QList<int> sl);
    void setHeadView();
    void setHeadLabels(QStringList labL);
    QStringList headLabels();
    HeadView* headView();
    QList<int> selectedRow();
    QString calCoreDivFrame(int core, int frame);
    QString timeStr(int sec);
    QString timeStr(QString time, int type);

    void resizeCol();

    int page();

    void multiSel();

    void setLeave();

    void reselectRow(QList<int> rowL);
    void calSelect();
    void setPreSel(QList<int> selL);
    QList<int> preSelRow();

    bool isDiffQuery(QJsonObject obj);

    void bindPage(Pagination* page);

    void setBackgroundImg(QString img);

    void setFastItem(QString key, QTableWidgetItem* item);
    void setFastItem(QTableWidgetItem* item, QString key);

    void selectAllRow(bool isAll);
    void setHHeaderLabels(QStringList labs);
    void setCheckBox(int row, int type);

    void store(int role, int column = 0);
    void select(int row, QVariant id);
    void restore();
    void clear();
    QList<QTableWidgetItem *> selectedItemsByColumn(int column = 0);

signals:
    void signalCheckBoxSelect(int row, bool select);
    void rowCountView();
    void rowCountView2(int);
    void wheel(bool down = false);
    void pageChanged(int page);
    void sigLeaveEvent();
    void calSel();
private:
    void cellEnterHand(int row);
    int headSecSize(int idx);
public slots:
    void setPage(int pa);
    int fastItem(QString key, int last = 0);
    int fastItemRm(QString key, int child = 0);
protected:
    void wheelEvent(QWheelEvent *event);
    void mouseMoveEvent(QMouseEvent* event);
    void leaveEvent(QEvent *);
private:
    int m_lastEnterRow;

    QTime m_wheelStamp;//降低滚轮灵敏度

    QList<int> m_headSize;
    HeadView* m_head;
    QStringList m_headLabel;
    QPair<int, QList<int> > m_selectP;
    QTimer m_selectTimer;

    int m_verScroll;
    int m_horScroll;
    bool m_recordScroll;
    QJsonObject m_queryObj;

    QString m_bgImg;

    QHash<QString, QTableWidgetItem*> m_fastItemH;
    QHash<QTableWidgetItem*, QString> m_fastItemH2;

    Debounce m_selectionDebounce;

    QList<int> m_lastClickRow;
    bool m_isSelectAll;

    QList<QVariant> m_selectedIds;
    int m_hScrollBarValue;
    int m_vScrollBarValue;

public:
    int m_page;
    Pagination* m_paget;
};

#endif // TABLEWIDGETDROPFILE_H
