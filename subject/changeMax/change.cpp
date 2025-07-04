#include "change.h"
#include "ui_change.h"

#include "tool/xfunc.h"
#include <QFileDialog>
#include "choosemaxversion.h"
// changeMaxSvc.h中__SIZE_TYPE__、_Complex用不到，但会编译错误，所以预置define
#define __SIZE_TYPE__ int
#define _Complex
#include "changeMax/changeMaxSvc/changeMaxSvc.h"
#include <QLibrary>
#include "tool/jsonutil.h"
#include "view/Item/transferhand.h"
#include <QtConcurrent>
#include "changeMax/progress.h"
#include "common/eventfilter.h"
#include "view/Item/menu.h"
#include "view/set/maxset.h"

//https://blog.csdn.net/fyxichen/article/details/78754687
typedef void (*GetJobsP)(GoInt, GoInt, GoMem*, GoInterface*);
typedef void (*GetJobsCacheP)(GoMem*);
typedef void (*InitChangeMaxSvcP)(char*, char*, char*, char*);
typedef void (*SetUserNameP)(char*);
typedef void (*PostJobsP)(char*, char*, char*, GoInterface*);
typedef void (*DownloadJobP)(GoInt);
typedef void (*DownloadDirP)(GoInt p0, GoMem* p1);
typedef void (*DeleteJobP)(GoInt);
typedef void (*CancelJobP)(GoInt);
typedef void (*SetAutoPushP)(GoUint8 p0);
typedef void (*SetAutoOpenDirP)(GoUint8 p0);
typedef void (*SetPushToP)(GoInt p0);
typedef void (*SetCacheDirP)(char*);
typedef void (*FreeP)(char*);
QLibrary svc("changeMaxSvc.dll");
GetJobsP getJobs;
GetJobsCacheP getJobsCache;
InitChangeMaxSvcP initChangeMaxSvc;
SetUserNameP setUserName;
PostJobsP postJobs;
DownloadJobP downloadJob;
DownloadDirP downloadDir;
DeleteJobP deleteJob;
CancelJobP cancelJob;
SetAutoPushP setAutoPush;
SetAutoOpenDirP setAutoOpenDir;
SetPushToP setPushTo;
SetCacheDirP setCacheDir;
FreeP cfree;

Change::Change(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Change),
    m_head(NULL),
    m_pageMenu(NULL),
    m_curState(0),
    m_errMsgBox(NULL)
{
    ui->setupUi(this);
    m_head = new HeadView(Qt::Horizontal, ui->table);
    ui->table->setHorizontalHeader(m_head);
    ui->table->horizontalHeader()->setStretchLastSection(true);

    BaseWidget::setProperty(ui->table->horizontalHeader(), "horizontal", true);

    connect(m_head, &HeadView::headerClicked, [=](int idx, int order){
        if (m_pageMenu) {
            m_pageMenu->move(QCursor::pos());
            m_pageMenu->show();

            QList<QAction*> acL  = m_pageMenu->actions();
            foreach (QAction* ac, acL) {
                QString icon = m_curState == ac->data().toInt() ? ":/sortPoint.png" : "";
                ac->setIcon(QIcon(icon));
            }
        }
    });

    connect(m_head, &HeadView::signalSelectAll, [=](bool b){
        ui->table->selectAllRow(b);
        qDebug()<<__FUNCTION__<<"change";
    });
    connect(ui->table, &TableWidgetDropFile::signalCheckBoxSelect, [=](int row, bool select){
        if (row == -1) {
            m_head->setSelectAll(select ? 1 : 0);
        }
        qDebug()<<__FUNCTION__<<row<<select;
    });

    BaseWidget::setProperty(ui->table->horizontalHeader(), "from", "Change");
    BaseWidget::setProperty(ui->table, "from", "Change");

    EFAcceptDrops *ef = new EFAcceptDrops(ui->table);
    ef->setAcceptSuffixs(QStringList()<< "max");
    ef->setAcceptMaxUrls(1);
    connect(ef, &EFAcceptDrops::dropFiles, this, &Change::showChooseMaxVersion, Qt::QueuedConnection);
}

int lastClickedRow = -1;  // 保存上一次点击的行号


void Change::mousePressEvent(QMouseEvent* event) {
    // 传递事件给基类处理
    QWidget::mousePressEvent(event);
}


Change::~Change()
{
    if (m_pageMenu)
        m_pageMenu->deleteLater();

    if (QLayout *lay = ui->table->layout()) {
        BaseWidget::deleteLayout(lay);
    }

    delete ui;
}

bool Change::init()
{
    if (!svc.isLoaded()) {
        if (!svc.load()) {
            qDebug()<< __FUNCTION__ << svc.errorString();
            return false;
        }
        getJobs = (GetJobsP)svc.resolve("GetJobs");
        getJobsCache = (GetJobsCacheP)svc.resolve("GetJobsCache");
        initChangeMaxSvc = (InitChangeMaxSvcP)svc.resolve("InitChangeMaxSvc");
        setUserName = (SetUserNameP)svc.resolve("SetUserName");
        postJobs = (PostJobsP)svc.resolve("PostJobs");
        downloadJob = (DownloadJobP)svc.resolve("DownloadJob");
        downloadDir = (DownloadDirP)svc.resolve("DownloadDir");
        deleteJob = (DeleteJobP)svc.resolve("DeleteJob");
        cancelJob = (CancelJobP)svc.resolve("CancelJob");
        setAutoPush = (SetAutoPushP)svc.resolve("SetAutoPush");
        setAutoOpenDir = (SetAutoOpenDirP)svc.resolve("SetAutoOpenDir");
        setPushTo = (SetPushToP)svc.resolve("SetPushTo");
        setCacheDir = (SetCacheDirP)svc.resolve("SetCacheDir");
        cfree = (FreeP)svc.resolve("Free");
    }

    QFile::remove(USERINFO->getUserPath() + "/log-cmax.txt");
    initChangeMaxSvc(USERINFO->cmax().toUtf8().data(),
                     USERINFO->accessToken().data(),
                     USERINFO->userId().toUtf8().data(),
                     USERINFO->getUserPath().toUtf8().data());
    if (setUserName)
        setUserName(USERINFO->userName().toUtf8().data());
    updateSet();

    QTimer* rtimer = new QTimer(this);
    connect(rtimer, SIGNAL(timeout()), this, SLOT(refreshUp()));
    rtimer->start(30000);

    QTimer* ctimer = new QTimer(this);
    connect(ctimer, SIGNAL(timeout()), this, SLOT(refreshCache()));
    ctimer->start(1000);

    m_headSize << 1 << 3 << 1 << 1 << 2 << 1;
    QList<int> sizeL;
    sizeL << 29 << 159 << 203 << 152 << 150 << 137;
    QStringList labL;
    labL << tr("") << tr("文件版本/目标版本") << tr("任务名称") << tr("提交时间") << tr("状态") << tr("操作");
    ui->table->setHeadSize(labL, sizeL);

    m_menu = new Menu(this);
    m_cancelAc = new QAction(tr("取消转换"), m_menu);
    connect(m_cancelAc, &QAction::triggered, this, [=](bool checked){
        MsgBox *mb = MsgTool::msgChoose(tr("确定要取消这个任务吗？"), this);
        mb->setTitle(tr("取消转换"));
        mb->setBackgroundMask();
        connect(mb, &MsgBox::accepted, this, [=]{
            QList<int> ids = selectedIds();
            QtConcurrent::run([=](){
                foreach (GoInt id, ids) {
                    cancelJob(id);
                }
                QMetaObject::invokeMethod(this, "refreshUp");
            });
        });
    });

    m_downloadAc = new QAction(tr("下载文件"), m_menu);
    connect(m_downloadAc, &QAction::triggered, this, [=](bool checked){
        MsgBox *mb = MsgTool::msgChoose(tr("是否下载这个文件？"), this);
        mb->setTitle(tr("下载文件"));
        mb->setBackgroundMask();
        connect(mb, &MsgBox::accepted, this, [=]{
            QList<int> ids = selectedIds();
            foreach (int id, ids) {
                downloadJobSlt(id);
            }
        });
    });



    m_openDownloadDir = new QAction(tr("打开目录"), m_menu);
    connect(m_openDownloadDir, &QAction::triggered, this, [=](bool checked){
        QList<int> ids = selectedIds();
        foreach (int id, ids) {
            openDirSlt(id);
        }
    });

    m_delAc = new QAction(tr("删除记录"), m_menu);
    connect(m_delAc, &QAction::triggered, this, [=](bool checked){
        MsgBox *mb = MsgTool::msgChoose(tr("确定要删除这条记录吗？"), this);
        mb->setTitle(tr("删除记录"));
        mb->setBackgroundMask();
        connect(mb, &MsgBox::accepted, this, [=]{
            QList<int> ids = selectedIds();
            QtConcurrent::run([=](){
                foreach (GoInt id, ids) {
                    deleteJob(id);
                }
                QMetaObject::invokeMethod(this, "refreshUp");
            });
        });
    });

    m_menu->addAction(m_cancelAc);
    m_menu->addAction(m_downloadAc);
    m_menu->addSeparator();
    m_menu->addAction(m_openDownloadDir);
    m_menu->addAction(m_delAc);

    ui->table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->table, &QTableWidget::customContextMenuRequested, this, [=](QPoint po){
        checkAction(m_cancelAc, QList<int>()<< 1 << 2 << 300 << 301);
        checkAction(m_downloadAc, QList<int>()<< 4 << 6);
        checkAction(m_openDownloadDir, QList<int>()<< 4 << 6);
        checkAction(m_delAc, QList<int>()<< 4 << 5 << 6 << 7);

        BaseWidget::adjustMove(m_menu, QCursor::pos().x(), QCursor::pos().y());//已经测试与menu移动无关。
        m_menu->show();
    });

    connect(ui->headSearch, &HeadSearch::searchName, [=](int,QString name){
        refreshUp();
    });

    ui->headSearch->hideMenu();
    ui->headSearch->hideSearch();
    connect(ui->headSearch, &HeadSearch::searchMove, [=]{
        ui->headSearch->move(this->width() - ui->headSearch->width() - 10, 0);
    });

    refreshUp();
    return true;
}

void Change::resizeTableItemWidth(double ratio){
    ui->table->setColumnWidth(0, int(19*ratio));
    ui->table->setColumnWidth(1, int(159*ratio));
    ui->table->setColumnWidth(2, int(203*ratio));
    ui->table->setColumnWidth(3, int(152*ratio));
    ui->table->setColumnWidth(4, int(150*ratio));
    ui->table->setColumnWidth(5, int(137*ratio));
}

void Change::downCover(int mid)
{

}

void Change::downErrorTip(QString tip)
{

}

void Change::listFile(QJsonObject obj)
{
    ui->table->store(TransSet::missid, TableCol::Version);

    QJsonArray arr = obj["rows"].toArray();

    for (int i = 0; i < arr.size(); ++i) {
        int row = ui->table->rowCount();
        ui->table->insertRow(row);

        QJsonObject sub = arr[i].toObject();
        int state = sub["toStatus"].toString().toInt();

        int id = sub["id"].toInt();
        QString name = sub["filename"].toString();

        //文件版本 目标版本
        QTableWidgetItem *item = new QTableWidgetItem();
        QWidget *wid = new QWidget(this);
        wid->setFocusPolicy(Qt::NoFocus);
        QHBoxLayout *lay = new QHBoxLayout(this);
        lay->setContentsMargins(QMargins(0, 0, 0, 0));
        QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred);
        QLabel *def = new QLabel(this);
        def->setStyleSheet("border-image: url(:/forward.png);");
        def->setFixedSize(14, 14);
        QLabel *front = new QLabel(this);
        front->setText(sub["source_version"].toString());
        QLabel *behind = new QLabel(this);
        behind->setText(sub["target_version"].toString());
        lay->addWidget(front);
        lay->addWidget(def);
        lay->addWidget(behind);
        lay->addSpacerItem(spacer);
        wid->setLayout(lay);

        ui->table->setCellWidget(row, TableCol::Version, wid);
        item->setData(TransSet::state, state);
        item->setData(TransSet::missid, id);
        ui->table->setItem(row, TableCol::Version, item);

        //任务名称
        item = new QTableWidgetItem(name);
        item->setToolTip(name);
        ui->table->setItem(row, TableCol::Name, item);

        //提交时间
        QString times = sub["submit_time"].toString().mid(0, 19).replace("T", " ");
        times = QDateTime::fromString(times, "yyyy-MM-dd HH:mm:ss").addSecs(28800).toString("yyyy-MM-dd HH:mm:ss");
        item = new QTableWidgetItem(ui->table->timeStr(times, 0));
        item->setToolTip(times);
        ui->table->setItem(row, TableCol::Time, item);

        //状态
        Progress *pb = new Progress(this);
        pb->setPercentage(sub.value("progress").toInt(-1), stateText(state), (Progress::State)progressState(state));
        item = new QTableWidgetItem;
        item->setTextAlignment(Qt::AlignTop);
        ui->table->setCellWidget(row, TableCol::State, pb);
        ui->table->setItem(row, TableCol::State, item);

        //操作
        TransferHand *th = new TransferHand(this);
        th->showOp();
        th->bindTableMenu(ui->table, m_menu);
        th->setData(0, sub["id"].toInt());
        item = new QTableWidgetItem;
        ui->table->setCellWidget(row, TableCol::Op, th);
        ui->table->setItem(row, TableCol::Op, item);

        //空列
        item = new QTableWidgetItem;
        ui->table->setCellWidget(row, TableCol::Empty, nullptr);
        ui->table->setItem(row, TableCol::Empty, item);

        ui->table->select(row, id);
    }

    ui->table->restore();
    setDefaultPage(ui->table, tr("支持MAX文件拖拽提交或点击右下角按钮提交"));
}

void Change::listFileCache(QJsonObject obj)
{
    QJsonArray arr = obj["rows"].toArray();

    int j = 0;
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject sub = arr[i].toObject();

        for (j; j < ui->table->rowCount(); j++) {
            if (ui->table->item(j, TableCol::Version)->data(TransSet::missid).toInt() == sub["id"].toInt()) {

                int state = sub["toStatus"].toString().toInt();
                if (Progress *pb = qobject_cast<Progress *>(ui->table->cellWidget(j, TableCol::State))) {
                    pb->setPercentage(sub.value("progress").toInt(-1), stateText(state), (Progress::State)progressState(state));
                }

                TransferHand* th = (TransferHand*)ui->table->cellWidget(j, TableCol::Op);
                th->setData(1, state);

                j++;
                break;
            }
        }
    }
}

QSpacerItem *HSpacerItem() {
    return new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
}

QSpacerItem *VSpacerItem() {
    return new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
}

void Change::setDefaultPage(QTableWidget *table, QString text)
{
    if (!table)
        return;

    bool show = !table->rowCount();
    if (show) {
        if (QLayout *lay = table->layout())
            return;

        QHBoxLayout *hlay = new QHBoxLayout;
        QVBoxLayout *vlay = new QVBoxLayout;
        vlay->addSpacerItem(VSpacerItem());

        QWidget *wid = new QWidget;
        wid->setFixedSize(200, 214);
        BaseWidget::setClass(wid, "tableDefault");

        QHBoxLayout *widHlay = new QHBoxLayout;
        widHlay->addSpacerItem(HSpacerItem());
        widHlay->addWidget(wid);
        widHlay->addSpacerItem(HSpacerItem());
        vlay->addLayout(widHlay);

        if (!text.isEmpty()) {
            QLabel *lab = new QLabel(text);
            lab->setAlignment(Qt::AlignCenter);
            BaseWidget::setClass(lab, "tableDefaultText");
            vlay->addWidget(lab);
        }

        vlay->addSpacerItem(VSpacerItem());
        vlay->setSpacing(40);

        hlay->addSpacerItem(HSpacerItem());
        hlay->addLayout(vlay);
        hlay->addSpacerItem(HSpacerItem());
        table->setLayout(hlay);
    } else {
        if (QLayout *lay = table->layout()) {
            BaseWidget::deleteLayout(lay);
        }
    }
}

void Change::resizeEvent(QResizeEvent *e)
{
    ui->widget->move(0, 0);
    ui->widget->resize(this->size());

    ui->headSearch->move(this->width() - ui->headSearch->width() - 10, 0);
    ui->headSearch->raise();

    ui->selectFile->move(this->width() - ui->selectFile->width() - 10, this->height() - ui->selectFile->height() - 10);
    QWidget::resizeEvent(e);
}

void Change::refreshUp()
{
    QPointer<Change> ptr = this;
    QtConcurrent::run([=](){
        GoMem ret;
        GoInterface err;
        //TODO 分页查询
        GoInt page = 1;
        GoInt pageCount = 10000;
        getJobs(page, pageCount, &ret, &err);

        if (!ptr)
            return;

        QString data = ret.data;
        cfree(ret.data);

        QJsonObject obj = JsonUtil::jsonStrToObj(data);
        if (!QString(data).isEmpty() && obj.isEmpty()) {
            qDebug()<< "数据不完整";
            QTimer::singleShot(5000, this, &Change::refreshUp);
            return;
        }

        QMetaObject::invokeMethod(this, "listFile",
                                  Q_ARG(QJsonObject, obj));
    });
}

void Change::refreshCache()
{
    QtConcurrent::run([=](){
        GoMem ret;
        getJobsCache(&ret);
        QString data = ret.data;
        cfree(ret.data);

        QJsonObject obj = JsonUtil::jsonStrToObj(data);
        QMetaObject::invokeMethod(this, "listFileCache",
                                  Q_ARG(QJsonObject, obj));
    });
}

/* 1 - 准备中
   2 - 排队中
   3 - 转模中
   4 - 转模成功
   5 - 转模失败
   6 - 已完成
   7 - 已取消
 * */
QString Change::stateText(int state)
{
    QString t;
    switch (state) {
        case 1: t = tr("准备中"); break;
        case 2: t = tr("排队中"); break;
        case 3: t = tr("转模中"); break;
        case 4: t = tr("转模成功"); break;
        case 5: t = tr("转模失败"); break;
        case 6: t = tr("已完成"); break;
        case 7: t = tr("已取消"); break;
        case 300: t = tr("待上传"); break;
        case 301: t = tr("上传中"); break;
        case 303: t = tr("上传失败"); break;
        case 305: t = tr("上传成功"); break;
        case 310: t = tr("待下载"); break;
        case 311: t = tr("下载中"); break;
        case 313: t = tr("下载失败"); break;
        case 314: t = tr("下载成功"); break;
    default:
        break;
    }
    return t;
}

int Change::progressState(int state)
{
    Progress::State pstate = Progress::Ing;
    switch (state) {
    case 1:
    case 2:
    case 3:
    case 300:
    case 301:
    case 310:
    case 311:
        pstate = Progress::Ing;
        break;
    case 5:
    case 7:
    case 303:
    case 313:
        pstate = Progress::Err;
        break;
    case 4:
    case 6:
    case 305:
    case 314:
        pstate = Progress::Succ;
        break;
    default:
        break;
    }
    return pstate;
}

QList<int> Change::selectedIds()
{
    QList<int> ids;
    QList<QTableWidgetItem *> itemL = ui->table->selectedItemsByColumn(TableCol::Version);
    foreach (QTableWidgetItem *item, itemL) {
        ids << item->data(TransSet::missid).toInt();
    }
    return ids;
}

void Change::checkAction(QAction *ac, QList<int> stateL)
{
    ac->setEnabled(true);

    QList<QTableWidgetItem *> list = ui->table->selectedItemsByColumn(TableCol::Version);
    if (list.isEmpty()) {
        ac->setEnabled(false);
    }
    for (int i = 0; i < list.length(); i++) {
        if (!stateL.contains(list.at(i)->data(TransSet::state).toInt())) {
            ac->setEnabled(false);
        }
    }
}

void Change::showChooseMaxVersion(QStringList files)
{
    if (!files.isEmpty()) {
        ChooseMaxVersion* c = new ChooseMaxVersion(this);
        connect(c, SIGNAL(postJobSig(QStringList,QString,QString)),
                this, SLOT(postJobSlt(QStringList,QString,QString)));
        connect(c, &ChooseMaxVersion::error, this, [=](QString text){
            MsgBox *mb = MsgTool::msgOkError(text);
            mb->setBackgroundMask();
            c->close();
        });
        c->init(files);
    }
}

QString Change::headSecSize(int idx)
{
    if (0 == idx) {
        return "200";
    }
    if (idx >= m_headSize.length())
        return "1";
    return QString::number((int)((double)m_headSize.at(idx)/(double)10*(double)width()));
}

void Change::postJobSlt(QStringList files, QString srcVer, QString tarVer)
{
    QtConcurrent::run([=](){
        foreach (QString file, files) {
            GoInterface err;
            postJobs(file.toUtf8().data(), srcVer.toUtf8().data(), tarVer.toUtf8().data(), &err);
        }

        QMetaObject::invokeMethod(this, "refreshUp");
    });
}

void Change::downloadJobSlt(int jobId)
{
    GoInt id = jobId;
    downloadJob(id);
}

void Change::openDirSlt(int jobId)
{
    GoInt id = jobId;
    GoMem ret;
    downloadDir(id, &ret);
    QString path = ret.data;
    cfree(ret.data);
    qDebug()<< __FUNCTION__ << jobId << path;

    if (!path.isEmpty()) {
        XFunc::openDir(path);
    }
}

void Change::updateSet()
{
    setAutoPush(MaxSet::autoPush());
    setAutoOpenDir(MaxSet::autoOpenDir());
    setPushTo(MaxSet::pushTo());
    setCacheDir(MaxSet::cacheDir().toUtf8().data());
}

void Change::on_selectFile_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "", "", "Model Files (*.max)");
    if (file.isEmpty())
        return;

    QStringList fl;
    fl << file;
    showChooseMaxVersion(fl);
}
