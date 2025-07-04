#include "downloaddao.h"

#include "tool/xfunc.h"
#include "Common/session.h"

#include <QSqlQueryModel>
#include <QSqlTableModel>

DownloadDAO* DownloadDAO::m_d = NULL;
DownloadDAO::DownloadDAO(QObject* parent) :
    QObject(parent)
{
    //务必在登录成功后构造
    m_db = QSqlDatabase::addDatabase("QSQLITE", QString("db_download"));
    m_db.setDatabaseName(USERINFO->getUserPath() + "/fileDown.db");
    m_db.open();

    m_fileQ = QSqlQuery(m_db);

    createTable();
    UfileIngToWait();
}

DownloadDAO *DownloadDAO::inst()
{
    if (m_d == NULL) {
        m_d = new DownloadDAO;
    }
    return m_d;
}

DownloadDAO::~DownloadDAO()
{

}

void DownloadDAO::releaseDown()
{
    m_db.close();
    if (m_d) {
        m_d->deleteLater(); m_d = NULL;
    }
}

void DownloadDAO::createTable()
{
    QString create = "create table down("
                     "id int,"//文件id
                     "ordernum varchar,"//订单号
                     "farmid int,"//农场id
                     "lpath varchar,"//本地路径
                     "hash varchar,"//hash
                     "size INTEGER,"//大小
                     "pri INTEGER,"//优先级
                     "state int,"//状态
                     "error int,"//错误码
                     "primary key(ordernum,id))";
    m_fileQ.exec(create);

    m_fileQ.prepare(QString("alter table down add mname varchar"));
    m_fileQ.exec();
    m_fileQ.prepare(QString("alter table down add camera varchar"));
    m_fileQ.exec();
    m_fileQ.prepare(QString("alter table down add order2 varchar"));
    m_fileQ.exec();
    m_fileQ.prepare(QString("alter table down add dpath varchar"));
    m_fileQ.exec();

    {
        QList<db_downfile> list;
        m_fileQ.prepare("select * from down where order2 is null");
        m_fileQ.exec();
        while (m_fileQ.next()) {
            db_downfile f;
            fileDB2Class(f, m_fileQ);
            list << f;
        }
        for (int i = 0; i < list.length(); i++) {
            db_downfile f = list.at(i);
            m_fileQ.prepare("update down set order2 = ? where ordernum = ?");
            m_fileQ.addBindValue(f.orderNum());
            m_fileQ.addBindValue(f.order);
            m_fileQ.exec();
        }
    }

    create = "create table unity("
                     "ordernum varchar,"//订单号
                     "lpath varchar,"//本地下载路径
                     "downok int,"//下载完成
                     "downsum int,"//下载总数
                     "primary key(ordernum))";
    create = "create table ResultDir("
                     "ordernum varchar,"//订单号
                     "lpath varchar,"//本地下载路径
                     "primary key(ordernum))";
    m_fileQ.exec(create);
}

void DownloadDAO::fileDB2Class(db_downfile& f, QSqlQuery& query)
{
    f.id = query.value("id").toInt();
    f.order = query.value("ordernum").toString();
    f.farmid = query.value("farmid").toInt();
    f.lpath = query.value("lpath").toString();
    f.hash = query.value("hash").toString();
    f.size = query.value("size").toLongLong();
    f.pri = query.value("pri").toLongLong();
    f.state = query.value("state").toInt();
    f.error = query.value("error").toInt();
    f.mname = query.value("mname").toString();
    f.camera = query.value("camera").toString();
    f.order2 = query.value("order2").toString();
    f.dpath = query.value("dpath").toString();
}

QList<db_downfile> DownloadDAO::allPrepare()
{
    QList<db_downfile> list;
    m_fileQ.prepare(QString("select * from down where order2 like ? or mname like ? limit %1,60").arg(m_fileC));
    m_fileQ.addBindValue(QString("%%1%").arg(m_search));
    m_fileQ.addBindValue(QString("%%1%").arg(m_search));
    m_fileQ.exec();
    while (m_fileQ.next()) {
        db_downfile f;
        fileDB2Class(f, m_fileQ);
        list << f;
    }
     return list;
}

db_downfile DownloadDAO::RfileToDown()
{
    db_downfile f;
    f.id = -1;

    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("select * from down where state = ? limit 1");
    m_fileQ.addBindValue(TransSet::downwait);
    m_fileQ.exec();
    while (m_fileQ.next()) {
        fileDB2Class(f, m_fileQ);
    }

    return f;
}

void DownloadDAO::UfileIngToWait()
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("update down set state = ? where state<>?");
    m_fileQ.addBindValue(TransSet::downwait);
    m_fileQ.addBindValue(TransSet::filedownerror);
    m_fileQ.exec();
}

void DownloadDAO::Cfile(QList<db_downfile> fL)
{
    QMutexLocker locker(&m_fileM);

    TransSet::transaction(m_db, TransSet::begin);

    m_fileQ.prepare("insert into down (id,ordernum,farmid,lpath,hash,size,pri,state,error,mname,camera,order2,dpath) values (?,?,?,?,?,?,?,?,?,?,?,?,?)");

    QList<db_downfile> tmp;
    QList<db_downfile>::const_iterator i;
    for (i = fL.constBegin(); i != fL.constEnd(); ++i) {
        m_fileQ.addBindValue((*i).id);
        m_fileQ.addBindValue((*i).order);
        m_fileQ.addBindValue((*i).farmid);
        m_fileQ.addBindValue((*i).lpath);
        m_fileQ.addBindValue((*i).hash);
        m_fileQ.addBindValue((*i).size);
        m_fileQ.addBindValue((*i).pri);
        m_fileQ.addBindValue((*i).state);
        m_fileQ.addBindValue((*i).error);
        m_fileQ.addBindValue((*i).mname);
        m_fileQ.addBindValue((*i).camera);
        m_fileQ.addBindValue((*i).order2);
        m_fileQ.addBindValue((*i).dpath);
        if (m_fileQ.exec())
            tmp << (*i);
    }

    TransSet::transaction(m_db, TransSet::commit);

    emit refreshAll();
}

void DownloadDAO::UfileState(db_downfile& file)
{
    QMutexLocker locker(&m_fileM);

    m_fileQ.prepare(QString("update down set state = ? %2 where id = ? and ordernum = ?").arg(file.state == TransSet::downerror ? ",error = ?" : ""));
    m_fileQ.addBindValue(file.state);
    file.state == TransSet::downerror ? m_fileQ.addBindValue(file.error) : void();
    m_fileQ.addBindValue(file.id);
    m_fileQ.addBindValue(file.order);
    if (m_fileQ.exec())
        emit fileStateChanged(file);
}

void DownloadDAO::UfileDownloadPath(db_downfile file)
{
    QMutexLocker locker(&m_fileM);

    m_fileQ.prepare("update down set dpath = ? where id = ? and ordernum = ?");
    m_fileQ.addBindValue(file.dpath);
    m_fileQ.addBindValue(file.id);
    m_fileQ.addBindValue(file.order);
    m_fileQ.exec();
}

void DownloadDAO::Dfile(db_downfile& file)
{
    QMutexLocker locker(&m_fileM);

    m_fileQ.prepare("delete from down where id = ? and ordernum = ?");
    m_fileQ.addBindValue(file.id);
    m_fileQ.addBindValue(file.order);
    if (m_fileQ.exec()) {
        emit fileDeleted(file);
    }
}

void DownloadDAO::Dfiles(QList<db_downfile> fileL)
{
    QMutexLocker locker(&m_fileM);

    for (int i = 0; i < fileL.length(); i++) {
        db_downfile file = fileL.at(i);
        m_fileQ.prepare("delete from down where id = ? and ordernum = ?");
        m_fileQ.addBindValue(file.id);
        m_fileQ.addBindValue(file.order);
        if (m_fileQ.exec()) {
            emit fileDeleted(file);
        }
    }

    emit refreshAll();
}

void DownloadDAO::DErrorFile()
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("delete from down where state = ?");
    m_fileQ.addBindValue(TransSet::filedownerror);
    m_fileQ.exec();
}

QList<db_downfile> DownloadDAO::RallFile()
{
    QMutexLocker lock(&m_fileM);

    m_fileC = 0;
    return allPrepare();
}

void DownloadDAO::changeSearch(QString search)
{
    m_search = search;
}

void DownloadDAO::Cunity(QString order, QString downPath)
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("insert into unity (ordernum,lpath,downok,downsum) values (?,?,?,?)");
    m_fileQ.addBindValue(order);
    m_fileQ.addBindValue(downPath);
    m_fileQ.addBindValue(0);
    m_fileQ.addBindValue(0);
    m_fileQ.exec();
}

QString DownloadDAO::RunityPath(QString order)
{
    QMutexLocker lock(&m_fileM);

    QString path;
    m_fileQ.prepare("select lpath from unity where ordernum = ?");
    m_fileQ.addBindValue(order);
    m_fileQ.exec();
    while (m_fileQ.next()) {
        path = m_fileQ.value("lpath").toString();
    }
    return path;
}

void DownloadDAO::Uu3dOk(QString order)
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("update unity set downok = downok + 1 where ordernum = ?");
    m_fileQ.addBindValue(order);
    m_fileQ.exec();
}

void DownloadDAO::Uu3dSum(QString order, int sum)
{
    QMutexLocker lock(&m_fileM);

    m_fileQ.prepare("update unity set downsum = ? where ordernum = ?");
    m_fileQ.addBindValue(sum);
    m_fileQ.addBindValue(order);
    m_fileQ.exec();
}

QString DownloadDAO::Ru3dFinish()
{
    QMutexLocker lock(&m_fileM);

    QString order;
    QString ret;
    m_fileQ.prepare("select ordernum,lpath from unity where downsum <> 0 and downok = downsum");
    m_fileQ.exec();
    while (m_fileQ.next()) {
        order = m_fileQ.value("ordernum").toString();
        ret = m_fileQ.value("lpath").toString();
    }
    if (!ret.isEmpty()) {
        m_fileQ.prepare("delete from unity where ordernum = ?");
        m_fileQ.addBindValue(order);
        m_fileQ.exec();
    }
    return ret;
}

void DownloadDAO::CResultDir(QString order, QString downPath)
{
    QMutexLocker lock(&m_fileM);
    m_fileQ.prepare("insert into ResultDir (ordernum,lpath) values (?,?)");
    m_fileQ.addBindValue(order);
    m_fileQ.addBindValue(downPath);
    m_fileQ.exec();
}

QString DownloadDAO::ResultDir(QString order)
{
    QMutexLocker lock(&m_fileM);

    QString path;
    m_fileQ.prepare("select lpath from ResultDir where ordernum = ?");
    m_fileQ.addBindValue(order);
    m_fileQ.exec();
    while (m_fileQ.next()) {
        path = m_fileQ.value("lpath").toString();
    }
    return path;
}

void DownloadDAO::DelResultDir(QString order)
{
    QMutexLocker lock(&m_fileM);
    m_fileQ.prepare("delete from unity where ordernum = ?");
    m_fileQ.addBindValue(order);
    m_fileQ.exec();
    return;
}
