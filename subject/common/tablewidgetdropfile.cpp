#include "tablewidgetdropfile.h"

#include <stdio.h>
#include <QDebug>

TableWidgetDropFile::TableWidgetDropFile(QWidget *parent) :
    m_lastEnterRow(-1),
    m_wheelStamp(QTime::currentTime()),
    m_page(0),
    m_head(NULL),
    m_verScroll(0),
    m_horScroll(0),
    m_recordScroll(true),
    m_paget(NULL),
    m_isSelectAll(false),
    QTableWidget(parent)
{
    this->verticalHeader()->setDefaultSectionSize(40);
    this->setWordWrap(false);
    this->setFrameShape(QFrame::NoFrame);
    this->setMouseTracking(true);//开启捕获鼠标功能
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);//禁止编辑表格

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    this->setShowGrid(false);
    this->setItemDelegate(new NoFocusDelegate());
    this->verticalHeader()->hide();//隐藏纵向数字列

    this->horizontalHeader()->setHighlightSections(false);//禁止高亮表头
}

TableWidgetDropFile::~TableWidgetDropFile()
{
}


void TableWidgetDropFile::setRowColor(int row, int type)
{
}

int TableWidgetDropFile::insertOneRow()
{
    if (rowCount() == 1 && columnCount() == columnSpan(0, 0)) {
        removeOneRow(0, true);
        emit rowCountView();
    }
    int row = rowCount();
    insertRow(row);
    if (0 == row)
        emit rowCountView();
    return row;
}

//源码显示 item已经被delete
void TableWidgetDropFile::removeOneRow(int row, bool notice)
{
    if (row < 0 || row >= this->rowCount())
        return;
    for (int i = 0; i < this->columnCount(); ++i) {
        if (this->cellWidget(row, i))
            this->removeCellWidget(row, i);
    }
    this->removeRow(row);

    if (!notice && rowCount() == 0)
        emit rowCountView();
}

void TableWidgetDropFile::removeAllRow()
{
    m_fastItemH.clear();
    m_fastItemH2.clear();

    while (this->rowCount() != 0)
        removeOneRow(0, true);
    emit rowCountView();
}

void TableWidgetDropFile::enableAutoColor()
{
    enableCellEnter();
}

void TableWidgetDropFile::enableCellEnter()
{
    connect(this, &QTableWidget::cellEntered, [=](int row, int col){
        cellEnterHand(row);
    });
}

void TableWidgetDropFile::setHeadSize(QStringList lab, QList<int> sl)
{
    if (lab.length() != sl.length())
        return;
    m_headSize = sl;
    setColumnCount(lab.length());
    for (int i = 0; i < lab.length(); ++i) {
        if (sl.at(i) < 0){
            this->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
        }
        setColumnWidth(i, headSecSize(i));
    }
    setHeadLabels(lab);
}

void TableWidgetDropFile::setHeadView()
{
    m_head = new HeadView(Qt::Horizontal, this);
    this->setHorizontalHeader(m_head);
    this->horizontalHeader()->setStretchLastSection(true);

    connect(m_head, &HeadView::signalSelectAll, [=](bool b){
        qDebug()<<__FUNCTION__<<b;
        if (b) {
            if (this->rowCount()) {
                this->selectAll();
                emit this->verticalScrollBar()->valueChanged(this->verticalScrollBar()->value());
            } else
                emit this->itemSelectionChanged();
        } else
            this->clearSelection();
    });
    connect(this, &QTableWidget::cellEntered, [=](int row, int col){
        if (selectionMode() == QAbstractItemView::SingleSelection ||
                selectionMode() == QAbstractItemView::NoSelection)
            return;
        QAbstractItemView::SelectionMode mode = QAbstractItemView::ExtendedSelection;
        if (m_head->checkIdx(col))
            mode = QAbstractItemView::MultiSelection;
        this->setSelectionMode(mode);
    });
    connect(&m_selectionDebounce, &Debounce::debout, [=]{
        bool all = this->rowCount() && this->selectedItemsByColumn().length() == this->rowCount();
        m_head->setSelectAll(all);
    });
    connect(this, &QTableWidget::itemSelectionChanged, [=]{
        m_selectionDebounce.startDeb(30);
    });
}

void TableWidgetDropFile::setHeadLabels(QStringList labL)
{
    m_headLabel = labL;
    setHorizontalHeaderLabels(labL);
    if (m_head)
        m_head->setHeadLab(labL);
}

QStringList TableWidgetDropFile::headLabels()
{
    return m_headLabel;
}

HeadView *TableWidgetDropFile::headView()
{
    return m_head;
}

QList<int> TableWidgetDropFile::selectedRow()
{
    QList<int> rowL;
    QList<QTableWidgetItem*> list = selectedItemsByColumn();
    foreach (QTableWidgetItem* item, list) {
        rowL << item->row();
    }
    return rowL;
}

QString TableWidgetDropFile::calCoreDivFrame(int core, int frame)
{
    int divSec = 0;
    QString divStr;
    if (frame != 0)
        divSec = core / frame;
    int day = divSec / 86400;
    int sec = divSec % 86400;
    day != 0 ? divStr.append(QString::number(day) + tr("天")) : QString();
    QTime tmp = QTime::fromMSecsSinceStartOfDay(sec * 1000);
    tmp.hour() != 0 ? divStr.append(QString::number(tmp.hour()) + tr("时")) : QString();
    tmp.minute() != 0 ? divStr.append(QString::number(tmp.minute()) + tr("分")) : QString();
    tmp.second() != 0 ? divStr.append(QString::number(tmp.second()) + tr("秒")) : QString();
    return divStr.isEmpty() ? "--" : divStr;
}

QString formatTime(int sec)
{
    int hour = sec / 3600;
    int minute = (sec  - hour * 3600) / 60;
    int second = (sec  - hour * 3600 - minute * 60);
    QString str;
    str.asprintf("%02d:%02d:%02d", hour, minute, second);
    return str;
}

QString TableWidgetDropFile::timeStr(int sec)
{
    return formatTime(sec);
}

QString TableWidgetDropFile::timeStr(QString time, int type)
{
    if (time.endsWith(".0"))
        time.remove(".0");

    if (0 == type) {
        QDateTime dt = QDateTime::fromString(time, "yyyy-MM-dd HH:mm:ss");
        if (dt.date() == QDateTime::currentDateTime().date())
            time = dt.toString("HH:mm:ss");
        else
            time = dt.toString("yyyy-MM-dd");
    }
    return time;
}

void TableWidgetDropFile::resizeCol()
{
    for (int i = 0; i < rowCount(); ++i) {
    }
}

int TableWidgetDropFile::page()
{
    m_page = qMax(0, m_page);
    return m_page;
}

void TableWidgetDropFile::setPage(int pa)
{
    m_page = pa;
    m_page = page();
    emit pageChanged(m_page);
}

void TableWidgetDropFile::cellEnterHand(int row)
{
    if (m_lastEnterRow == row)
        return;
    setRowColor(row, 2);
    setRowColor(m_lastEnterRow, 1);
    m_lastEnterRow = row;
}

int TableWidgetDropFile::headSecSize(int idx)
{
    if (idx >= m_headSize.length())
        return 1;
    int sum = 0;
    for (int i = 0; i < m_headSize.length(); ++i) {
            sum += qAbs(m_headSize.at(i));
    }

    int ret = 1;
    if (sum >= width())
        ret = m_headSize.at(idx);
    else
        ret = (int)(m_headSize.at(idx) * width() / sum);
    return qAbs(ret);
}

void TableWidgetDropFile::multiSel()
{
    setSelectionMode(QAbstractItemView::MultiSelection);
}

void TableWidgetDropFile::setLeave()
{
    setRowColor(m_lastEnterRow, 1);
    m_lastEnterRow = -1;
}

//此方法选择较快 cpu占用小
void TableWidgetDropFile::reselectRow(QList<int> rowL)
{
    QAbstractItemView::SelectionMode mode = this->selectionMode();
    this->setSelectionMode(QAbstractItemView::MultiSelection);
    QAbstractItemModel* model = this->model();
    int colCount = this->columnCount();
    QItemSelection selection;
    foreach (int row, rowL) {
        selection.append(QItemSelectionRange(model->index(row, 0), model->index(row, colCount - 1)));
    }
    this->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
    this->setSelectionMode(mode);
}

void TableWidgetDropFile::calSelect()
{
    m_selectP.first = -1;
    m_selectTimer.setSingleShot(true);
    connect(&m_selectTimer, &QTimer::timeout, [=]{
        if (page() != m_selectP.first) {
            m_selectP.first = page();
            m_selectP.second.clear();
        }
        emit calSel();
    });
    connect(this, &QTableWidget::itemSelectionChanged, [=]{
        m_selectTimer.start(500);
    });
}

void TableWidgetDropFile::setPreSel(QList<int> selL)
{
    m_selectP.second = selL;
}

QList<int> TableWidgetDropFile::preSelRow()
{
    return m_selectP.second;
}

bool TableWidgetDropFile::isDiffQuery(QJsonObject obj)
{
    QJsonObject t1 = obj;
    QJsonObject t2 = m_queryObj;
    m_queryObj = obj;
//    if (t1 != t2)
//        setRecordScroll(false);
    t1.remove("page"); t2.remove("page");
    return t1 != t2;
}

void TableWidgetDropFile::bindPage(Pagination *page)
{
    m_paget = page;
    connect(this, &TableWidgetDropFile::wheel, [=](bool down){
        !down ? page->on_pre_clicked() : page->on_next_clicked();
    });
}

void TableWidgetDropFile::setBackgroundImg(QString img)
{
    m_bgImg = img;
    connect(this, &TableWidgetDropFile::rowCountView, [=](){
        QString bg = "QTableWidget{background-repeat:notrepeat;background-position:center;background-image:url(%1);}";
        bg = bg.arg(m_bgImg);
        QString st = this->styleSheet(); st.remove(bg);
        this->setStyleSheet(0 == this->rowCount() ? st + bg : st);
    });
    emit this->rowCountView();
}

void TableWidgetDropFile::setFastItem(QString key, QTableWidgetItem *item)
{
    m_fastItemH.insert(key, item);
}

void TableWidgetDropFile::setFastItem(QTableWidgetItem *item, QString key)
{
    m_fastItemH2.insert(item, key);
}

void TableWidgetDropFile::selectAllRow(bool isAll)
{

}

void TableWidgetDropFile::setHHeaderLabels(QStringList labs)
{
    if (labs.isEmpty() || labs.length() % 2 != 0)
        return;
    this->setColumnCount(labs.count() / 2);
    for (int i = labs.length() / 2 - 1; i >= 0 ; --i) {
        bool ok = false;
        int width = labs.at(i * 2 + 1).toInt(&ok);
        this->setColumnWidth(i, ok ? width : 100);
        labs.removeAt(i * 2 + 1);
    }
    this->setHorizontalHeaderLabels(labs);
}

void TableWidgetDropFile::setCheckBox(int row, int type)
{
    if (row < this->rowCount() && row >= 0) {
        QTableWidgetItem* item = this->item(row, 0);
        item->setData(Qt::UserRole + 123, QVariant(type));

        if (1 == type || 3 == type) {
            bool select = 3 == type ? true : false;
            emit signalCheckBoxSelect(row, select);

            bool selectAll = m_lastClickRow.length() == this->rowCount() ? true : false;
            if (!m_isSelectAll && selectAll) {
                emit signalCheckBoxSelect(-1, true);
                m_isSelectAll = true;
            }
            if (!selectAll && m_isSelectAll)
                emit signalCheckBoxSelect(-1, false);
            m_isSelectAll = selectAll;
        }

    }
}

void TableWidgetDropFile::store(int role, int column)
{
    setSelectionMode(QAbstractItemView::MultiSelection);
    m_selectedIds.clear();
    foreach (auto item, selectedItemsByColumn(column)) {
        m_selectedIds << item->data(role);
    }
    m_hScrollBarValue = horizontalScrollBar()->value();
    m_vScrollBarValue = verticalScrollBar()->value();

    clear();
}

void TableWidgetDropFile::select(int row, QVariant id)
{
    if (m_selectedIds.contains(id))
        selectRow(row);
}

void TableWidgetDropFile::restore()
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    horizontalScrollBar()->setValue(m_hScrollBarValue);
    verticalScrollBar()->setValue(m_vScrollBarValue);
}

void TableWidgetDropFile::clear()
{
    while (rowCount()) {
        removeRow(0);
    }
}

QList<QTableWidgetItem *> TableWidgetDropFile::selectedItemsByColumn(int column)
{
    QList<QTableWidgetItem *> itemL;
    foreach (QTableWidgetItem *item, selectedItems()) {
        if (item->column() == column) {
            itemL << item;
        }
    }
    return itemL;
}

int TableWidgetDropFile::fastItem(QString key, int last)
{
    int ret = -1;
    QList<QTableWidgetItem*> itemL;
    itemL << m_fastItemH.value(key);
    if (1 == last)
        itemL << m_fastItemH2.keys(key);
    foreach (QTableWidgetItem* item, itemL) {
        if (item) {
            int tmp = this->row(item);
            ret = qMax(ret, tmp);
        }
    }
    return ret;
}

int TableWidgetDropFile::fastItemRm(QString key, int child)
{
    int ret = -1;
    QList<QTableWidgetItem*> itemL;
    itemL << m_fastItemH.value(key);
    if (1 == child)
        itemL.clear();
    itemL << m_fastItemH2.keys(key);
    foreach (QTableWidgetItem* item, itemL) {
        if (item) {
            ret = this->row(item);
            if (0 == child)
                m_fastItemH.remove(key);
            m_fastItemH2.remove(item);
            this->removeOneRow(ret);
        }
    }
    return ret;
}

void TableWidgetDropFile::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() != 0) {
        QTime cur = QTime::currentTime();
        if (qAbs(m_wheelStamp.msecsTo(cur)) > 500) {
            m_wheelStamp = cur;
            QScrollBar* rollBar = this->verticalScrollBar();
            int value = rollBar->value();
            if (event->angleDelta().y() < 0) {//后滚
                if(value == rollBar->maximum()) {
                    setPage(m_page + 1);
                    emit wheel(true);
                }
            } else {
                if(value == rollBar->minimum()) {
                    setPage(m_page - 1);
                    emit wheel(false);
                }
            }
        }
    }
    QTableWidget::wheelEvent(event);
}

void TableWidgetDropFile::mouseMoveEvent(QMouseEvent *event)
{
    if (m_lastEnterRow >= 0 && !this->itemAt(event->pos())) {
        setLeave();
        emit sigLeaveEvent();
    }
    return QTableWidget::mouseMoveEvent(event);
}

void TableWidgetDropFile::leaveEvent(QEvent *)
{
    setLeave();
    emit sigLeaveEvent();
}


void LeftSpace::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect;
    rect.setX(rect.x() + 15);

    NoFocusDelegate::paint(painter, option, index);

    QVariant iconvar = index.data(Qt::DecorationRole);
    QIcon icon = qvariant_cast<QIcon>(iconvar);
    qApp->style()->drawItemPixmap(painter, rect, Qt::AlignLeft | Qt::AlignVCenter, icon.pixmap(option.decorationSize));
    rect.setX(rect.x() + 30);

    QString text = index.data(Qt::DisplayRole).toString();
    text = option.fontMetrics.elidedText(text, Qt::ElideRight, option.rect.width() - rect.x());
    qApp->style()->drawItemText(painter, rect, Qt::AlignLeft | Qt::AlignVCenter, option.palette, true, text);
}

void LeftSpace::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    NoFocusDelegate::initStyleOption(option, index);
    option->text = "";
    option->icon = QIcon();
    option->decorationSize = QSize(35, option->decorationSize.height());
}


void MidIcon::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    NoFocusDelegate::paint(painter, option, index);

    QVariant iconvar = index.data(Qt::DecorationRole);
    QIcon icon = qvariant_cast<QIcon>(iconvar);
    if (!icon.isNull())
        qApp->style()->drawItemPixmap(painter, option.rect, Qt::AlignCenter, icon.pixmap(option.decorationSize));
}

void MidIcon::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    NoFocusDelegate::initStyleOption(option, index);
    option->icon = QIcon();
    option->decorationSize = QSize(0, 0);
}
