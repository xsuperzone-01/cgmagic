#ifndef DOWNLOADMAXDB_H
#define DOWNLOADMAXDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMutex>
#include <QList>
#include "transferMax/downloadmaxset.h"

class DownloadMaxDb : public QObject
{
    Q_OBJECT
public:
    explicit DownloadMaxDb(QObject *parent = nullptr);
    ~DownloadMaxDb();

    static DownloadMaxDb* instance();
    int insertSql(downloadMaxFile file);   //增删改查数据库
    void updateSql(downloadMaxFile file, int primaryId);
    void initUpdateDbSql(int downStatus, int primaryId);
    void deleteSql(int primaryId);
    downloadMaxFile selectSql(QString taskId);
    QList<downloadMaxFile> selectAllSql();

signals:

private:
    void createDatabase();       //创建数据库
    void createMaxFileTable();   //创建Max存储表

private:
    static DownloadMaxDb* maxDb;

    QSqlDatabase downloadMaxDb;
    QSqlQuery fileSql;
    QMutex mutex;
};

#endif // DOWNLOADMAXDB_H
