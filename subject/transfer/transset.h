#ifndef TRANSSET_H
#define TRANSSET_H

#include <QHash>
#include <QString>
#include <QVariant>
#include <QSqlDatabase>
#include <QThread>
#include <QObject>
#include <QFile>
#include <QCryptographicHash>

class TransferProgress {
public:
    TransferProgress() {
        completeSize = totalSize = speed = 0;
    }
    TransferProgress(const TransferProgress& d) {
        completeSize = d.completeSize;
        totalSize = d.totalSize;
        speed = d.speed;
        state = d.state;
    }
    TransferProgress& operator=(const TransferProgress& d) {
        completeSize = d.completeSize;
        totalSize = d.totalSize;
        speed = d.speed;
        state = d.state;
        return *this;
    }
    int progress() {
        int pro = 0;
        if (totalSize != 0) {
            pro = completeSize * 100 / totalSize;
        }
        return pro;
    }
    qint64 completeSize;
    qint64 totalSize;
    int speed;
    int state;
};

class db_downfile{
public:
    int id;
    QString order;
    int farmid;
    QString lpath;
    QString hash;
    qint64 size;
    qint64 pri;
    int state;
    int error;
    QString mname;
    QString camera;
    QString order2;
    QString dpath;

    TransferProgress tp;

    db_downfile() {
    }
    QString orderNum() {
        return order.split("_").first();
    }

    db_downfile(const db_downfile& d) {
        id = d.id;
        order = d.order;
        farmid = d.farmid;
        lpath = d.lpath;
        hash = d.hash;
        size = d.size;
        pri = d.pri;
        state = d.state;
        error = d.error;
        mname = d.mname;
        camera = d.camera;
        order2 = d.order2;
        tp = d.tp;
        dpath = d.dpath;
    }
    db_downfile& operator=(const db_downfile& d) {
        id = d.id;
        order = d.order;
        farmid = d.farmid;
        lpath = d.lpath;
        hash = d.hash;
        size = d.size;
        pri = d.pri;
        state = d.state;
        error = d.error;
        mname = d.mname;
        camera = d.camera;
        order2 = d.order2;
        tp = d.tp;
        dpath = d.dpath;
        return *this;
    }

};
Q_DECLARE_METATYPE(db_downfile)

class db_upfile{
public:
    int id;
    QString order;
    int farmid;
    QString lpath;
    QString hash;
    qint64 modtime;
    qint64 size;
    qint64 pri;
    int state;
    int error;
    int orderid;

    //----
    QString lzipPath;
    qint64 zipSize;
    TransferProgress tp;

    db_upfile() {
        id = 0;
        farmid = 0;
        state = 10;
        error = 0;
        zipSize = 0;
    }
    db_upfile(const db_upfile& d) {
        id = d.id;
        order = d.order;
        farmid = d.farmid;
        lpath = d.lpath;
        hash = d.hash;
        modtime = d.modtime;
        pri = d.pri;
        size = d.size;
        state = d.state;
        error = d.error;
        lzipPath = d.lzipPath;
        zipSize = d.zipSize;
        orderid = d.orderid;
        tp = d.tp;
    }
    db_upfile& operator=(const db_upfile& d) {
        id = d.id;
        order = d.order;
        farmid = d.farmid;
        lpath = d.lpath;
        hash = d.hash;
        modtime = d.modtime;
        pri = d.pri;
        size = d.size;
        state = d.state;
        error = d.error;
        lzipPath = d.lzipPath;
        zipSize = d.zipSize;
        orderid = d.orderid;
        tp = d.tp;
        return *this;
    }
};
Q_DECLARE_METATYPE(db_upfile)

class Farm  {
public:
    int id;
    QString domain;
    QString ip;
    int port;
    int error;
    Farm() {
        id = -1;
        port = -1;
        error = 0;
    }
    Farm(const Farm& d) {
        id = d.id;
        domain = d.domain;
        ip = d.ip;
        port = d.port;
        error = d.error;
    }
    Farm& operator=(const Farm& d) {
        id = d.id;
        domain = d.domain;
        ip = d.ip;
        port = d.port;
        error = d.error;
        return *this;
    }
};

class DownloadUrl {
public:
    int status;
    QString url;
    DownloadUrl() {
        status = 0;
    }
};

class TransSet : public QObject
{
    Q_OBJECT
public:
    enum FileState {
        downwait = 0,//等待下载
        downing = 1,//下载中
        downpause = 2,//下载暂停
        downerror = 3,//下载异常

        upwait = 10,//等待上传
        upping = 11,//上传中
        uppause = 12,//上传暂停
        uperror = 13,//上传异常

        missionUp = 20,//任务可上传
        missionNoUp = 21,//任务不可上传，已被取消

        fileuperror = 30,//重传数次后可删除
        filedownerror = 31
    };

    enum transact {
        begin,
        rollback,
        commit
    };

    enum ItemData{
        QtUserRole = 0x0100,
        farmid,
        farmname,
        filepath,
        missid,
        missname,
        fileid,
        order,
        state,
        downfile
    };

public:
    TransSet();
    static QString codeText(int code);
    static void transaction(QSqlDatabase& db, transact type);
    static QString transState(int state, int error = -1);
    static QString fileMD5(QFile *file, bool& pause);
    static QByteArray protocol(int sn, int funcCode, int dataType, qint64 dataLen, QByteArray byte);
    static int dataSize(int size, int time);
};
#endif // TRANSSET_H
