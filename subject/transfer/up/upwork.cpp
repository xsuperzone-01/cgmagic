#include "upwork.h"

#include <QHostAddress>
#include "quazip/JlCompress.h"
#include "tool/jsonutil.h"
#include "db/uploaddao.h"

#define UpStep 200 //ms

UpWork::UpWork(QObject *parent) :
    QObject(parent),
    m_socket(NULL),
    m_startPoint(0),
    m_uppedSize(0),
    m_oldSize(0),
    m_srcSize(0),
    m_zipRatio(1.00),
    m_file(NULL),
    m_error(0),
    m_timer(NULL),
    m_speedTimer(NULL),
    m_isQuit(false),
    m_send(false),
    m_dataSize(10 * 1024),
    m_curTime(0)
{
}

UpWork::~UpWork()
{
    if (m_timer) {
        m_timer->stop();m_timer->deleteLater();m_timer = NULL;
        m_speedTimer->stop();m_speedTimer->deleteLater();m_speedTimer = NULL;
    }
    if (m_socket) {
        disconnect(m_socket,0,0,0);
        m_socket->abort();
    }
    if (m_file) {
        m_file->close();delete m_file;m_file = NULL;
    }
}

void UpWork::upReq(const db_upfile &file)
{
    qD QString("开启上传:%1").arg(m_sign);

    m_dbFile = file;
    m_dbFile.tp.totalSize = file.size;

    //增加农场信息获取错误
    if (m_error != 0) {
        uError(m_error);return;
    }

    m_file = new QFile;

    QFileInfo info(m_dbFile.lpath);
    if (!info.exists()) {
        uError(10000);return;
    }
    qint64 modTime = info.lastModified().toMSecsSinceEpoch();
    if (modTime != m_dbFile.modtime) {
        m_dbFile.modtime = modTime;
        m_dbFile.size = info.size();
        m_dbFile.hash.clear();
        UPDAO->UfileModTime(m_dbFile);
        uError(10019);return;
    }

    if (m_dbFile.hash.isEmpty()) {
        m_file->setFileName(m_dbFile.lpath);
        if (!m_file->open(QIODevice::ReadOnly)) {
            uError(10001);return;
        }

        QString hash = TransSet::fileMD5(m_file, m_isQuit);
        if (m_isQuit) {
            finishUp(1);return;
        }
        if (hash.isEmpty()) {
            uError(10016);return;
        }
        m_dbFile.hash = hash;
        emit updateFile(1, m_dbFile);
    }

    m_srcSize = m_dbFile.size;

    if (!m_socket) {
        m_socket = new QTcpSocket(this);
        connect(m_socket, SIGNAL(connected()), this, SLOT(connected()));
        connect(m_socket, SIGNAL(readyRead()), this, SLOT(readData()));
        connect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
        connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(errorRecord(QAbstractSocket::SocketError)));
//        connect(m_socket, &QTcpSocket::disconnected, [=]{
//            finishUp();
//        });

        m_timer = new QTimer;
        m_timer->setSingleShot(true);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(timeOut()));

        m_speedList <<0<<0<<0 <<0<<0<<0 <<0<<0<<0 <<0;
        m_speedTimer = new QTimer(this);
        m_speedTimer->setInterval(1000);
        connect(m_speedTimer, &QTimer::timeout, [=]{
            qint64 sumByte = 0;
            foreach (qint64 byte, m_speedList) {
                sumByte += byte;
            }
            m_speedList.takeLast();
            m_speedList.prepend(0);
            qint64 speed = sumByte / m_speedList.length();
            qint64 upped = m_startPoint + m_uppedSize;

//            upped = (qint64)(((double)1.00 / (double)m_zipRatio) * (double)upped);
//            if (upped > m_srcSize)
//                upped = m_srcSize;

            if (upped == m_file->size())
                upped--;
            m_dbFile.tp.completeSize = upped;
            m_dbFile.tp.speed = speed;
//            emit UPDAO->fileSpeedChanged(m_dbFile.order, m_dbFile.lpath, upped, /*m_srcSize*/m_file->size(), speed);
            emit upSpeed(m_dbFile, speed);
            m_oldSize = m_uppedSize;
        });

    }

    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        m_socket->connectToHost(QHostAddress(m_ip), m_port, QIODevice::ReadWrite);
        m_timer->start(30000);
    }
}

void UpWork::setIPPort(QString ip, int port, int farmErr)
{
    m_ip = ip;
    m_port = port;
    m_error = farmErr;
}

void UpWork::setSignMd5(QString rule, QString md5)
{
    md5 = md5.left(8);
    qD QString("%1_%2").arg(rule).arg(md5);
    m_sign = md5;
}

QString UpWork::zipPath()
{
    return QString("%1/%2_%3_%4.zip").arg(USERINFO->getUserPath()).arg(m_dbFile.farmid).arg(m_dbFile.order).arg(m_dbFile.id);
}

void UpWork::connected()
{
    m_timer->stop();

    //先询问
    QJsonObject obj;
    if (m_dbFile.id != 0)
        obj.insert("id", m_dbFile.id);
    obj.insert("hash", m_dbFile.hash);
    obj.insert("num", m_dbFile.order);
    obj.insert("path", m_dbFile.lpath);
    obj.insert("size", m_dbFile.size);


    QByteArray objB = JsonUtil::jsonObjToByte(obj);
    qD QString("秒传询问:%1_%2").arg(m_sign).arg(QString(objB));
    QByteArray reqByte = TransSet::protocol(0, 130100, 1, objB.length(), objB);

    m_socket->write(reqByte);

    m_timer->start(30000);
}

void UpWork::readData()
{
    m_timer->stop();

    int packLength = 24;//去掉token
    qint64 length = m_socket->bytesAvailable();
    if (length >= packLength) {
        QByteArray buffer = m_socket->read(packLength);


        int sn = -1;
        memcpy_s(&sn, sizeof(int), buffer.data() + 4, 4);
        sn = XFunc::intLowToHigh(sn);
        int status = -1;
        memcpy_s(&status, sizeof(int), buffer.data() + 16, 4);
        status = XFunc::intLowToHigh(status);

        //id询问
        if (0 == sn) {
            qD QString("秒传ret:%1_%2").arg(m_sign).arg(TransSet::codeText(status));
            if (13010000 == status) {//秒传
                finishUp(0);
            } else if (13010001 == status) {
                buffer = m_socket->readAll();
                QJsonObject obj = JsonUtil::jsonStrToObj(buffer);
                m_dbFile.id = obj.value("id").toInt(0);
                emit updateFile(2, m_dbFile);
                startZip();
            } else if (13010005 == status){//订单已经取消
                m_error = status;
                finishUp(0);
            } else {
                uError(status);
            }
        }

        //压缩询问
        if (1 == sn) {
            qD QString("压缩ret:%1_%2").arg(m_sign).arg(TransSet::codeText(status));
            if (13010100 == status) {
                buffer = m_socket->readAll();
                QJsonObject obj = JsonUtil::jsonStrToObj(buffer);
                m_startPoint = obj.value("startPoint").toVariant().toLongLong();
                startUp();
            } else if (13010103 == status) {//上传成功
                finishUp(0);
            } else if (13010104 == status) {//后台处理中
                m_timer->start(15000);
                m_startPoint = m_file->size();
            } else {
                uError(status);
            }
        }

        //完成返回
        if (2 == sn) {
            qD QString("上传ret:%1_%2").arg(m_sign).arg(TransSet::codeText(status));
            if (13010200 == status) {
                finishUp(0);
            } else {
                uError(status);
            }
        }
    } else {
        uError(10011);return;//协议头有错
    }
}

void UpWork::errorRecord(QAbstractSocket::SocketError error)
{
    uError(100000 + error);
}

void UpWork::bytesWritten(qint64 len)
{
    if (!m_send)
        return;

    qint64 tt = QDateTime::currentMSecsSinceEpoch();
    m_dataSize = TransSet::dataSize(m_dataSize, tt - m_curTime);
    m_curTime = tt;

    m_timer->stop();

    m_uppedSize += len;
    m_speedList.replace(0, m_uppedSize - m_oldSize);

    QTimer::singleShot(UpStep / 2, this, SLOT(sendData()));
}

void UpWork::timeOut()
{
    //超时
    uError(10010);
}

void UpWork::startZip()
{
    //定位到压缩包
    m_dbFile.lzipPath = zipPath();
    m_file->close();
    m_file->setFileName(m_dbFile.lzipPath);

    if (!m_file->exists()) {
        bool cr = JlCompress::xrenderCompress(m_dbFile.lzipPath + ".tmp", m_dbFile.lpath, m_isQuit);
        if (m_isQuit) {
            finishUp(1);return;
        }
        if (!cr) {
            uError(10017);return;
        }
        QFile::rename(m_dbFile.lzipPath + ".tmp", m_dbFile.lzipPath);
        m_file->setFileName(m_dbFile.lzipPath);
    }

    m_dbFile.zipSize = m_file->size();

    QJsonObject obj;
    obj.insert("zipSize", m_file->size());
    QByteArray objB = JsonUtil::jsonObjToByte(obj);
    qD QString("压缩询问:%1_%2").arg(m_sign).arg(QString(objB));
    QByteArray reqByte = TransSet::protocol(1, 130101, 1, objB.length(), objB);
    m_socket->write(reqByte);

    m_timer->start(30000);

    m_speedTimer->start();
}

void UpWork::startUp()
{
    if (!m_file->open(QIODevice::ReadOnly)) {
        uError(10001);return;
    }
    m_zipRatio = (double)((double)m_file->size() / (double)m_srcSize);
    qD QString("压缩率:%1_%2").arg(m_sign).arg(m_zipRatio);

    m_dbFile.tp.totalSize = m_file->size();
    m_dbFile.tp.completeSize = m_startPoint;

    if (m_startPoint > m_file->size()) {
        uError(10013);return;//起始位置过大
    }
    if (!m_file->seek(m_startPoint)) {
        uError(10015);return;//文件访问失败
    }

    qD QString("filesize:%1_startpos:%2").arg(m_file->size()).arg(m_startPoint);
    QByteArray reqByte = TransSet::protocol(2, 130102, 2, m_file->size() - m_startPoint, "");
    m_socket->write(reqByte);

    QTimer::singleShot(UpStep, this, SLOT(sendData()));
}

void UpWork::sendData()
{
    m_send = true;

    if (!m_isQuit && (m_file->pos() != m_file->size())) {
        QByteArray fb;
        fb.resize(m_dataSize);
        qint64 fr = m_file->read(fb.data(), fb.length());
        if (fr == -1) {
            uError(10015);return;//文件访问失败
        }
        fb.resize(fr);

        m_timer->start(60000);

        m_curTime = QDateTime::currentMSecsSinceEpoch();

        qint64 wb = m_socket->write(fb);
//        m_socket->flush();

        //可能写入失败
        if (wb < 0) {
            uError(10012);return;
        }


        return;
    }

    if (m_isQuit) {
        finishUp(1);return;
    }

    qD QString("%1_%2").arg(m_sign).arg("数据上传完成，等待返回");
    m_timer->start(60000);
    m_send = false;
}

//0删除 1改状态 2重传 3重传数次后停止
void UpWork::finishUp(int isDel)
{
    if (m_socket) {
        disconnect(m_socket,0,0,0);
        m_socket->flush();
        m_socket->abort();
    }
    if (m_file)
        m_file->close();
    qD QString("关闭上传:%1_%2").arg(m_sign).arg(TransSet::codeText(m_error));

    if (m_error != 0) {
        m_dbFile.state = TransSet::uperror;
        m_dbFile.error = m_error;
    }

    //后台hash校验失败时删除压缩文件
    if (m_error == 13010203 || 0 == isDel) {
        m_dbFile.lzipPath = zipPath();
        qD QString("删除压缩文件:%1").arg(m_dbFile.lzipPath);
        XFunc::veryDel(m_dbFile.lzipPath);
        XFunc::veryDel(m_dbFile.lzipPath + ".tmp");
        if (m_error == 13010203) {
            m_dbFile.hash.clear();
            UPDAO->UfileModTime(m_dbFile);
        }
    }

    emit UpWorkFinished(m_dbFile, isDel);
}

void UpWork::uError(int err)
{
    m_error = err;
    QList<int> retryL;retryL << 10010 << 100001 << 100005 << 100007;
    if (-1 != retryL.indexOf(m_error))
        finishUp(2);
    else
        finishUp(3);
}
