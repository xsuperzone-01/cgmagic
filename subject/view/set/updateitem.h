#ifndef UPDATEITEM_H
#define UPDATEITEM_H

#include <QWidget>
#include <QJsonObject>
#include <QNetworkReply>
#include <QPointer>
#include <QFile>

namespace Ui {
class UpdateItem;
}

class UpdateItem : public QWidget
{
    Q_OBJECT

public:
    explicit UpdateItem(QWidget *parent = nullptr);
    ~UpdateItem();

    void initUpdateItem(QJsonObject obj);

private slots:
    void on_install_clicked();
    void writeFile();
    void replyFinished();
    void downloadProgress(qint64 complete, qint64 total);

private:
    Ui::UpdateItem *ui;

    bool m_client;
    QJsonObject m_obj;
    QMap<QPointer<QNetworkReply>, QPointer<QFile>> m_replyFile;
    qint64 m_completeSize;
    qint64 m_totalSize;
};

#endif // UPDATEITEM_H
