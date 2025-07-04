#include "downloadprocess.h"
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QUrl>
#include <QFile>
#include <QCryptographicHash>
#include <QTimer>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include "config/userinfo.h"
#include "tool/network.h"
#include "Windows.h"

DownloadProcess::DownloadProcess():
    status(0)
{
}

DownloadProcess::~DownloadProcess(){

}

void DownloadProcess::setDownloadTask(QJsonObject taskInfos){
    fileSize = taskInfos.value("resultFileSize").toInt();
    fileMd5 = taskInfos.value("resultFileMd5").toString();
    qDebug()<<"fileSize and fileMd5 is:"<<fileSize<<" "<<fileMd5;

    tmp = taskInfos;
    QString url = UserInfo::instance()->getDownloadUrl() ;
    int id = taskInfos.value("id").toInt();
    QString fullUrl = url + QString("/cgmagic-console/task/conversion/result/download/id?id=%1").arg(QString::number(id));

    NET->get(fullUrl, [=](FuncBody f){
        int code = f.j.value("code").toInt();
        if(code == 200){
            QJsonObject data = f.j.value("data").toObject().value("data").toObject();
            int downloadId = data.value("downloadId").toInt();
            int id = data.value("id").toInt();
            QString downloadIdUrl = url + QString("/cgmagic-console/task/conversion/result/download/url?id=%1&downloadId=%2").arg(QString::number(id)).arg(QString::number(downloadId));
            loopGetDownloadFileUrl(tmp, downloadIdUrl);
        }else{
            qDebug()<<"Download error is:"<<f.errMsg;
        }
    }, this);
}

void DownloadProcess::loopGetDownloadFileUrl(QJsonObject obj, QString downloadIdUrl){
    QString tmpUrl = downloadIdUrl;
    QJsonObject tmpObj = obj;

    downloadUrl.clear();
    FuncBody f = NET->sync(GET, downloadIdUrl, "", 10);
    int code = f.j.value("data").toObject().value("code").toInt();
    if( code != 1){
        QString tipsContent = f.j.value("data").toObject().value("message").toString();
        emit showFileCleared(tipsContent);
        return;
    }else{
        downloadUrl = f.j.value("data").toObject().value("data").toObject().value("downloadUrl").toString();
        qDebug()<<"downloadUrl is:"<<downloadUrl;
        if(!downloadUrl.isEmpty()){
            emit getDownloadUrlSuccess();
            status = 2;
            setDownloadStatus(status, tmpObj);
            return;
        }else{
            QTimer::singleShot(1000, std::bind(&DownloadProcess::loopGetDownloadFileUrl, this, tmpObj, tmpUrl));
        }
    }
}

void DownloadProcess::run() {
    QNetworkAccessManager manager;
    QNetworkRequest request(downloadUrl);
    reply = manager.get(request);

    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));

    status = 4;
    setDownloadStatus(status, tmp);
    QString fileStorePath;
    QString filename = tmp.value("name").toString();
    fileStorePath = tmp.value("storePath").toString() + "\\" + filename;

    QFile file(fileStorePath);
    file.open(QIODevice::WriteOnly);
    connect(reply, &QNetworkReply::readyRead, [&]() {
        file.write(reply->readAll());
    });

    connect(reply, &QNetworkReply::errorOccurred, [&](QNetworkReply::NetworkError code) {
        status = 3;
        QString replyError = reply->errorString();
        tmp.insert("errorMessage", replyError);
        qDebug()<<"reply error is:"<<replyError;
    });
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    file.close();
    if(vertifyFileMD5(fileStorePath, fileMd5)){
        qDebug()<<"Download file successful!";
        QString endTime = getTimeDownloadSuccess();
        tmp.insert("endTime", endTime);
        status = 7;
    }else{
        qDebug()<<"Vertify failure and delete this file";
        deleteErrorFile(fileStorePath);
        status = 6;
    }

    if(status == 7){
        openFileLocation(fileStorePath);
    }else{}

    setDownloadStatus(status, tmp);
    reply->deleteLater();

    emit downloadProgressEnd();
}

void DownloadProcess::stopProcess(){
    if(reply && reply->isRunning()){
        reply->abort();
    }
}

bool DownloadProcess::isFileExists(QString filePath){
    if (QFile::exists(filePath)) {
        return true;
    } else {
        return false;
    }
}

void DownloadProcess::openFileLocation(QString filePath){
    if (QFile::exists(filePath)) {
        QString command = QString("/select,\"%1\"").arg(QDir::toNativeSeparators(filePath));
        ShellExecute(NULL, L"open", L"explorer.exe", command.toStdWString().c_str(), NULL, SW_SHOWNORMAL);
    } else {
        qDebug() << "File does not exist:" << filePath;
    }
}

//获取文件下载成功的最后结束时间
QString DownloadProcess::getTimeDownloadSuccess(){
    QDateTime currentDateTime = QDateTime::currentDateTime();
    return currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
}

//删除因校验MD5错误的文件
void DownloadProcess::deleteErrorFile(QString filePath){
    QFile file(filePath);
    if (file.exists()) {
        if (file.remove()) {
            qDebug() << "File deleted successfully:" << filePath;
        } else {
            qDebug() << "Failed to delete file:" << filePath;
        }
    } else {
        qDebug() << "File does not exist:" << filePath;
    }
}

void DownloadProcess::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal){
    static QTime lastUpdate = QTime::currentTime();
    int elapsed = lastUpdate.msecsTo(QTime::currentTime());
    if(elapsed < 1500){
        return;
    }else{}
    lastUpdate = QTime::currentTime();

    double progress = 0.0;
    double speed = 0.0;
    if(bytesTotal > 0){
        progress = static_cast<double>(bytesReceived) / bytesTotal * 100.0;
        speed = static_cast<double>(bytesTotal-bytesReceived)/1024/1024;
    }else{}

    tmp.insert("downProgress", progress);
    tmp.insert("downSpeed", speed);

    status = 2;
    setDownloadStatus(status, tmp);
}

bool DownloadProcess::vertifyFileMD5(QString filePath, QString fileMd5){
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for MD5" << file.errorString();
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    if (!hash.addData(&file)) {
        return false;
    }

    file.close();
    QString md5 = hash.result().toHex();
    qDebug()<<"md5 is:"<<md5;
    return md5 == fileMd5;
}

void DownloadProcess::setDownloadStatus(qint64 status, QJsonObject obj){
    DownloadFile::DownloadStatus downStatus;

    switch (status) {
    case 1:
        downStatus = DownloadFile::DownloadStatus::Queued;
        break;
    case 2:
        downStatus = DownloadFile::DownloadStatus::Downloading;
        break;
    case 3:
        downStatus = DownloadFile::DownloadStatus::DownloadFailed;
        break;
    case 4:
        downStatus = DownloadFile::DownloadStatus::DownloadCompleted;
        break;
    case 5:
        downStatus = DownloadFile::DownloadStatus::Verifying;
        break;
    case 6:
        downStatus = DownloadFile::DownloadStatus::VerificationFailed;
        break;
    case 7:
        downStatus = DownloadFile::DownloadStatus::DownloadSuccessful;
        break;
    default:
        break;
    }

    emit downloadStatus(downStatus, obj);
}

