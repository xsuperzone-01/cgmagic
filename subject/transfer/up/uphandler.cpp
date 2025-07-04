#include "uphandler.h"

#include <QTimer>
#include "transfer/sessiontimer.h"
#include "common/clearmax.h"
#include "tool/jsonutil.h"
#include "db/uploaddao.h"
#include "tool/network.h"

#define MAX_UP 3

UpHandler::UpHandler(TransHandler *parent) :
    TransHandler(parent)
{
    for (int i = 0; i < MAX_UP; ++i) {
        QThread* fileTh = new QThread;
        m_pool.insert(fileTh, NULL);
        fileTh->start();
    }
    m_detailC = 0;
    m_fileId = 0;
}

UpHandler::~UpHandler()
{
    while (!m_work.isEmpty()) {
        workFinished(m_work.values().first()->m_dbFile, 1);
    }
}

void UpHandler::startScan()
{
    QTimer::singleShot(100, this, SLOT(scan()));
}

void UpHandler::scan()
{
    if (!SessionTimer::m_isScan)
        return;

    int nextDown = MAX_UP - m_work.size();
    if (nextDown > 0) {

        db_upfile file = UPDAO->RfileToUp();

        if (file.id != -1) {
            //先判断对应任务是否可上传
            int mstate = 0;
            if (!m_missionState.contains(file.order)) {
                mstate = missionUpState(file.order);
            } else {
                mstate = m_missionState.value(file.order);
            }

            qD "missionUp policy:"<<mstate;

            if (mstate == 10010) {
                startScan();return;
            }
            if (mstate == TransSet::missionNoUp) {
                UPDAO->DfileByOrder(file.order);

                //清理文件
                ClearMax* cm = new ClearMax;
                connect(cm, &QThread::finished, this, [=](){
                    cm->deleteLater();
                });
                cm->start();
                startScan();return;
            }

            m_missionState.insert(file.order, mstate);

            //获取农场信息
            if (m_fileId > 10000)
                m_fileId = 0;
            else
                m_fileId++;
            Farm farm = farmInfo(file.farmid, m_fileId);

            file.state = TransSet::upping;
            file.error = 0;
            UPDAO->UfileState(file);

            QString rule = workRule(file);
            UpWork* upw = new UpWork;
            upw->setIPPort(farm.ip, farm.port, farm.error);
            upw->setSignMd5(rule, XFunc::Md5(rule));
            connect(upw, SIGNAL(UpWorkFinished(db_upfile&, int)),
                    this, SLOT(workFinished(db_upfile&, int)));
            connect(upw, SIGNAL(upSpeed(db_upfile,qint64)),
                    this, SLOT(upSpeed(db_upfile,qint64)));
            connect(upw, SIGNAL(updateFile(int,db_upfile)),
                    this, SLOT(updateFile(int,db_upfile)));
            m_work.insert(rule, upw);

            QList<QThread*> fileThList = m_pool.keys();
            for (int i = 0; i < fileThList.length(); ++i) {
                QThread* fileTh = fileThList.at(i);
                if (m_pool.value(fileTh) == NULL) {
                    m_pool.insert(fileTh, upw);
                    upw->moveToThread(fileTh);
                    break;
                }
            }

            QINVOKE(upw, "upReq",
                    Q_ARG(db_upfile, file));
        } else {
//            if (!m_missionState.isEmpty()) {
//                m_missionState.clear();
                //清理文件
//                ClearMax* cm = new ClearMax;
//                cm->start();
//            }
        }
    }

    if (!m_upDetail.isEmpty()) {
        m_detailC++;
        if (m_detailC == 15) {
            m_detailC = 0;
            QList<QString> orderL = m_upDetail.keys();
            for (int i = 0; i < orderL.length(); ++i) {
                QString order = orderL.at(i);
                QJsonObject obj = m_upDetail.value(order);
                NET->xrput(QString("/bs/mission/%1/upload/detail").arg(order), JsonUtil::jsonObjToByte(obj), [=](FuncBody f){

                }, this);
            }
            m_upDetail.clear();
        }
    }

    startScan();
}

//isDel==0 正确上传后删除
void UpHandler::workFinished(db_upfile file, int isDel)
{
    //订单已经取消
    if (13010005 == file.error) {
        m_missionState.insert(file.order, TransSet::missionNoUp);
    }

    UpWork* upw = m_work.take(workRule(file));
    if (!upw)
        return;

    disconnect(upw,0,0,0);
    upw->m_isQuit = true;

    QThread* wt = m_pool.key(upw);
    m_pool.insert(wt, NULL);

    upw->deleteLater();upw = NULL;

    if (!SessionTimer::m_isScan)
        return;

    //将文件删除的db操作放到handler线程
    if (0 == isDel) {
        UPDAO->Dfile(file);
        upSpeed(file, -1);
    }
    if (0 != isDel)
        UPDAO->UfileState(file);

    //超时重传
    if (2 == isDel || 3 == isDel) {
        removeFs(file.farmid);
        if (3 == isDel) {
            if (!reTrans(workRule(file))) {
                file.state = TransSet::fileuperror;
                UPDAO->UfileState(file);
                XFunc::veryDel(file.lzipPath);
                XFunc::veryDel(file.lzipPath + ".tmp");
                //由传输列表的右键删除触发
//                NET->xrput(QString("/bs/mission/%1/upload/fail").arg(file.order), "", [=](FuncBody f){

//                }, this);
                return;
            }
        }
        file.state = TransSet::upwait;
        UPDAO->UfileState(file);
    }

    if (0 == isDel) {
        //插件需要拷贝图片，所以全部传完再删
        //删除所有文件，是插件备份的文件 appdata/local下
        QString maxPath = file.lpath;
        if (USERINFO->isMaxPath(maxPath)) {
            XFunc::veryDel(maxPath);
        }

        //删除压缩文件
        QString lzipPath = file.lzipPath;
        if (lzipPath.contains("AppData") && lzipPath.contains("Xcgmagic"))
        {
            XFunc::veryDel(lzipPath);
        }
    }
}

void UpHandler::upSpeed(db_upfile file, qint64 speed)
{
    QJsonObject obj = m_upDetail.value(file.order);
    obj.insert("speed", -1 == speed ? obj.value("speed").toVariant().toLongLong() : ((obj.value("speed").toVariant().toLongLong() + speed) / 2));
    if (-1 == speed) {
        obj.insert("size", obj.value("size").toVariant().toLongLong() + file.size);
        obj.insert("compressSize", obj.value("compressSize").toVariant().toLongLong() + file.zipSize);
    }
    m_upDetail.insert(file.order, obj);
}

void UpHandler::updateFile(int type, db_upfile file)
{
    if (!SessionTimer::m_isScan)
        return;

    if (1 == type)
        UPDAO->UfileHash(file);
    if (2 == type)
        UPDAO->UfileId(file);
}

void UpHandler::readFile()
{
    setProgress(UPDAO->RallFile());
}

void UpHandler::readFileExceptError()
{
    setProgress(UPDAO->RallFileExceptError());
}

void UpHandler::readFileError()
{
    setProgress(UPDAO->RallFileError());
}

void UpHandler::delErrorFile()
{
    UPDAO->DErrorFile();
}

void UpHandler::delUp(QString order)
{
    NET->xrput(QString("/bs/mission/%1/upload/fail").arg(order), "", [=](FuncBody f){

    }, this);

    UPDAO->DErrorFile(order);
    UPDAO->DfileByOrder(order);

    foreach (QString rule, m_work.keys()) {
        if (rule.contains(order)) {
            UpWork* upw = m_work.value(rule);
            if (upw)
                workFinished(upw->m_dbFile, 0);
        }
    }

    //清理文件
    ClearMax* cm = new ClearMax;
    connect(cm, &QThread::finished, this, [=](){
        cm->deleteLater();
    });
    cm->start();
}

//作为m_work的键
QString UpHandler::workRule(db_upfile &file)
{
    return QString("%1_%2").arg(file.order).arg(file.lpath);
}

void UpHandler::setProgress(QList<db_upfile> fileL)
{
    QList<db_upfile> showL;
    foreach (db_upfile f, fileL) {
        QString workKey = workRule(f);
        if (UpWork *uw = m_work.value(workKey)) {
            f.tp = uw->m_dbFile.tp;
        }
        showL << f;
    }
    emit UPDAO->allFile(showL);
}
