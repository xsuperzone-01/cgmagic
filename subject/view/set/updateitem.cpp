#include "updateitem.h"
#include "ui_updateitem.h"

#include <QDir>
#include "common/basewidget.h"
#include "tool/network.h"
#include "tool/jsonutil.h"
#include "quazip/JlCompress.h"
#include "versions/versionmanager.h"

#define RENDER_PLUGIN_ID 41

UpdateItem::UpdateItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UpdateItem),
    m_client(false),
    m_completeSize(0),
    m_totalSize(0)
{
    ui->setupUi(this);

    BaseWidget::setClass(ui->install, "noBtn");
}

UpdateItem::~UpdateItem()
{
    foreach (QPointer<QFile> wf, m_replyFile.values()) {
        if (wf) {
            wf->close();
        }
    }

    delete ui;
}

void UpdateItem::initUpdateItem(QJsonObject obj)
{
    m_obj = obj;
    ui->install->hide();

    QString nv = obj.value("version").toString();
    QString cv = obj.value("currentVersion").toString();
    QString mt = VersionManager::instance()->clientTime(obj);
    m_client = obj.value("isClient").toBool();

    ui->version->setText(tr("%3版本  %1 (%2)").arg(nv).arg(mt)
                         .arg(m_client ? tr("客户端") : tr("插件")));

    if (nv != cv) {
        ui->detail->setText(obj.value("detail").toString());
        ui->install->show();
    } else {
        ui->detail->setText(tr("已是最新版本"));
    }

    int teh = ui->detail->toPlainText().split("\n").length() * 30;
    ui->detail->setFixedHeight(teh);

    setFixedHeight(100);
    setFixedHeight(minimumSizeHint().height());
}

void UpdateItem::on_install_clicked()
{
    if (m_client) {
        ui->install->setEnabled(false);
        VersionManager::instance()->popup();
        return;
    }

    ui->install->hide();
    ui->progress->show();
    downloadProgress(0, 0);

    int softId = m_obj.value("softwareId").toInt();
    QString version = m_obj.value("version").toString();
    QString policyUrl = m_obj.value("policyUrl").toString();
    NET->get(policyUrl, [=](FuncBody f) {
        QJsonArray files = f.j.value("files").toArray();
        if (files.isEmpty()) {
            ui->install->show();
            ui->progress->setText(tr("无法获取文件"));
            return;
        }

        QString dir = QString("%1_%2/%3").arg(softId).arg(m_obj.value("softwareTag").toString()).arg(version);
        QDir().mkpath(dir);

        QNetworkAccessManager *man = new QNetworkAccessManager(this);
        foreach (QJsonValue v, files) {
            QJsonObject f = v.toObject();
            QString name = f.value("path").toString();
            QFile *wf = new QFile(dir + "/" + name, this);
            wf->open(QFile::WriteOnly|QFile::Truncate);
            QNetworkRequest req(QUrl(QString(policyUrl).replace("policy.json", name)));
            QNetworkReply *reply = man->get(req);
            m_replyFile.insert(reply, wf);
            connect(reply, &QNetworkReply::readyRead, this, &UpdateItem::writeFile);
            connect(reply, &QNetworkReply::downloadProgress, this, &UpdateItem::downloadProgress);
            connect(reply, &QNetworkReply::finished, this, &UpdateItem::replyFinished);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        }
    }, this);
}

void UpdateItem::writeFile()
{
    if (QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender())) {
        QFile *wf = m_replyFile.value(reply);
        if (wf) {
            qint64 wn = wf->write(reply->readAll());
        }
    }
}

void UpdateItem::replyFinished()
{
    if (QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender())) {
        QFile *wf = m_replyFile.value(reply);
        if (wf) {
            wf->close();
            wf->deleteLater();

            QString file = wf->fileName();
            qDebug()<< file;
            if (file.contains("plugin.zip")) {
                JlCompress::extractDir(file);
                //TODO 安装
            }
        }
        m_replyFile.remove(reply);
    }
}

void UpdateItem::downloadProgress(qint64 complete, qint64 total)
{
    m_completeSize += complete;
    m_totalSize += total;

    int pro = 0;
    if (m_totalSize != 0) {
        pro = m_completeSize * 100 / m_totalSize;
    }
    ui->progress->setText(tr("正在下载 - %1%").arg(pro));
}
