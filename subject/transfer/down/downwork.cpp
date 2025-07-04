#include "downwork.h"

#include <QHostAddress>
#include <QDir>
#include <QUrlQuery>

#include "tool/winutil.h"
#include "tool/jsonutil.h"
#include "tool/network.h"
#include "view/set/renderset.h"
#include "common/session.h"

DownWork::DownWork(QObject *parent) :
    QObject(parent),
    m_socket(NULL),
    m_error(0),
    m_file(NULL),
    m_cfgFile(NULL),
    m_startPoint(0),
    m_newSize(0),
    m_oldSize(0),
    m_timer(NULL),
    m_speedTimer(NULL),
    m_isFirst(true),
    m_speedCount(0),
    m_isQuit(false)
{
}

DownWork::~DownWork()
{
    if (m_reply)
        m_reply->deleteLater();
}

void DownWork::downReq(db_downfile file)
{
    m_bakFile = file;
    m_dbFile = file;
    m_dbFile.tp.totalSize = file.size;

    //test
//    if (file.id == 11460) {
//        m_link = "https://ndsv1-1hgsd00-9c-9c-vrqflo0.cdnnode.cn:9295/fs698f85d5.a.bdydns.com/bdydns_6489dfdf78e2bd906c882491b1d34cbf2f25fc8b/BaiduNetdisk_7.19.0.18.exe?MqD7sp=i5aSmpCKi8LOycnOx8fKyszP2ZyXmpyUwsfMx8rHz83MxtmGmZuMj4vCzsnJzsfIzs7Mz8fPytmMnIqWm8K6vJ3Gu5fKr4aXyZq5sJTLu7K9itmGmY-NlsLOz8_ZnJOMi5DCytmGmZCPi8LKzM7ZkZCbmpOJk8LJ2Y2ajoyPm8LMz88~&UYs5cp=hpmXkIyLws7NyNHP0c_RztmGmZybkZCPi8LP2YaZjJSWj8LN2YaZjZqOlpvCm8_GnM_JnpzMmZmanZ6ezM-Zx5nOy87Km87NmcvJzM_ZhpmenYzCz9mGmYuWkprCzsnJzsfIzs7Mz8jKyw~~&sent_http_access-control-expose-headers=Content-Length%2C%20ETag%2C%20x-bs-request-id%2C%20x-pcs-request-id&sent_http_access-control-allow-origin=*&sent_http_access-control-allow-credentials=true&sent_http_access-control-allow-headers=Range%2C%20Origin%2C%20Content-Type%2C%20Accept%2C%20Content-Length&sent_http_access-control-allow-methods=GET%2C%20PUT%2C%20POST%2C%20DELETE%2C%20OPTIONS%2C%20HEAD";
//        m_dbFile.size = 219477520;
//        m_dbFile.hash = QString("EEC1F6C1FD848E65AD1398D24C1AD821").toLower();
//    }

    if (m_error != 0) {
        dError(m_error);return;
    }

    if (m_link.isEmpty()) {
        dError(10021);
        return;
    }

    //添加下载路径
    QString downPath;
    if (RenderSet::channel()) {
        QFileInfo  file(m_dbFile.lpath);
        downPath = file.fileName();
    } else {
        downPath = m_dbFile.lpath;
    }
    //配置了相机前缀。无法进行本地覆盖提醒，因为DownHandler::missionResult无法获取子任务id
    if (RenderSet::prefix()) {
        QUrl url(m_link);
        QString path = url.path();
        // /xiaoguotu_result/用户id/订单号/子任务id/1
        if (!path.isEmpty()) {
            QStringList pathL = path.split("/");
            if (pathL.length() >= 5) {
                downPath = pathL.at(4) + "_" + downPath;
            }
        }
    }
    qDebug() << __FUNCTION__ << downPath;
    downPath = validPath(downPath);
    QString exist = DDAO->RunityPath(m_dbFile.orderNum());
    QString ResultDir = DDAO->ResultDir(m_dbFile.orderNum());
    if (RenderSet::pushTo() == RenderSet::Cache || (exist.isEmpty() && ResultDir.isEmpty()) || (!ResultDir.isEmpty() && !QDir(ResultDir).exists()))
        downPath = RenderSet::cacheDir() + QString("\\%1\\").arg(file.order.split("_").first()) + downPath;
    else if (!exist.isEmpty())
        downPath = exist + "\\" + downPath;
    else
        downPath = ResultDir + "\\" + downPath;
    QDir downDir = QFileInfo(downPath).absoluteDir();
    if (!downDir.exists())
        downDir.mkpath(downDir.path());

    // 重名操作
    if (QFile::exists(downPath)) {
        QString newPath = downPath;
        QFileInfo srcfi(newPath);
        QFileInfo destfi(newPath);
        while (destfi.exists()) {
            QString newName = srcfi.fileName().replace(srcfi.completeBaseName(), srcfi.completeBaseName() + QString(" %1").arg(QDateTime::currentMSecsSinceEpoch()));
            newPath = srcfi.absoluteDir().absoluteFilePath(newName);
            destfi = QFileInfo(newPath);
        }

        RenderSet::SameFile cover = RenderSet::sameFile();
        if (RenderSet::Confirm == cover) {
            // 按任务划分
            int rc = Session::instance()->m_downHand->missionCover(m_dbFile.order);
            if (-1 == rc) {
                int ret = RenderSet::Ignore;
                QINVOKE(Session::instance()->CurWid(), "downloadConfirm", Qt::BlockingQueuedConnection,
                        Q_RETURN_ARG(int, ret),
                        Q_ARG(QString, QDir().toNativeSeparators(downPath)),
                        Q_ARG(QString, QDir().toNativeSeparators(newPath)));
                cover = (RenderSet::SameFile)ret;
                Session::instance()->m_downHand->setMissionCover(m_dbFile.order, ret);
            } else {
                cover = (RenderSet::SameFile)rc;
            }
        }

        switch (cover) {
        case RenderSet::Rename:
            downPath = newPath;
            break;
        case RenderSet::Ignore:
            finishDown(0);
            return;
        default:
            break;
        }
    }

    m_dbFile.lpath = downPath;
    m_dbFile.dpath = downPath;
    DDAO->UfileDownloadPath(m_dbFile);
    qD QString("downPath %1").arg(m_dbFile.lpath);

    QFileInfo info(m_dbFile.lpath + DTEMP);
    if (!info.exists()) {
        int cf = WinUtil::createFixedSizeFile(m_dbFile.lpath + DTEMP, m_dbFile.size);
        if (-1 == cf || -2 == cf) {
            dError(-1 == cf ? 10014 : 10020);return;
        }
    }

    m_file = new QFile(m_dbFile.lpath + DTEMP, this);
    m_cfgFile = new QFile(m_dbFile.lpath + DTEMPXR, this);

    if (!m_file->open(QIODevice::ReadWrite) || !m_cfgFile->open(QIODevice::ReadWrite)) {
        dError(10001);return;//打开失败
    }

    bool ok = false;
    qint64 pos = m_cfgFile->readAll().toLongLong(&ok);
    if (ok)
        m_startPoint = pos;

    m_dbFile.tp.completeSize = m_startPoint;
    if (m_startPoint == m_dbFile.size) {
        fileHash();return;
    }

    m_file->seek(m_startPoint);

    if (m_link.isEmpty()) {
        if (!m_socket) {
            m_socket = new QTcpSocket(this);
            connect(m_socket, SIGNAL(connected()), this, SLOT(connected()));
            connect(m_socket, SIGNAL(readyRead()), this, SLOT(readData()));

            connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                    this, SLOT(errorRecord(QAbstractSocket::SocketError)));
    //        connect(m_socket, SIGNAL(disconnected()), this, SLOT(requestFinished()));
        }

        if (m_socket->state() == QAbstractSocket::UnconnectedState) {
            m_socket->connectToHost(QHostAddress(m_ip), m_port, QIODevice::ReadWrite);
        }
    } else {
        QNetworkAccessManager *man = new QNetworkAccessManager(this);
        QNetworkRequest req;
        req.setUrl(QUrl(m_link));
        NET->setSsl(req);
        req.setRawHeader("Range", QString("bytes=%1-").arg(m_startPoint).toUtf8());

        QUrlQuery uq(m_link);

        m_reply = man->get(req);
        connect(m_reply, &QNetworkReply::readyRead, this, &DownWork::writeFile);
        connect(m_reply, &QNetworkReply::finished, this, &DownWork::replyFinished);
        connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));
    }

    if (!m_timer) {
        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(timeOut()));

        m_speedList << 0 << 0 << 0;
        m_speedTimer = new QTimer(this);
        m_speedTimer->setInterval(1000);
        connect(m_speedTimer, &QTimer::timeout, [=]{
            qint64 deltaSize = m_newSize - m_oldSize;

            qint64 sumByte = 0;
            foreach (qint64 byte, m_speedList) {
                sumByte += byte;
            }
            m_speedList.takeLast();
            m_speedList.prepend(0);
            qint64 speed = sumByte / m_speedList.length();
            m_dbFile.tp.completeSize = m_startPoint;
            m_dbFile.tp.speed = speed;

            m_oldSize = m_newSize;
            if (deltaSize == 0) {
                m_speedCount++;
                if (m_speedCount > 30)
                    timeOut();
            } else
                m_speedCount = 0;
        });
        m_speedTimer->start();
    }

    if (m_link.isEmpty())
        m_timer->start(30000);
}

void DownWork::setIPPort(QString ip, int port, int farmErr)
{
    m_ip = ip;
    m_port = port;
    m_error = farmErr;
}

void DownWork::setLink(QString link, int error)
{
    m_link = link;
    m_error = error;
}

void DownWork::readData()
{
    m_timer->stop();

    qint64 length = m_socket->bytesAvailable();

    if (m_isFirst) {
        m_isFirst = false;

        int packLength = 24;//去掉token
        if (length >= packLength) {
            QByteArray buffer = m_socket->read(packLength);
//            int sn = -1;
//            memcpy(&sn, buffer.data() + 4, 4);
//            sn = XFunc::intLowToHigh(sn);
            int status = -1;
            memcpy_s(&status, sizeof(int), buffer.data() + 16, 4);
            status = XFunc::intLowToHigh(status);
            qD QString("downRet %1_%2").arg(m_dbFile.id).arg(TransSet::codeText(status));

            if (13020300 == status) {
                if (m_socket->bytesAvailable() > 0)
                    writeFile();
            } else if (13020305 == status){//文件已全部下载
                finishDown(0);return;
            } else {
                dError(status);return;
            }
        } else {
            dError(10011);return;//协议头有错
        }
    } else {
        if (length > 0)
            writeFile();
    }
}

void DownWork::errorRecord(QAbstractSocket::SocketError error)
{
    dError(100000 + error);
}

void DownWork::replyError(QNetworkReply::NetworkError error)
{
    qDebug()<< __FUNCTION__ << m_dbFile.id << error;
    dError(110000 + error);
}

void DownWork::replyFinished()
{
    int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QUrl reurl = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    qDebug()<< __FUNCTION__ << m_dbFile.id << "http status:" << status << reurl;

//    if (!reurl.isEmpty()) {
//        qDebug()<< "use redirection:" << reurl.toString();
//        m_link = reurl.toString();

//        m_file->close();
//        m_file->deleteLater();
//        m_cfgFile->close();
//        m_cfgFile->deleteLater();
//        m_reply->deleteLater();

//        downReq(m_bakFile);
//        return;
//    }
}

void DownWork::connected()
{
    m_timer->stop();

    QJsonObject obj;
    obj.insert("id", m_dbFile.id);
    obj.insert("startPoint", m_startPoint);
    QByteArray objB = JsonUtil::jsonObjToByte(obj);
    QByteArray reqByte = TransSet::protocol(0, 130203, 1, objB.length(), objB);
    qD QString("downAsk %1_%2").arg(m_dbFile.id).arg(QString(objB));
    m_socket->write(reqByte);

    m_timer->start(30000);
}

void DownWork::timeOut()
{
    dError(10010);
}

//0删除 1改状态 2重传 3重传数次后停止
void DownWork::finishDown(int isDel)
{
    if (m_speedTimer)
        m_speedTimer->stop();

    qD QString("downFinish %1_%2").arg(m_dbFile.id).arg(TransSet::codeText(m_error));

    if (m_socket) {
        disconnect(m_socket, 0, 0, 0);
        m_socket->flush();
        m_socket->abort();
    }

    if (m_error != 0) {
        m_dbFile.state = TransSet::downerror;
        m_dbFile.error = m_error;
    }

    if (m_file) {
        m_file->close();
        m_cfgFile->close();
    }

    emit DownWorkFinished(m_dbFile, isDel);
}

void DownWork::writeFile()
{
    QByteArray byte;
    if (m_socket)
        byte = m_socket->readAll();
    else if (m_reply)
        byte = m_reply->readAll();
    qint64 fr = m_file->write(byte.data(), byte.length());

    if (fr == -1) {
        dError(10015);return;//文件访问失败
    }
    m_file->flush();

    m_startPoint += byte.length();

    if (!m_cfgFile->seek(0)) {
        dError(10015);return;//文件访问失败
    }

    QByteArray sp = QString("%1").arg(m_startPoint).toUtf8();
    fr = m_cfgFile->write(sp.data(), sp.length());
    if (fr == -1) {
        dError(10015);return;//文件访问失败
    }
    m_cfgFile->flush();

    m_newSize += byte.length();
    m_speedList.replace(0, m_newSize - m_oldSize);

    if (m_startPoint == m_dbFile.size) {
        fileHash();
    }
}

void DownWork::dError(int err)
{
    m_error = err;
    QList<int> retryL;retryL << 10010;
    if (m_socket)
        retryL << 100001 << 100005 << 100007;
    else if (m_reply)
        retryL << 110002 << 110004;
    if (-1 != retryL.indexOf(m_error))
        finishDown(2);
    else
        finishDown(3);
}

bool DownWork::fileHash()
{
    QString hash = TransSet::fileMD5(m_file, m_isQuit);
    if (m_isQuit) {
        finishDown(1);return false;
    }
    if (hash != m_dbFile.hash) {
        m_cfgFile->remove();
        dError(10018); return false;
    }

    m_file->close();
    m_cfgFile->close();

    QFile::remove(m_dbFile.lpath + DTEMPXR);
    QFile::remove(m_dbFile.lpath);
    QFile::rename(m_dbFile.lpath + DTEMP, m_dbFile.lpath);
    finishDown(0);

    return true;
}

QString DownWork::validPath(QString path)
{
    static QStringList invalidL = QStringList()<< ":" << "*" << "?" << "\"" << "<" << ">" << "|";
    foreach (QString c, invalidL) {
        path.replace(c, "");
    }
    qD __FUNCTION__ << path;
    return path;
}
