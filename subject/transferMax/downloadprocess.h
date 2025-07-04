#ifndef DOWNLOADPROCESS_H
#define DOWNLOADPROCESS_H
#pragma once

#include <QObject>
#include <QRunnable>
#include <QJsonObject>
#include <QPointer>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include "downloadmaxset.h"

class DownloadProcess : public QObject, public QRunnable
{
    Q_OBJECT
public:
    DownloadProcess();
    ~DownloadProcess();

signals:
    void downloadStatus(DownloadFile::DownloadStatus downloadStatus, QJsonObject downFileInfos);
    void getDownloadUrlSuccess();
    void downloadProgressEnd();
    void showFileCleared(QString tipContent); //结果文件已被清理的信号

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

protected:
    void run() override;

public:
    void setDownloadTask(QJsonObject taskInfos);
    void stopProcess();

private:
    bool vertifyFileMD5(QString filePath, QString fileMd5);
    void setDownloadStatus(qint64 status, QJsonObject obj);
    void loopGetDownloadFileUrl(QJsonObject obj, QString downloadIdUrl);
    void deleteErrorFile(QString filePath);
    QString getTimeDownloadSuccess();
    void openFileLocation(QString filePath);
    bool isFileExists(QString filePath);

private:
    QJsonObject taskJson;
    QString downloadUrl;
    QString fileMd5;
    qint64 fileSize;

    QJsonObject tmp;
    qint64 status;

    QPointer<QNetworkReply> reply;

    QEventLoop eventLoop;
    QFile file;
};

#endif // DOWNLOADPROCESS_H
