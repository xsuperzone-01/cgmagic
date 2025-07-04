#include "downloadmaxdb.h"
#include "config/userinfo.h"
#include <QDebug>
#include <QSqlError>
#include <QMutexLocker>

DownloadMaxDb* DownloadMaxDb::maxDb = nullptr;
DownloadMaxDb::DownloadMaxDb(QObject *parent)
    : QObject{parent}
{
    createDatabase();
    createMaxFileTable();
}

DownloadMaxDb::~DownloadMaxDb()
{

}

DownloadMaxDb *DownloadMaxDb::instance()
{
    if (maxDb == nullptr) {
        maxDb = new DownloadMaxDb;
    }else{}

    return maxDb;
}

void DownloadMaxDb::createDatabase(){
    downloadMaxDb = QSqlDatabase::addDatabase("QSQLITE", QString("downloadMax"));
    downloadMaxDb.setDatabaseName(UserInfo::instance()->getUserPath() + "/downloadMax.db");
    if(downloadMaxDb.open()){
        fileSql = QSqlQuery(downloadMaxDb);
        qDebug()<<"downloadMaxDb open success!";
    }else{
        qDebug()<<"downloadMaxDb open fail and error is:"<<downloadMaxDb.lastError().text();
    }
}

void DownloadMaxDb::createMaxFileTable(){
    QString table = "CREATE TABLE IF NOT EXISTS downloadMax ("
                               "primaryId INTEGER PRIMARY KEY AUTOINCREMENT, "
                               "id INTEGER, "
                               "userId VARCHAR, "
                               "userName VARCHAR, "
                               "convertType INTEGER, "
                               "taskId VARCHAR, "
                               "name VARCHAR, "
                               "resultFileSize INTEGER, "
                               "downStatus INTEGER, "
                               "downProgress DOUBLE, "
                               "downSpeed DOUBLE, "
                               "startTime VARCHAR, "
                               "endTime VARCHAR, "
                               "errorMessage VARCHAR, "
                               "resume INTEGER DEFAULT 0, "
                               "resultFileMd5 VARCHAR)";

    fileSql.exec(table);
}

int DownloadMaxDb::insertSql(downloadMaxFile file){
    QMutexLocker locker(&mutex);

    fileSql.prepare("INSERT INTO downloadMax (id, userId, userName, convertType, taskId, name, resultFileSize, downStatus, downProgress, downSpeed, startTime, endTime, errorMessage, resume, resultFileMd5) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    fileSql.addBindValue(file.id);
    fileSql.addBindValue(file.userId);
    fileSql.addBindValue(file.userName);
    fileSql.addBindValue(file.convertType);
    fileSql.addBindValue(file.taskId);
    fileSql.addBindValue(file.name);
    fileSql.addBindValue(file.resultFileSize);
    fileSql.addBindValue(file.downStatus);
    fileSql.addBindValue(file.downProgress);
    fileSql.addBindValue(file.downSpeed);
    fileSql.addBindValue(file.startTime);
    fileSql.addBindValue(file.endTime);
    fileSql.addBindValue(file.errorMessage);
    fileSql.addBindValue(file.resume);
    fileSql.addBindValue(file.resultFileMd5);
    fileSql.exec();

    QVariant lastId = fileSql.lastInsertId();
    int primaryId = lastId.toInt();

    return primaryId;
}

void DownloadMaxDb::updateSql(downloadMaxFile file, int primaryId){
    QMutexLocker locker(&mutex);

    fileSql.prepare("UPDATE downloadMax SET downStatus = ?, downProgress = ?, downSpeed = ?, startTime = ?, endTime = ?, errorMessage = ? "
                  "WHERE primaryId = ?");
    fileSql.addBindValue(file.downStatus);
    fileSql.addBindValue(file.downProgress);
    fileSql.addBindValue(file.downSpeed);
    fileSql.addBindValue(file.startTime);
    fileSql.addBindValue(file.endTime);
    fileSql.addBindValue(file.errorMessage);
    fileSql.addBindValue(primaryId);
    fileSql.exec();
}

void DownloadMaxDb::initUpdateDbSql(int downStatus, int primaryId){
    QMutexLocker locker(&mutex);

    fileSql.prepare("UPDATE downloadMax SET downStatus = ?"
                    "WHERE primaryId = ?");
    fileSql.addBindValue(downStatus);
    fileSql.addBindValue(primaryId);
    fileSql.exec();
}

void DownloadMaxDb::deleteSql(int primaryId){
    QMutexLocker locker(&mutex);

    fileSql.prepare("DELETE FROM downloadMax WHERE primaryId = ?");
    fileSql.addBindValue(primaryId);
    fileSql.exec();
}

downloadMaxFile DownloadMaxDb::selectSql(QString taskId){
    QMutexLocker locker(&mutex);

    downloadMaxFile data;
    fileSql.prepare("SELECT * FROM downloadMax WHERE taskId = ?");
    fileSql.addBindValue(taskId);

    if (fileSql.exec()) {
        if (fileSql.next()) {  // 如果有结果
            data.id = fileSql.value("id").toInt();
            data.userId = fileSql.value("userId").toString();
            data.userName = fileSql.value("userName").toString();
            data.convertType = fileSql.value("convertType").toInt();
            data.taskId = fileSql.value("taskId").toString();
            data.name = fileSql.value("name").toString();
            data.resultFileSize = fileSql.value("resultFileSize").toInt();
            data.downStatus = fileSql.value("downStatus").toInt();
            data.downProgress = fileSql.value("downProgress").toDouble();
            data.downSpeed = fileSql.value("downSpeed").toDouble();
            data.startTime = fileSql.value("startTime").toString();
            data.endTime = fileSql.value("endTime").toString();
            data.errorMessage = fileSql.value("errorMessage").toString();
            data.resume = fileSql.value("resume").toInt();
            data.resultFileMd5 = fileSql.value("resultFileMd5").toString();
        }
    }

    return data;
}

QList<downloadMaxFile> DownloadMaxDb::selectAllSql(){
    QMutexLocker locker(&mutex);

    QList<downloadMaxFile> dataList;
    fileSql.prepare("SELECT * FROM downloadMax ORDER BY primaryId DESC");

    if (fileSql.exec()) {
        while (fileSql.next()) {
            downloadMaxFile data;
            data.primaryId = fileSql.value("primaryId").toInt();
            data.id = fileSql.value("id").toInt();
            data.userId = fileSql.value("userId").toString();
            data.userName = fileSql.value("userName").toString();
            data.convertType = fileSql.value("convertType").toInt();
            data.taskId = fileSql.value("taskId").toString();
            data.name = fileSql.value("name").toString();
            data.resultFileSize = fileSql.value("resultFileSize").toInt();
            data.downStatus = fileSql.value("downStatus").toInt();
            data.downProgress = fileSql.value("downProgress").toDouble();
            data.downSpeed = fileSql.value("downSpeed").toDouble();
            data.startTime = fileSql.value("startTime").toString();
            data.endTime = fileSql.value("endTime").toString();
            data.errorMessage = fileSql.value("errorMessage").toString();
            data.resume = fileSql.value("resume").toInt();
            data.resultFileMd5 = fileSql.value("resultFileMd5").toString();

            dataList.append(data);  // 添加每条记录到列表中
        }
    }

    return dataList;  // 返回包含所有记录的列表
}
