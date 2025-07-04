#include "downloadinfos.h"

DownloadInfos::DownloadInfos(QObject *parent)
    : QObject{parent},
    type(0)
{
}

DownloadInfos::~DownloadInfos(){

}

//主界面初始化-读物数据库全部文件
void DownloadInfos::readFiles()
{
    type = 1;
    QList<QJsonObject> fileInfos;
    QList<downloadMaxFile> DbFiles = DownloadMaxDb::instance()->selectAllSql();
    fileInfos = getDbFileList(DbFiles);
    for(QJsonObject &obj : fileInfos){
        int status = obj.value("downStatus").toInt();
        if(status == static_cast<int>(DownloadFile::DownloadStatus::Downloading)){
            status = static_cast<int>(DownloadFile::DownloadStatus::DownloadFailed);
        }else if(status == static_cast<int>(DownloadFile::DownloadStatus::Verifying)){
            status = static_cast<int>(DownloadFile::DownloadStatus::VerificationFailed);
        }else{}
        downloadMaxFile tmp = convertToDb(obj);
        int primaryIdDb = obj.value("primaryId").toInt();
        tmp.downStatus = status;
        obj.insert("downStatus", status);
        DownloadMaxDb::instance()->updateSql(tmp, primaryIdDb);
    }

    emit initEntirelyDownloadWidget(fileInfos, type);
}

//文件下载过程中的状态实时更新
void DownloadInfos::downFileStatusRealTime(DownloadFile::DownloadStatus downloadStatus, QJsonObject file){
    type = 2;

    //假设从数据库中的主键id为primaryIdDb;
    QString clickStartTime = file.value("clickStartTime").toString();
    int status = static_cast<int>(downloadStatus);
    file.insert("downStatus", status);

    downloadMaxFile tmp = convertToDb(file);
    if(downloadPriamryIdCache.value(clickStartTime).toInt() == 0){
        int primaryId = DownloadMaxDb::instance()->insertSql(tmp);
        downloadPriamryIdCache.insert(clickStartTime, primaryId);
    }else{
        int primaryIdDb = downloadPriamryIdCache.value(clickStartTime).toInt();
        DownloadMaxDb::instance()->updateSql(tmp, primaryIdDb);
    }

    if(file.value("isDelete").toBool()){
        int isDeletePrimaryId = file.value("primaryId").toInt();
        DownloadMaxDb::instance()->deleteSql(isDeletePrimaryId);
    }else{}

    QList<downloadMaxFile> DbFiles = DownloadMaxDb::instance()->selectAllSql();
    QList<QJsonObject> fileInfos = getDbFileList(DbFiles);
    emit initEntirelyDownloadWidget(fileInfos, type);
}

QList<QJsonObject> DownloadInfos::getDbFileList(QList<downloadMaxFile> dbFiles){
    QList<QJsonObject> fileInfos;
    for(int i = 0; i < dbFiles.size(); i++){
        downloadMaxFile tmp = dbFiles.at(i);
        QJsonObject tmpObj = convertToJson(tmp);
        fileInfos.append(tmpObj);
    }

    return fileInfos;
}

//Json转换成downloadMaxFile类型
downloadMaxFile DownloadInfos::convertToDb(QJsonObject obj){
    downloadMaxFile tmp;
    tmp.primaryId = obj.value("primaryId").toInt();
    tmp.id = obj.value("id").toInt();
    tmp.userId = obj.value("userId").toString();
    tmp.userName = obj.value("userName").toString();
    tmp.convertType = obj.value("convertType").toInt();
    tmp.taskId = obj.value("taskId").toString();
    tmp.name = obj.value("name").toString();
    tmp.resultFileSize = obj.value("resultFileSize").toInt();
    tmp.downStatus = obj.value("downStatus").toInt();
    tmp.downProgress = obj.value("downProgress").toDouble();
    tmp.downSpeed = obj.value("downSpeed").toDouble();
    tmp.startTime = obj.value("startTime").toString();
    tmp.endTime = obj.value("endTime").toString();
    tmp.errorMessage = obj.value("errorMessage").toString();
    tmp.resume = obj.value("resume").toInt();
    tmp.resultFileMd5 = obj.value("resultFileMd5").toString();

    return tmp;
}

//downloadMaxFile转换成Json类型
QJsonObject DownloadInfos::convertToJson(downloadMaxFile file){
    QJsonObject obj;
    obj.insert("primaryId", file.primaryId);
    obj.insert("id", file.id);
    obj.insert("userId", file.userId);
    obj.insert("userName", file.userName);
    obj.insert("convertType", file.convertType);
    obj.insert("taskId", file.taskId);
    obj.insert("name", file.name);
    obj.insert("resultFileSize", file.resultFileSize);
    obj.insert("downStatus", file.downStatus);
    obj.insert("downProgress", file.downProgress);
    obj.insert("downSpeed", file.downSpeed);
    obj.insert("startTime", file.startTime);
    obj.insert("endTime", file.endTime);
    obj.insert("errorMessage", file.errorMessage);
    obj.insert("resume", file.resume);
    obj.insert("resultFileMd5", file.resultFileMd5);

    return obj;
}

