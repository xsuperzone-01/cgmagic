#ifndef DOWNLOADMAXSET_H
#define DOWNLOADMAXSET_H
#pragma once

#include <QString>
#include <QObject>

class DownloadMaxSet
{
public:
    DownloadMaxSet();
    ~DownloadMaxSet();
};

#endif // DOWNLOADMAXSET_H

class DownloadFile{
public:
    enum class DownloadStatus {
        Queued = 1,         // 排队中
        Downloading,        // 下载中
        DownloadFailed,     // 下载失败
        DownloadCompleted,  // 下载完成
        Verifying,          // 校验中
        VerificationFailed, // 校验失败
        DownloadSuccessful  // 下载成功
    };
};


class downloadMaxFile{
public:
    int primaryId;
    int id;
    QString userId;
    QString userName;
    int convertType;
    QString taskId;
    QString name;
    int resultFileSize;
    int downStatus;
    double downProgress;
    double downSpeed;
    QString startTime;
    QString endTime;
    QString errorMessage;
    int resume;
    QString resultFileMd5;

public:
    downloadMaxFile(): primaryId(0), id(0), convertType(0), resultFileSize(0), downStatus(0), downProgress(0.0), downSpeed(0.0), resume(0) {}

    downloadMaxFile(const downloadMaxFile &other) {
        primaryId = other.primaryId;
        id = other.id;
        userId = other.userId;
        userName = other.userName;
        convertType = other.convertType;
        taskId = other.taskId;
        name = other.name;
        resultFileSize = other.resultFileSize;
        downStatus = other.downStatus;
        downProgress = other.downProgress;
        downSpeed = other.downSpeed;
        startTime = other.startTime;
        endTime = other.endTime;
        errorMessage = other.errorMessage;
        resume = other.resume;
        resultFileMd5 = other.resultFileMd5;

    }

    downloadMaxFile& operator=(const downloadMaxFile &other) {
        if (this != &other) {
            primaryId = other.primaryId;
            id = other.id;
            id = other.id;
            userId = other.userId;
            userName = other.userName;
            convertType = other.convertType;
            taskId = other.taskId;
            name = other.name;
            resultFileSize = other.resultFileSize;
            downStatus = other.downStatus;
            downProgress = other.downProgress;
            downSpeed = other.downSpeed;
            startTime = other.startTime;
            endTime = other.endTime;
            errorMessage = other.errorMessage;
            resume = other.resume;
            resultFileMd5 = other.resultFileMd5;
        }
        return *this;
    }
};

Q_DECLARE_METATYPE(downloadMaxFile);
