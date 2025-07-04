#ifndef USERDAO_H
#define USERDAO_H

#include <QVariant>
#include <QSqlQuery>
#include "common/Singleton.h"

class db_user {
public:
    QString name;
    QString pwd;
    int autologin;
    int savepwd;
    QString info;
    db_user() {
        autologin = 0;
        savepwd = 0;
    }
};

class db_cloud {
public:
    QString id;
    QString userid;
    QString cloud;
    int     pid;
    QString proName;
    QString orderNum;
    int workspaceId;
    QString nodeId;
    QString windowTitle;

    db_cloud() {
        id = "";
        userid = "";
        cloud = "";
        pid = -1;
        proName = "";
        orderNum = "";
        windowTitle = "";
        workspaceId = 0;
    }
};

class UserDAO : public QObject
{
    Q_OBJECT
    PATTERN_SINGLETON_DECLARE(UserDAO);
public:

    db_user readUser(bool parent);
    void saveUser(db_user user, bool parent);

    void setAllUser(const QString &key, const QVariant &value);
    QVariant readAllUser(const QString &key);

    QList<db_cloud> readAllCloud();
    db_cloud readCloudByOrder(const QString orderNum);
    db_cloud readCloudByNodeId(const QString nodeId);
    QList<db_cloud> readCloudByType(const QString cloud);
    void insertCloud(db_cloud cloud);
    void delCloud(const QString id);
    void delCloudByUserid(const QString userid);
    void modifyCloudPid(const QString id, const int pid);


    static QString tableCol(QSqlDatabase& db, QString table, int idx);
private:
    void createTable();
    QString userThird(bool parent);
    void query2Class(db_cloud& c, QSqlQuery& q);
private:
    static UserDAO* m_d;
    QSqlQuery m_userQ;

    QMutex m_cloud_mutex;
};

#endif // USERDAO_H
