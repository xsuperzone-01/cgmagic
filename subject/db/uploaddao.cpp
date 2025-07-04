#include "uploaddao.h"

#include "config/userinfo.h"
#include "tool/xfunc.h"
#include "common/session.h"

UploadDAO* UploadDAO::m_d = NULL;

UploadDAO::UploadDAO()
{
    //务必在登录成功后构造
    m_db = QSqlDatabase::addDatabase("QSQLITE", "db_upload");
    m_db.setDatabaseName(USERINFO->getUserPath() + "/fileUpload.db");
    m_db.open();

    m_fileQ = QSqlQuery(m_db);

    createTable();

    UfileIngToWait();
}

UploadDAO::~UploadDAO()
{
}

void UploadDAO::releaseUp()
{
    m_db.close();
    if (m_d) {
        m_d->deleteLater(); m_d = NULL;
    }
}

UploadDAO *UploadDAO::inst()
{
    if (m_d == NULL) {
        m_d = new UploadDAO;
    }
    return m_d;
}

void UploadDAO::createTable()
{
    QString create = "create table up("
                     "id int,"//id
                     "ordernum varchar,"//订单号
                     "farmid int,"//农场id
                     "lpath varchar,"//路径
                     "hash varchar,"//hash
                     "modtime INTEGER,"//文件最后修改时间
                     "size INTEGER,"//大小
                     "pri INTEGER,"//时间戳
                     "state int,"//状态
                     "error int,"//错误码
                     "primary key(ordernum,lpath))";
    m_fileQ.exec(create);

    m_fileQ.prepare(QString("alter table up add orderid int"));
    m_fileQ.exec();

    create = "create table upsrc("
             "ordernum varchar,"
             "lpath varchar,"
             "primary key(ordernum))";
    m_fileQ.exec(create);
}

void UploadDAO::fileDB2Class(db_upfile &f, QSqlQuery &query)
{
    f.id = query.value("id").toInt();
    f.order = query.value("ordernum").toString();//
    f.farmid = query.value("farmid").toInt();
    f.lpath = query.value("lpath").toString();//
    f.hash = query.value("hash").toString();
    f.modtime = query.value("modtime").toLongLong();
    f.pri = query.value("pri").toLongLong();
    f.size = query.value("size").toLongLong();
    f.state = query.value("state").toInt();//
    f.error = query.value("error").toInt();//
    f.orderid = query.value("orderid").toInt();
}

QList<db_upfile> UploadDAO::allPrepare()
{
    QList<db_upfile> list;
    m_fileQ.prepare(QString("select * from up limit %1,30").arg(m_fileC));
    m_fileQ.exec();
    while (m_fileQ.next()) {
        db_upfile f;
        fileDB2Class(f, m_fileQ);
        list << f;
    }
    return list;
}

void UploadDAO::Cfile(db_upfile tmpUp, QList<db_upfile> fL)
{
    QMutexLocker locker(&m_fileM);

    TransSet::transaction(m_db, TransSet::begin);

    m_fileQ.prepare("insert into up (id,ordernum,farmid,lpath,hash,modtime,size,pri,state,error,orderid) values (?,?,?,?,?,?,?,?,?,?,?)");

    QList<db_upfile> tmp;
    QList<db_upfile>::const_iterator i;
    for (i = fL.constBegin(); i != fL.constEnd(); ++i) {
        m_fileQ.addBindValue((*i).id);
        m_fileQ.addBindValue(tmpUp.order);
        m_fileQ.addBindValue(tmpUp.farmid);
        m_fileQ.addBindValue((*i).lpath);
        m_fileQ.addBindValue((*i).hash);
        m_fileQ.addBindValue((*i).modtime);
        m_fileQ.addBindValue((*i).size);
        m_fileQ.addBindValue((*i).pri);
        m_fileQ.addBindValue((*i).state);
        m_fileQ.addBindValue((*i).error);
        m_fileQ.addBindValue((*i).orderid);
        if (m_fileQ.exec())
            tmp << (*i);
    }
    TransSet::transaction(m_db, TransSet::commit);

    emit refreshAll();
}

void UploadDAO::UfileId(db_upfile& file)
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("update up set id = ? where ordernum = ? and lpath = ?");
    m_fileQ.addBindValue(file.id);
    m_fileQ.addBindValue(file.order);
    m_fileQ.addBindValue(file.lpath);
    m_fileQ.exec();
}

db_upfile UploadDAO::RfileToUp()
{
    db_upfile f;
    f.id = -1;

    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("select * from up where state = ? limit 1");
    m_fileQ.addBindValue(TransSet::upwait);
    m_fileQ.exec();
    while (m_fileQ.next()) {
        fileDB2Class(f, m_fileQ);
    }

    return f;
}

void UploadDAO::UfileState(db_upfile &file)
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare(QString("update up set state = ? %1 where ordernum = ? and lpath = ?").arg(file.state == TransSet::uperror ? ",error = ?" : ""));
    m_fileQ.addBindValue(file.state);
    file.state == TransSet::uperror ? m_fileQ.addBindValue(file.error) : void();
    m_fileQ.addBindValue(file.order);
    m_fileQ.addBindValue(file.lpath);

    if (m_fileQ.exec())
        emit fileStateChanged(file);
}

void UploadDAO::Dfile(db_upfile &file)
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("delete from up where ordernum = ? and lpath = ?");
    m_fileQ.addBindValue(file.order);
    m_fileQ.addBindValue(file.lpath);
    if (m_fileQ.exec()) {
        emit fileDeleted(file);
    }
}

void UploadDAO::DfileByOrder(QString order)
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("delete from up where ordernum = ?");
    m_fileQ.addBindValue(order);
    if (m_fileQ.exec()) {
        emit fileDeletedByOrder(order);
    }
}

void UploadDAO::DErrorFile(QString order)
{
    QMutexLocker lock(&m_fileM);
    // 先删除临时文件
    if (order.isEmpty()) {
        m_fileQ.prepare("select * from up where state = ?");
        m_fileQ.addBindValue(TransSet::fileuperror);
    } else {
        m_fileQ.prepare("select * from up where ordernum = ? and state = ?");
        m_fileQ.addBindValue(order);
        m_fileQ.addBindValue(TransSet::fileuperror);
    }
    m_fileQ.exec();
    while (m_fileQ.next()) {
        QString maxPath = m_fileQ.value("lpath").toString();
        if (USERINFO->isMaxPath(maxPath)) {
            XFunc::veryDel(maxPath);
        }

        QString lzipPath = m_fileQ.value("lzipPath").toString();
        if (lzipPath.contains("AppData") && lzipPath.contains("Xcgmagic"))
        {
            XFunc::veryDel(lzipPath);
        }
    }
}

QList<db_upfile> UploadDAO::RallFile()
{
    QMutexLocker lock(&m_fileM);

    m_fileC = 0;
    return allPrepare();
}

QList<db_upfile> UploadDAO::RallFileExceptError()
{
    QMutexLocker lock(&m_fileM);

    m_fileC = 0;
    QList<db_upfile> list;
    m_fileQ.prepare(QString("select * from up where state <> ? and state <> ? limit %1,30").arg(m_fileC));
    m_fileQ.addBindValue(TransSet::uperror);
    m_fileQ.addBindValue(TransSet::fileuperror);
    m_fileQ.exec();
    while (m_fileQ.next()) {
        db_upfile f;
        fileDB2Class(f, m_fileQ);
        list << f;
    }
    return list;
}

QList<db_upfile> UploadDAO::RallFileError()
{
    QMutexLocker lock(&m_fileM);

    m_fileC = 0;
    QList<db_upfile> list;
    m_fileQ.prepare(QString("select * from up where state = ? or state = ? limit %1,30").arg(m_fileC));
    m_fileQ.addBindValue(TransSet::uperror);
    m_fileQ.addBindValue(TransSet::fileuperror);
    m_fileQ.exec();
    while (m_fileQ.next()) {
        db_upfile f;
        fileDB2Class(f, m_fileQ);
        list << f;
    }
    return list;
}

void UploadDAO::UfileIngToWait()
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("update up set state = ? where state<>?");
    m_fileQ.addBindValue(TransSet::upwait);
    m_fileQ.addBindValue(TransSet::fileuperror);
    m_fileQ.exec();
}

void UploadDAO::UfileModTime(db_upfile &file)
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("update up set hash = ?,modtime = ?,size = ? where ordernum = ? and lpath = ?");
    m_fileQ.addBindValue(file.hash);
    m_fileQ.addBindValue(file.modtime);
    m_fileQ.addBindValue(file.size);
    m_fileQ.addBindValue(file.order);
    m_fileQ.addBindValue(file.lpath);
    m_fileQ.exec();
}

void UploadDAO::UfileHash(db_upfile &file)
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("update up set hash = ? where ordernum = ? and lpath = ?");
    m_fileQ.addBindValue(file.hash);
    m_fileQ.addBindValue(file.order);
    m_fileQ.addBindValue(file.lpath);
    m_fileQ.exec();
}

void UploadDAO::CsrcPath(QString order, QString path)
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("insert into upsrc (ordernum,lpath) values (?,?)");
    m_fileQ.addBindValue(order);
    m_fileQ.addBindValue(path);
    m_fileQ.exec();
}


