#include "downhandler.h"

#include <QTimer>
#include "transfer/sessiontimer.h"
#include "common/trayicon.h"
#include "common/session.h"
#include "tool/jsonutil.h"
#include "tool/network.h"
#include "view/set/renderset.h"

#define MAX_DOWN 1

DownHandler::DownHandler(TransHandler *parent) :
    TransHandler(parent)
{
    for (int i = 0; i < MAX_DOWN; ++i) {
        QThread* fileTh = new QThread;
        m_pool.insert(fileTh, NULL);
        fileTh->start();
    }
}

DownHandler::~DownHandler()
{
    while (!m_work.isEmpty()) {
        workFinished(m_work.values().first()->m_dbFile, 1);
    }
}

void DownHandler::startScan()
{
    QTimer::singleShot(100, this, SLOT(scan()));

    connect(DDAO, &DownloadDAO::fileDeleted, this, &DownHandler::removeTempFile, Qt::UniqueConnection);
}

void DownHandler::scan()
{
    if (!SessionTimer::m_isScan)
        return;

    int nextDown = MAX_DOWN - m_work.size();
    if (nextDown > 0) {
        db_downfile file = DDAO->RfileToDown();
        if (file.id != -1) {
//            Farm farm = farmInfo(file.farmid, file.id);
            DownloadUrl link = downloadLink(file.farmid, file.id);

            file.state = TransSet::downing;
            file.error = 0;
            DDAO->UfileState(file);

            DownWork* dw = new DownWork;
//            dw->setIPPort(farm.ip, farm.port, farm.error);
            dw->setLink(link.url, link.status == 11002200 ? 0 : link.status);
            connect(dw, SIGNAL(DownWorkFinished(db_downfile&, int)),
                    this, SLOT(workFinished(db_downfile&, int)));

            m_work.insert(workRule(file), dw);

            QList<QThread*> fileThList = m_pool.keys();
            for (int i = 0; i < fileThList.length(); ++i) {
                QThread* fileTh = fileThList.at(i);
                if (m_pool.value(fileTh) == NULL) {
                    m_pool.insert(fileTh, dw);
                    dw->moveToThread(fileTh);
                    break;
                }
            }

            QINVOKE(dw, "downReq",
                    Q_ARG(db_downfile, file));
        } else {
            QMutexLocker lock(&m_mcMu);
            if (m_missionCover.size() > 0)
                m_missionCover.clear();
        }
    }

    startScan();
}

void DownHandler::workFinished(db_downfile &file, int isDel)
{
    QString workKey = workRule(file);

    DownWork* dw = m_work.take(workKey);
    if (!dw)
        return;

    disconnect(dw,0,0,0);
    dw->m_isQuit = true;

    QThread* wt = m_pool.key(dw);
    m_pool.insert(wt, NULL);

    dw->deleteLater();dw = NULL;

    if (!SessionTimer::m_isScan)
        return;

    if (0 == isDel) {
        DDAO->Uu3dOk(file.orderNum());
        DDAO->Dfile(file);

        if (RenderSet::autoOpenDir() && !file.dpath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(file.dpath).absoluteDir().path()));
        }
    }
    if (0 != isDel)
        DDAO->UfileState(file);

    //超时重传
    if (2 == isDel || 3 == isDel) {
        removeFs(file.farmid);
        if (3 == isDel) {
            if (!reTrans(workKey)) {
                file.state = TransSet::filedownerror;
                DDAO->UfileState(file);
                removeTempFile(file);
                return;
            }
        }
        file.state = TransSet::downwait;
        DDAO->UfileState(file);
    }

    //简单加上红点
    if (0 == isDel)
        emit downWorkOver();
}

void DownHandler::workCanceled(int id)
{

}

void DownHandler::readFile()
{
    QList<db_downfile> showL;
    QList<db_downfile> fileL = DDAO->RallFile();
    foreach (db_downfile f, fileL) {
        QString workKey = workRule(f);
        if (DownWork *dw = m_work.value(workKey)) {
            f.tp = dw->m_dbFile.tp;
        }
        showL << f;
    }
    emit DDAO->allFile(showL);
}

void DownHandler::updateFileState(QList<db_downfile> fileL, int toState)
{
    for (int i = 0; i < fileL.length(); i++) {
        db_downfile file = fileL.at(i);
        if (-1 == toState) {
//            DDAO->Dfiles(fileL);
        } else {
            file.state = toState;
            DDAO->UfileState(file);
        }

        workFinished(file, -1);
    }
    if (-1 == toState) {
        DDAO->Dfiles(fileL);
    }
}

void DownHandler::delErrorFile()
{
    DDAO->DErrorFile();
}

void DownHandler::missionResult(int mid, bool autoRes, bool isBatch)
{
    //是否处理推送
    if (autoRes && (!RenderSet::autoPush() || !USERINFO->m_xgtRole.download))
        return;

    NET->xrget(QString("/bs/mission/%1/results").arg(mid), [=](FuncBody f){
        QJsonObject obj = f.j;
        QJsonArray arr = obj["rows"].toArray();

        int status = obj["status"].toInt();
        if (11000905 == status) {
            QString text = QString("账户余额不足无法下载，请充值！\r\n任务名：%1 - %2").arg(obj["name"].toString()).arg(obj["camera"].toString());
            QINVOKE(&Session::instance()->mainWid()->m_msgTool, "msgOk",
                    Q_ARG(QString, text), Q_ARG(QWidget*, Session::instance()->mainWid()));
        }
        if (11000906 == status) {
            QINVOKE(&Session::instance()->mainWid()->m_msgTool, "msgOk",
                    Q_ARG(QString, tr("下载失败，权限不足")), Q_ARG(QWidget*, Session::instance()->mainWid()));
        }
        if (arr.isEmpty()) {
            QINVOKE(TrayIcon::instance(), "showInfoMsg",
                    Q_ARG(QString, ""),
                    Q_ARG(QString, tr("当前无可下载的文件！")));
            return;
        }

        obj.insert("autoPush", autoRes);
        m_cover.insert(mid, obj);

        coverResult(mid, 0);
    }, this);
}

void DownHandler::coverResult(int mid, int cover)
{
    QJsonObject obj = m_cover.take(mid);
    if (cover != 0)
        return;

    bool autoPush = obj.value("autoPush").toBool();
    QJsonArray arr = obj["rows"].toArray();
    qint64 stamp = QDateTime::currentMSecsSinceEpoch();
    QList<db_downfile> uL;
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject row = arr.at(i).toObject();

        db_downfile uf;
        uf.id = row["id"].toInt();
        //订单号_任务id,作为键
        uf.order = QString("%1_%2").arg(obj["num"].toString()).arg(mid);
        uf.order2 = obj["num"].toString();
        uf.mname = obj["name"].toString();
        uf.camera = obj["camera"].toString();
        uf.farmid = obj["farmId"].toInt();
        uf.lpath = row["path"].toString();
        uf.hash = row["hash"].toString();
        uf.size = row.value("size").toVariant().toLongLong();
        uf.state = TransSet::downwait;
        uf.error = 0;
        uf.pri = stamp * 10000 + i;

        if (autoPush) {
            if (RenderSet::psd() && !uf.lpath.endsWith(".psd")) {
                continue;
            }
        }

        m_downList << uf;
    }

    //批量下载会出现重复记录，所以先延时
    if (!m_delayTimer.isActive()) {
        connect(&m_delayTimer, SIGNAL(timeout()), this, SLOT(delayDown()), Qt::UniqueConnection);
        m_delayTimer.setSingleShot(true);
    }
    m_delayTimer.start(2000);
}

void DownHandler::setMissionCover(QString order, int cover)
{
    QMutexLocker lock(&m_mcMu);
    m_missionCover.insert(order, cover);
}

int DownHandler::missionCover(QString order)
{
    QMutexLocker lock(&m_mcMu);
    if (!m_missionCover.contains(order))
        return -1;
    return m_missionCover.value(order);
}

void DownHandler::delayDown()
{
    if (!m_downList.isEmpty()) {
        DDAO->Cfile(m_downList);
        DDAO->Uu3dSum(m_downList.first().orderNum(), m_downList.length());
        QINVOKE(TrayIcon::instance(), "showInfoMsg",
                Q_ARG(QString, ""),
                Q_ARG(QString, tr("已加入下载列表！")));
        m_downList.clear();
    }
}

void DownHandler::removeTempFile(db_downfile file)
{
    if (file.dpath.isEmpty())
        return;
    qDebug()<< __FUNCTION__ << file.dpath;
    QFile::remove(file.dpath + DTEMPXR);
    QFile::remove(file.dpath + DTEMP);
}

QString DownHandler::workRule(db_downfile &file)
{
    return QString("%1_%2").arg(file.order).arg(file.id);
}
