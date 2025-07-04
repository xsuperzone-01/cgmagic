#include "transfer.h"
#include "ui_transfer.h"

#include "tool/xfunc.h"
#include "transfer/sessiontimer.h"
#include "view/Item/checkboxitem.h"
#include "db/uploaddao.h"
#include "changeMax/progress.h"
#include "view/Item/transferhand.h"
#include "view/Item/menu.h"
#include "changeMax/change.h"


Transfer::Transfer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Transfer),
    m_head(NULL),
    m_pageMenu(NULL),
    m_curPage(0),
    m_curState(0)
{
    ui->setupUi(this);

    ui->table->setHeadView();
    BaseWidget::setProperty(ui->table->horizontalHeader(), "horizontal", true);

    m_menu = new Menu(this);
    m_delAc = new QAction(tr("删除记录"), m_menu);

    ui->table->setAlternatingRowColors(true);
    ui->table->setContextMenuPolicy(Qt::CustomContextMenu);

    BaseWidget::setProperty(ui->table->horizontalHeader(), "from", "Change");
    BaseWidget::setProperty(ui->table, "from", "Change");

    connect(&m_timer, &QTimer::timeout, this, &Transfer::refreshUp);
    m_timer.start(1000);

    ui->delBatchBtn->hide();    //默认是隐藏的
}

Transfer::~Transfer()
{
    if (m_pageMenu)
        m_pageMenu->deleteLater();

    if (QLayout *lay = ui->table->layout()) {
        BaseWidget::deleteLayout(lay);
    }

    delete ui;
}

void Transfer::resizeDownTableItemWidth(double ratio){
    ui->table->setColumnWidth(0, int(-36*ratio));
    ui->table->setColumnWidth(1, int(99*ratio));
    ui->table->setColumnWidth(2, int(146*ratio));
    ui->table->setColumnWidth(3, int(114*ratio));
    ui->table->setColumnWidth(4, int(86*ratio));
    ui->table->setColumnWidth(5, int(77*ratio));
    ui->table->setColumnWidth(6, int(76*ratio));
    ui->table->setColumnWidth(7, int(99*ratio));
    ui->table->setColumnWidth(8, int(75*ratio));
}

void Transfer::resizeUpTableItemWidth(double ratio){
    ui->table->setColumnWidth(0, int(-36*ratio));
    ui->table->setColumnWidth(1, int(252*ratio));
    ui->table->setColumnWidth(2, int(148*ratio));
    ui->table->setColumnWidth(3, int(156*ratio));
    ui->table->setColumnWidth(4, int(126*ratio));
    ui->table->setColumnWidth(5, int(88*ratio));
}

//是否隐藏批量删除按钮
void Transfer::hideDelBatchBtn(bool isHide){
    if(isHide){
        ui->delBatchBtn->hide();
    }else{
        ui->delBatchBtn->show();
    }
}

void Transfer::initUpload()
{
    m_curPage = 1;
    QList<int> sizeL;
    sizeL << -36 << 252 << 148 << 156 << 126 << 88;
    QStringList labL;
    labL << "" << tr("文件名称") << tr("文件大小") << tr("速度") << tr("状态") << tr("操作");
    ui->table->setHeadSize(labL, sizeL);

    ui->table->setItemDelegateForColumn(0, new CheckBoxItem(ui->table));
    {
        QMap<int, HeadView::HEADTYPE> typeMap;
        typeMap[0] = HeadView::SELECTALL;
        typeMap[5] = HeadView::SORT;
        if (HeadView *hv = qobject_cast<HeadView *>(ui->table->horizontalHeader())) {
            hv->setHeadType(typeMap);
        }

        m_pageMenu = new QMenu(this);
        QStringList acL;
        acL << tr("全部") << tr("上传")<< tr("异常");
        for (int i = 0; i < acL.length(); ++i) {
            QAction* a = new QAction(acL.at(i), m_pageMenu);
            a->setData(i);
            connect(a, &QAction::triggered, [=](bool checked){
//                ui->table->removeAllRow();
                m_curState = a->data().toInt();
                if (0 == m_curState)
                    QINVOKE(Session::instance()->m_upHand, "readFile");
                if (1 == m_curState)
                    QINVOKE(Session::instance()->m_upHand, "readFileExceptError");
                if (2 == m_curState)
                    QINVOKE(Session::instance()->m_upHand, "readFileError");
            });
            m_pageMenu->addAction(a);
        }
    }

    connect(UPDAO, SIGNAL(allFile(QList<db_upfile>)), this, SLOT(allFile(QList<db_upfile>)));

    connect(m_delAc, &QAction::triggered, [=](bool checked){
        QList<QTableWidgetItem*> iL = ui->table->selectedItemsByColumn();
        QList<QString> oL;
        for (int i = 0; i < iL.length(); i++) {
            oL.append(iL.at(i)->data(TransSet::order).toString());
        }
        for (int i = 0; i < oL.length(); i++) {
            QINVOKE(Session::instance()->m_upHand, "delUp", Q_ARG(QString, oL.at(i)));
        }
    });

    m_menu->addAction(m_delAc);

    connect(ui->table, &QTableWidget::customContextMenuRequested, [=](QPoint po){
        checkAction(m_delAc, QList<int>()<<TransSet::uperror << TransSet::fileuperror);
        QPoint v(21, 21);
        m_menu->move(QCursor::pos() - v);
        m_menu->show();
    });//点击右键后判断是否需要可点击

    refreshUp();

    ui->headSearch->hide();

}

void Transfer::initDownload()
{
    m_curPage = 2;

    QMap<int, HeadView::HEADTYPE> typeMap;
    typeMap[0] = HeadView::SELECTALL;
    if (HeadView *hv = qobject_cast<HeadView *>(ui->table->horizontalHeader())) {
        hv->setHeadType(typeMap);
    }

    QList<int> sizeL;
    sizeL << -36 << 99 << 146 << 114 << 86 << 77 << 76 << 95 << 75;
    QStringList labL;
    labL << "" << tr("任务单号") << tr("任务名") << tr("相机名") << tr("出图名") << tr("文件大小") << tr("速度") << tr("状态") << tr("操作");
    ui->table->setHeadSize(labL, sizeL);

    ui->table->setItemDelegateForColumn(0, new CheckBoxItem(ui->table));
//    connect(ui->table, &TableWidgetDropFile::itemSelectionChanged, this, [=]{
//        foreach (QTableWidgetItem *item, ui->table->selectedItemsByColumn()) {
//            item->setCheckState(Qt::Checked);
//        }
//    });
    connect(DDAO, SIGNAL(allFile(QList<db_downfile>)), this, SLOT(allFile(QList<db_downfile>)));

    connect(m_delAc, &QAction::triggered, [=](bool checked){
        QINVOKE(Session::instance()->m_downHand, "updateFileState",
                Q_ARG(QList<db_downfile>, this->selectedDownFile()),
                Q_ARG(int, -1));
    });
    m_pauseAc = new QAction(tr("暂停下载"), m_menu);
    connect(m_pauseAc, &QAction::triggered, [=](bool checked){
        QINVOKE(Session::instance()->m_downHand, "updateFileState",
                Q_ARG(QList<db_downfile>, this->selectedDownFile()),
                Q_ARG(int, TransSet::downpause));
    });
    m_startAc = new QAction(tr("开始下载"), m_menu);
    connect(m_startAc, &QAction::triggered, [=](bool checked){
        QINVOKE(Session::instance()->m_downHand, "updateFileState",
                Q_ARG(QList<db_downfile>, this->selectedDownFile()),
                Q_ARG(int, TransSet::downwait));
    });

    m_menu->addAction(m_startAc);
    m_menu->addAction(m_pauseAc);
    m_menu->addAction(m_delAc);

    connect(ui->table, &QTableWidget::customContextMenuRequested, [=](QPoint po){
        checkAction(m_startAc, QList<int>()<<TransSet::downpause<<TransSet::downerror<<TransSet::filedownerror);
        checkAction(m_pauseAc, QList<int>()<<TransSet::downwait<<TransSet::downing);
        checkAction(m_delAc, QList<int>()<<TransSet::downwait<<TransSet::downing<<TransSet::downpause<<TransSet::downerror<<TransSet::filedownerror);

//        m_menu->move(QCursor::pos());
        BaseWidget::adjustMove(m_menu, QCursor::pos().x(), QCursor::pos().y());
        m_menu->show();
    });

    refreshUp();

    connect(ui->headSearch, &HeadSearch::searchName, [=](int,QString name){
        DDAO->changeSearch(name);
        refreshUp();
    });
    ui->headSearch->hideMenu();
    ui->headSearch->hideRefresh();
    ui->headSearch->hideSearch(true);
    connect(ui->headSearch, &HeadSearch::searchMove, [=]{
        ui->headSearch->move(this->width() - ui->headSearch->width(), 0);
    });
}

void Transfer::refresh()
{
    refreshUp();
}

void Transfer::resizeEvent(QResizeEvent *e)
{
    ui->widget->move(0, 0);
    ui->widget->resize(this->size());

//    for (int i = 0; i < ui->table->columnCount(); ++i) {
//        ui->table->setColumnWidth(i, headSecSize(i).toInt());
//    }

    ui->headSearch->move(this->width() - ui->headSearch->width(), 0);
    ui->headSearch->raise();

    return QWidget::resizeEvent(e);
}

void Transfer::allFile(QList<db_upfile> fl)
{
/*
    QList<db_upfile> list;
    db_upfile a;
    a.lpath = "文件名";
    a.order = "任务单号";
    a.state = 13;
    a.error = 0;
    a.tp.speed = 10;
    a.tp.completeSize = 1024;
    a.tp.totalSize = 4096;
    fl.append(a);

    db_upfile b;
    b.lpath = "文件名";
    b.order = "任务单号";
    b.state = 30;
    b.error = 0;
    b.tp.speed = 10;
    b.tp.completeSize = 1024;
    b.tp.totalSize = 4096;
    fl.append(b);

    db_upfile c;
    c.lpath = "文件名";
    c.order = "任务单号";
    c.state = 31;
    c.error = 0;
    c.tp.speed = 10;
    c.tp.completeSize = 1024;
    c.tp.totalSize = 4096;
    fl.append(c);
*/

    ui->table->store(Qt::UserRole);

    for (int i = 0; i < fl.length(); ++i) {
//        for (int i = 0; i < 2; ++i) {
        int row = ui->table->rowCount();
        ui->table->insertRow(row);

        db_upfile f = fl.at(i);

        //可选
        QTableWidgetItem* item = new QTableWidgetItem;
        item->setData(Qt::UserRole, f.lpath);
        item->setData(TransSet::order, f.order);
        item->setData(TransSet::filepath, f.lpath);
        item->setData(TransSet::state, f.state);
        ui->table->setItem(row, UploadTableCol::Check, item);
        ui->table->setCheckBox(row, 1);

        //文件名
        QFileInfo fi(f.lpath);
        item = new QTableWidgetItem(fi.fileName());
        ui->table->setItem(row, UploadTableCol::Name, item);

        //文件大小 有压缩，所以取tp里面的totalSize
        QString size = QString("%1/%2").arg(XFunc::getSizeString(f.tp.completeSize)).arg(XFunc::getSizeString(f.tp.totalSize));
        item = new QTableWidgetItem(size);
        ui->table->setItem(row, UploadTableCol::Size, item);

        //速度
        item = new QTableWidgetItem(XFunc::getSpeedString(f.tp.speed));
        ui->table->setItem(row, UploadTableCol::Speed, item);

        //状态
        Progress *pb = new Progress(this);
//        pb->initProgress(f.tp.progress(), TransSet::transState(f.state, f.error), (Progress::State)progressState(f.state));
        pb->setPercentage(f.tp.progress(), TransSet::transState(f.state, f.error), (Progress::State)progressState(f.state));
        item = new QTableWidgetItem;
        item->setToolTip(TransSet::codeText(f.error));
        ui->table->setItem(row, UploadTableCol::State, item);
        ui->table->setCellWidget(row, UploadTableCol::State, pb);

        //操作
        TransferHand *th = new TransferHand(this);
        th->showDel();
        qDebug()<<"状态代码为"<<f.state;
        th->changeDel(f.state);
        th->bindTableMenu(ui->table, m_menu);
        item = new QTableWidgetItem;
        ui->table->setItem(row, UploadTableCol::Op, item);
        ui->table->setCellWidget(row, UploadTableCol::Op, th);

        ui->table->select(row, f.lpath);
    }

    ui->table->restore();
    Change::setDefaultPage(ui->table, tr("当前没有上传中的任务"));
}

void Transfer::allFile(QList<db_downfile> fl)
{
/*
    QList<db_downfile> list;
    db_downfile u;
    u.id = 0;
    u.lpath = "文件名";
    u.order = "任务单号";
    u.mname = "任务名";
    u.camera = "相机名";

    u.state = 0;
    u.error = 0;
    u.tp.speed = 10;
    u.tp.completeSize = 1024;
    u.tp.totalSize = 4096;
    u.size = 4096;
    fl.append(u);

    db_downfile v;
    v.id = 1;
    v.lpath = "文件";
    v.order = "任务单";
    v.mname = "任务";
    v.camera = "相机";

    v.state = 0;
    v.error = 0;
    v.tp.speed = 10;
    v.tp.completeSize = 2048;
    v.tp.totalSize = 4096;
    v.size = 4096;
    fl.append(v);
*/
    ui->table->store(Qt::UserRole);

    for (int i = 0; i < fl.length(); ++i) {
        int row = ui->table->rowCount();
        ui->table->insertRow(row);

        db_downfile f = fl.at(i);

        //可选
        QTableWidgetItem* item = new QTableWidgetItem;
        item->setData(Qt::UserRole, f.id);
        item->setData(TransSet::order, f.order);
        item->setData(TransSet::fileid, f.id);
        item->setData(TransSet::state, f.state);
        item->setData(TransSet::downfile, QVariant::fromValue(f));
        ui->table->setItem(row, DownloadTableCol::DCheck, item);
        ui->table->setCheckBox(row, 1);

        //单号
        item = new QTableWidgetItem(f.orderNum());
        ui->table->setItem(row, DownloadTableCol::DOrder, item);
        //任务名
        item = new QTableWidgetItem(f.mname);
        ui->table->setItem(row, DownloadTableCol::DName, item);
        //相机名
        item = new QTableWidgetItem(f.camera);
        ui->table->setItem(row, DownloadTableCol::DCamera, item);

        //文件名
        QFileInfo fi(f.lpath);
        item = new QTableWidgetItem(fi.fileName());
        ui->table->setItem(row, DownloadTableCol::DOut, item);

        //文件大小
        QString size = QString("%1/%2").arg(XFunc::getSizeString(f.tp.completeSize)).arg(XFunc::getSizeString(f.size));
        item = new QTableWidgetItem(size);
        ui->table->setItem(row, DownloadTableCol::DSize, item);

        //速度
        item = new QTableWidgetItem(XFunc::getSpeedString(f.tp.speed));
        ui->table->setItem(row, DownloadTableCol::DSpeed, item);

        //状态
        Progress *pb = new Progress(this);
//        pb->initProgress(f.tp.progress(), TransSet::transState(f.state, f.error), (Progress::State)progressState(f.state));
        pb->setPercentage(f.tp.progress(), TransSet::transState(f.state, f.error), (Progress::State)progressState(f.state));
        item = new QTableWidgetItem;
        item->setToolTip(TransSet::codeText(f.error));
        ui->table->setItem(row, DownloadTableCol::DState, item);
        ui->table->setCellWidget(row, DownloadTableCol::DState, pb);

        //操作
        TransferHand *th = new TransferHand(this);
        th->showOp();
        th->bindTableMenu(ui->table, m_menu);
        item = new QTableWidgetItem;
        ui->table->setItem(row, DownloadTableCol::DOp, item);
        ui->table->setCellWidget(row, DownloadTableCol::DOp, th);

        ui->table->select(row, f.id);
    }

    ui->table->restore();
    Change::setDefaultPage(ui->table, tr("当前没有下载中的任务"));
}

void Transfer::refreshUp()
{
    if (1 == m_curPage) {
        emit m_pageMenu->actions().at(m_curState)->triggered(true);
    }
    if (2 == m_curPage) {
        QINVOKE(Session::instance()->m_downHand, "readFile");
    }
}

QColor Transfer::stateColor(int state)
{
    QColor color("#ccc");
    if (state == TransSet::downerror ||
        state == TransSet::uperror ||
        state == TransSet::filedownerror ||
        state == TransSet::fileuperror)
        color = QColor("#e94b5a");
    return color;
}

QList<db_downfile> Transfer::selectedDownFile()
{
    QList<db_downfile> dL;
    QList<QTableWidgetItem*> iL = ui->table->selectedItemsByColumn();
    for (int i = iL.length() - 1; i >= 0; i--) {
        dL << iL.at(i)->data(TransSet::downfile).value<db_downfile>();
    }
    return dL;
}

void Transfer::checkAction(QAction *ac, QList<int> stateL)
{
    ac->setEnabled(true);

    QList<QTableWidgetItem *> list = ui->table->selectedItemsByColumn();
    if (list.isEmpty()) {
        ac->setEnabled(false);
    }
    for (int i = 0; i < list.length(); i++) {
        if (!stateL.contains(list.at(i)->data(TransSet::state).toInt())) {
            ac->setEnabled(false);
        }
    }
}

int Transfer::progressState(int state)
{
    Progress::State pstate = Progress::Ing;
    switch (state) {
    case TransSet::downerror:
    case TransSet::uperror:
    case TransSet::fileuperror:
    case TransSet::filedownerror:
        pstate = Progress::Err;
        break;
    default:
        break;
    }
    return pstate;
}

QString Transfer::headSecSize(int idx)
{
    if (0 == idx) {
        return "30";
    }
    if (idx >= m_headSize.length())
        return "1";
    return QString::number((int)((double)m_headSize.at(idx)/(double)10*(double)width()));
}

void Transfer::on_delBatchBtn_clicked()
{
    if(this->selectedDownFile().isEmpty()){
        qDebug()<<"File is empty!";
    }else{
        QINVOKE(Session::instance()->m_downHand, "updateFileState",
                Q_ARG(QList<db_downfile>, this->selectedDownFile()),
                Q_ARG(int, -1));
    }
}
