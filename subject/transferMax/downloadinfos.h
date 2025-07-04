#ifndef DOWNLOADINFOS_H
#define DOWNLOADINFOS_H
#pragma once

#include <QObject>
#include "db/downloadmaxdb.h"
#include <QJsonObject>

class DownloadInfos : public QObject
{
    Q_OBJECT
public:
    explicit DownloadInfos(QObject *parent = nullptr);
    ~DownloadInfos();

signals:
    void initEntirelyDownloadWidget(QList<QJsonObject> fileLists, int type);

private:
    QJsonObject convertToJson(downloadMaxFile file);
    downloadMaxFile convertToDb(QJsonObject obj);
    QList<QJsonObject> getDbFileList(QList<downloadMaxFile> dbFiles);

public slots:
    void readFiles();
    void downFileStatusRealTime(DownloadFile::DownloadStatus downloadStatus, QJsonObject file);

private:
    int type;   //type==1表示登录界面初始化一次性的，type==2表示下载过程的状态实时初始化

    QJsonObject downloadPriamryIdCache;
};

#endif // DOWNLOADINFOS_H
