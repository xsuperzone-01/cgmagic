#include "userdao.h"

#include "config/userinfo.h"

#include <QSqlRecord>
#include <QDebug>
#include <QSqlError>
#include "tool/jsonutil.h"
#include <QUuid>

PATTERN_SINGLETON_IMPLEMENT(UserDAO);

UserDAO::UserDAO(QObject *parent) :
    QObject(parent)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "db_user");
    db.setDatabaseName(USERINFO->allUserPath() + "/user.db");
    db.open();

    m_userQ = QSqlQuery(db);
    createTable();
}

UserDAO::~UserDAO()
{

}

QList<db_cloud> UserDAO::readAllCloud()
{
    QMutexLocker locker(&m_cloud_mutex);

    QList<db_cloud> list;

    m_userQ.prepare("select * from remote_connect where userid = ?");
    m_userQ.addBindValue(USERINFO->instance()->userId());
    m_userQ.exec();

    while (m_userQ.next()) {
        db_cloud cloud;
        query2Class(cloud, m_userQ);
        list.append(cloud);
    }

    return list;
}

db_cloud UserDAO::readCloudByOrder(const QString orderNum)
{
    QMutexLocker locker(&m_cloud_mutex);

    db_cloud cloud;
    m_userQ.prepare("select * from remote_connect where orderNum = ?");
    m_userQ.addBindValue(orderNum);
    m_userQ.exec();

    while (m_userQ.next()) {
        query2Class(cloud, m_userQ);
    }
    return cloud;
}

db_cloud UserDAO::readCloudByNodeId(const QString nodeId)
{
    QMutexLocker locker(&m_cloud_mutex);

    db_cloud cloud;
    m_userQ.prepare("select * from remote_connect where nodeId = ?");
    m_userQ.addBindValue(nodeId);
    m_userQ.exec();

    while (m_userQ.next()) {
        query2Class(cloud, m_userQ);
    }
    return cloud;
}

QList<db_cloud> UserDAO::readCloudByType(const QString cloud)
{
    QMutexLocker locker(&m_cloud_mutex);

    QList<db_cloud> list;

    m_userQ.prepare("select * from remote_connect where cloud = ?");
    m_userQ.addBindValue(cloud);
    m_userQ.exec();

    while (m_userQ.next()) {
        db_cloud cloud;
        query2Class(cloud, m_userQ);
        list.append(cloud);
    }

    return list;
}

void UserDAO::insertCloud(db_cloud cloud)
{
    QMutexLocker locker(&m_cloud_mutex);

    if (cloud.id.isEmpty()) {
        QUuid id = QUuid::createUuid();
        cloud.id = id.toString();
    }
    if (cloud.userid.isEmpty()) {
        cloud.userid = USERINFO->instance()->userId();
    }

    // 判断是否存在同一个订单号，如果存在，则删除
    m_userQ.prepare("delete from remote_connect where orderNum = ?");
    m_userQ.addBindValue(cloud.orderNum);
    m_userQ.exec();

    m_userQ.prepare("insert into remote_connect (id,userid,cloud,pid,proName,orderNum,windowTitle,workspaceId,nodeId) values (?,?,?,?,?,?,?,?,?)");
    m_userQ.addBindValue(cloud.id);
    m_userQ.addBindValue(cloud.userid);
    m_userQ.addBindValue(cloud.cloud);
    m_userQ.addBindValue(cloud.pid);
    m_userQ.addBindValue(cloud.proName);
    m_userQ.addBindValue(cloud.orderNum);
    m_userQ.addBindValue(cloud.windowTitle);
    m_userQ.addBindValue(cloud.workspaceId);
    m_userQ.addBindValue(cloud.nodeId);
    m_userQ.exec();
}

void UserDAO::modifyCloudPid(const QString id, const int pid)
{
    QMutexLocker locker(&m_cloud_mutex);

    m_userQ.prepare("update remote_connect set pid = ? where id = ?");
    m_userQ.addBindValue(pid);
    m_userQ.addBindValue(id);
    m_userQ.exec();
}

void UserDAO::delCloud(const QString id)
{
    QMutexLocker locker(&m_cloud_mutex);

    if (id.isEmpty()) {
        return;
    }
    m_userQ.prepare("delete from remote_connect where id = ?");
    m_userQ.addBindValue(id);
    m_userQ.exec();
}

void UserDAO::delCloudByUserid(const QString userid)
{
    QMutexLocker locker(&m_cloud_mutex);

    qDebug()<< __FUNCTION__ << userid;
    if (userid.isEmpty()) {
        return;
    }
    m_userQ.prepare("delete from remote_connect where userid = ?");
    m_userQ.addBindValue(userid);
    m_userQ.exec();
}

db_user UserDAO::readUser(bool parent)
{
    db_user u;

    m_userQ.prepare("select * from user where third = ?");
    m_userQ.addBindValue(userThird(parent));
    m_userQ.exec();

    while (m_userQ.next()) {
        u.info = m_userQ.value("info").toString();
    }
    return u;
}

void UserDAO::saveUser(db_user user, bool parent)
{
    m_userQ.prepare("delete from user where third = ?");
    m_userQ.addBindValue(userThird(parent));
    m_userQ.exec();

    m_userQ.prepare("insert into user (third,info) values (?,?)");
    m_userQ.addBindValue(userThird(parent));
    m_userQ.addBindValue(user.info);
    m_userQ.exec();
}

void UserDAO::setAllUser(const QString &key, const QVariant &value)
{
    m_userQ.prepare("delete from allUserSet where key = ?");
    m_userQ.addBindValue(key);
    m_userQ.exec();

    m_userQ.prepare("insert into allUserSet (key,value) values (?,?)");
    m_userQ.addBindValue(key);
    m_userQ.addBindValue(value);
    m_userQ.exec();
}

QVariant UserDAO::readAllUser(const QString &key)
{
    m_userQ.prepare("select * from allUserSet where key = ?");
    m_userQ.addBindValue(key);
    m_userQ.exec();

    QVariant value;
    while (m_userQ.next()) {
        value = m_userQ.value("value").toString();
    }
    return value;
}


QString UserDAO::tableCol(QSqlDatabase &db, QString table, int idx)
{
    QStringList retL;
    QStringList colL;
    QStringList qL;
    QSqlRecord record = db.record(table);
    for (int i = 0; i < record.count(); ++i) {
        colL << record.fieldName(i);
        qL << "?";
    }
    if (0 == idx)
        retL = colL;
    if (1 == idx)
        retL = qL;
    return retL.join(",");
}

void UserDAO::createTable()
{
    QString create = "create table user("
                     "third varchar,"
                     "info varchar)";
    m_userQ.exec(create);

    create = "create table remote_connect("
             "id varchar,"
             "userid varchar,"
             "cloud varchar,"
             "pid int,"
             "proName varchar,"
             "orderNum varchar,"
             "windowTitle varchar)";
    m_userQ.exec(create);
    m_userQ.prepare(QString("alter table remote_connect add workspaceId varchar"));
    m_userQ.exec();
    m_userQ.prepare(QString("alter table remote_connect add nodeId varchar"));
    m_userQ.exec();

    create = "create table allUserSet("
             "key varchar,"
             "value QVariant)";
    m_userQ.exec(create);
}

QString UserDAO::userThird(bool parent)
{
    QString t = USERINFO->instance()->thirdGroup();
    if (!parent) {
        t += "_sub";
    }
    return t;
}

void UserDAO::query2Class(db_cloud &c, QSqlQuery &q)
{
    c.id = q.value("id").toString();
    c.userid = q.value("userid").toString();
    c.cloud = q.value("cloud").toString();
    c.pid = q.value("pid").toInt();
    c.proName = q.value("proName").toString();
    c.orderNum = q.value("orderNum").toString();
    c.windowTitle = q.value("windowTitle").toString();
    c.workspaceId = q.value("workspaceId").toInt();
    c.nodeId = q.value("nodeId").toString();
}
