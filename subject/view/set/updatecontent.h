#ifndef UPDATECONTENT_H
#define UPDATECONTENT_H

#include <QWidget>
#include <QJsonObject>
#include <QNetworkReply>
#include <QPointer>
#include <QFile>
#include "common/baseclickwidget.h"

namespace Ui {
class UpdateContent;
}

class UpdateContent : public QWidget
{
    Q_OBJECT

public:
    enum UpdateStatus {
        ContinueInstall,
        UpdatePlugin,
        ContinueUpdate,
        UpdateError
    };

    explicit UpdateContent(QWidget *parent = nullptr);
    ~UpdateContent();

    void initUpdateClient();
    void initUpdateModule();
    void update(QJsonObject obj);
    void update_clicked();
    void Allunclicked();
    void Allclicked();
    void Bottomtype();

    void ChangeStatus(UpdateStatus u);
    void setUpdateContentShow(bool isContentShow);

signals:
    void updated(bool o);
    void updateModule();
    void btnEnable();
    void btnabled();

private slots:
    void writeFile();
    void replyFinished();
    void downloadProgress(qint64 complete, qint64 total);

private:
    Ui::UpdateContent *ui;
    QPointer<QTimer> count;
    int pro;
    bool m_client;
    QJsonObject m_obj;
    QMap<QPointer<QNetworkReply>, QPointer<QFile>> m_replyFile;
    qint64 m_completeSize;
    qint64 m_totalSize;
    QString version;//当前版本号
    QString compressPath;//压缩包路径
    QString policyUrl;
    QString path;//AllUser文件夹，解压文件夹
    QString postfix;//后缀
    UpdateStatus updateStatus;

private:
    void updateFinished();
    void updateFail();
    void updateSave();
    void versionSave();
    void updateNext();
    void updateInstall();
    bool compareHash(QJsonObject f);
    void download();
    void retry();
    void timeProgress();
//    void zeroProgress(); 
};

#endif // UPDATECONTENT_H
