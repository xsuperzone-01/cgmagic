#ifndef DOWNLOADMAXWIDGET_H
#define DOWNLOADMAXWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QPointer>
#include <QJsonObject>
#include <QJsonArray>
#include "base/baseheader.h"

namespace Ui {
class DownloadMaxWidget;
}

class DownloadMaxWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadMaxWidget(QWidget *parent = nullptr);
    ~DownloadMaxWidget();

private slots:
    void on_searchButton_clicked();

    void on_prvBtn_clicked();

    void on_nextBtn_clicked();

    void on_jumpBtn_clicked();

    void openFile(QJsonObject obj);

private:
    void initTableView(QStringList headers);

    void populateTableData(QJsonObject json, int row);

    void updateTableData();

    QString getStatusText(int downStatus);

    void insertTableData(const QJsonObject &json, int row);

    void updateTableData(const QJsonObject &json);

public:
    void setDownloadFiles(QList<QJsonObject> fileLists);

private:
    Ui::DownloadMaxWidget *ui;

private:
    QPointer<QStandardItemModel> model;
    QPointer<BaseHeader> header;

    int pageNumbers;            // 每页显示的记录数
    int currentPage;            // 当前页
    int totalRecords;           // 总记录数
    int totalPages;             //总页数
    QList<QJsonObject> allData; // 保存所有数据
    QString storePath;          //存储路径
    QJsonObject startTimeObj;  //存储视图中文件的下载时间
};

#endif // DOWNLOADMAXWIDGET_H
