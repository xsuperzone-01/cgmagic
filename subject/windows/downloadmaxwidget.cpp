#include "downloadmaxwidget.h"
#include "ui_downloadmaxwidget.h"
#include <QDebug>
#include <QProgressBar>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include "config/userinfo.h"
#include <QFile>
#include "Windows.h"
#include <QDir>
#include <QEventLoop>
#include <QTimer>

DownloadMaxWidget::DownloadMaxWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DownloadMaxWidget)
    , pageNumbers(10)
    , currentPage(0)
    , totalPages(0)
    , totalRecords(0)
{
    ui->setupUi(this);

    ui->widget_6->hide();
    this->hide();
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->horizontalHeader()->setVisible(true);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->tableView->setShowGrid(false);
    ui->tableView->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView->setFocusPolicy(Qt::NoFocus);

    QStringList headers;
    headers<<tr("文件名称")<<tr("下载时间")<<tr("结束时间")<<tr("下载状态")<<tr("下载速度")<<tr("下载进度")<<tr("操作");

    storePath = UserInfo::instance()->readAllIni(QString("Set"), QString("cacheDir")).toString() + QString("\\%1").arg(QString("ConvertDownloads"));

    model = new QStandardItemModel(pageNumbers, headers.size(), this);
    header = new BaseHeader(Qt::Horizontal, ui->tableView);
    initTableView(headers);
}

DownloadMaxWidget::~DownloadMaxWidget()
{
    delete ui;
}

void DownloadMaxWidget::initTableView(QStringList headers){
    int rowFixedHeight = 50;
    model->setHorizontalHeaderLabels(headers);
    ui->tableView->setModel(model);
    ui->tableView->setHorizontalHeader(header);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->verticalHeader()->setDefaultSectionSize(rowFixedHeight);
}

void DownloadMaxWidget::setDownloadFiles(QList<QJsonObject> fileLists){
    allData.clear();
    allData = fileLists;
    totalRecords = allData.size();
    updateTableData();
}

void DownloadMaxWidget::updateTableData() {
    int startIndex = currentPage * pageNumbers;
    int endIndex = qMin(startIndex + pageNumbers, totalRecords);

    for (int i = 0; i < totalRecords; ++i) {
        populateTableData(allData.at(i), i);
    }

    // ui->prvBtn->setEnabled(currentPage > 0);
    // ui->nextBtn->setEnabled(currentPage < (totalRecords + pageNumbers - 1) / pageNumbers - 1);

    // totalPages = (totalRecords + pageNumbers - 1) / pageNumbers;
    // ui->pageShowLab->setText(QString("当前%1页/总共%2页").arg(currentPage + 1).arg(totalPages));
}

void DownloadMaxWidget::populateTableData(QJsonObject json, int row) {
    QStringList startTimeList = startTimeObj.keys();
    if(!startTimeList.contains(json.value("startTime").toString())){
        startTimeObj.insert(json.value("startTime").toString(), json.value("primaryId").toInt());
        model->insertRow(row);
        insertTableData(json, row);
    }else{
        //检测正在下载中的数据才进行更新
        if(!json.value("isInitRead").toBool()){
            updateTableData(json);
        }else{}
    }
}

void DownloadMaxWidget::updateTableData(const QJsonObject &json) {
    QString startTime = json.value("startTime").toString();
    QModelIndex startTimeIndex;
    int row = 0;

    for (int i = 0; i < model->rowCount(); ++i) {
        startTimeIndex = model->index(i, 1);
        if (model->data(startTimeIndex).toString() == startTime) {
            row = i;
        }
    }

    // 更新结束时间
    QString endTime = json.value("endTime").toString();
    QStandardItem *endTimeItem = model->item(row, 2);
    endTimeItem->setText(endTime);

    // 更新下载状态
    int downStatus = json.value("downStatus").toInt();
    QString downStatusText = getStatusText(downStatus);
    QStandardItem *downStatusItem = model->item(row, 3);
    downStatusItem->setText(downStatusText);

    // 更新速度
    double speed = json.value("downSpeed").toDouble();
    QStandardItem *speedItem = model->item(row, 4);
    speedItem->setText(QString("%1kb/s").arg(QString::number(speed, 'f', 2)));

    // 更新进度条
    double progressValue = json.value("downProgress").toDouble();
    if (downStatus == 7) {
        progressValue = 100.00;
    }
    QProgressBar *progressBar = qobject_cast<QProgressBar*>(ui->tableView->indexWidget(model->index(row, 5))->findChild<QProgressBar*>());
    if (progressBar) {
        progressBar->setValue(qRound(progressValue));
    }

    if(qRound(progressValue) == 100){
        double downloadSpeed = 0.0;
        speedItem->setText(QString("%1kb/s").arg(QString::number(downloadSpeed, 'f', 2)));
        QPushButton *actionButton = new QPushButton(tr("打开目录"));
        actionButton->setObjectName(json.value("startTime").toString());
        QString filePath = storePath + "\\" + json.value("name").toString();
        if(QFile::exists(filePath) || downStatus != 7){
            actionButton->setEnabled(true);
            actionButton->setStyleSheet("background-color: #33A4FF; color: white;");
            connect(actionButton, &QPushButton::clicked, this, [this, json]() {
                openFile(json);
            });
        }else{
            actionButton->setEnabled(false);
            actionButton->setStyleSheet("background-color: rgba(255,255,255,0.1); color: #2C2D31;");
        }

        actionButton->setFixedSize(80, 25);
        QWidget *buttonContainer = new QWidget(this);
        QVBoxLayout *buttonLayout = new QVBoxLayout(buttonContainer);
        buttonLayout->addWidget(actionButton);
        buttonLayout->setAlignment(Qt::AlignCenter);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        ui->tableView->setIndexWidget(model->index(row, 6), buttonContainer);
    }else{
    }
}

void DownloadMaxWidget::insertTableData(const QJsonObject &json, int row){
    //文件名称元素
    QStandardItem *fileNameItem = new QStandardItem(json.value("name").toString());
    fileNameItem->setTextAlignment(Qt::AlignCenter);
    model->setItem(row, 0, fileNameItem);

    //开始时间元素
    QStandardItem *startTimeItem = new QStandardItem(json.value("startTime").toString());
    startTimeItem->setTextAlignment(Qt::AlignCenter);
    model->setItem(row, 1, startTimeItem);

    //结束时间元素
    QStandardItem *endTimeItem = new QStandardItem(json.value("endTime").toString());
    endTimeItem->setTextAlignment(Qt::AlignCenter);
    model->setItem(row, 2, endTimeItem);

    //状态元素
    int downStatus = json.value("downStatus").toInt();
    QString downStatusText = getStatusText(downStatus);
    QStandardItem *downStatusItem = new QStandardItem(downStatusText);
    downStatusItem->setTextAlignment(Qt::AlignCenter);
    model->setItem(row, 3, downStatusItem);

    //速度元素
    double speed = json.value("downSpeed").toDouble();
    speed = 0.0;
    QStandardItem *speedItem = new QStandardItem(QString("%1kb/s").arg(QString::number(speed, 'f', 2)));
    speedItem->setTextAlignment(Qt::AlignCenter);
    model->setItem(row, 4, speedItem);

    //进度条元素
    QProgressBar *progressBar = new QProgressBar(this);
    double progressValue = json.value("downProgress").toDouble();
    if(downStatus == 7){
        progressValue = 100.00;
    }else{}
    progressBar->setValue(qRound(progressValue));
    progressBar->setTextVisible(false);
    progressBar->setFixedSize(100, 20);
    QWidget *progressContainer = new QWidget(this);
    QVBoxLayout *progressLayout = new QVBoxLayout(progressContainer);
    progressLayout->addWidget(progressBar);
    progressLayout->setAlignment(Qt::AlignCenter);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    ui->tableView->setIndexWidget(model->index(row, 5), progressContainer);

    //操作按钮元素
    if(json.value("downStatus").toInt() != 2 && json.value("downStatus").toInt() != 1){
        QPushButton *actionButton = new QPushButton(tr("打开目录"));
        actionButton->setObjectName(json.value("startTime").toString());
        if(qRound(progressValue) == 100 || downStatus == 7){
            QString filePath = storePath + "\\" + json.value("name").toString();
            if(QFile::exists(filePath)){
                actionButton->setEnabled(true);
                actionButton->setStyleSheet("background-color: #33A4FF; color: white;");
                connect(actionButton, &QPushButton::clicked, this, [this, json]() {
                    openFile(json);
                });
            }else{
                actionButton->setEnabled(false);
                actionButton->setStyleSheet("background-color: rgba(255,255,255,0.1); color: #2C2D31;");
            }

        }else{
            actionButton->setEnabled(false);
            actionButton->setStyleSheet("background-color: rgba(255,255,255,0.1); color: #2C2D31;");
        }
        actionButton->setFixedSize(80, 25);
        QWidget *buttonContainer = new QWidget(this);
        QVBoxLayout *buttonLayout = new QVBoxLayout(buttonContainer);
        buttonLayout->addWidget(actionButton);
        buttonLayout->setAlignment(Qt::AlignCenter);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        ui->tableView->setIndexWidget(model->index(row, 6), buttonContainer);
    }else{}
}

void DownloadMaxWidget::openFile(QJsonObject obj){
    QString fileStorePath = storePath + "\\" + obj.value("name").toString();
    if (QFile::exists(fileStorePath)) {
        QString command = QString("/select,\"%1\"").arg(QDir::toNativeSeparators(fileStorePath));
        ShellExecute(NULL, L"open", L"explorer.exe", command.toStdWString().c_str(), NULL, SW_SHOWNORMAL);
    } else {
    }
}

QString DownloadMaxWidget::getStatusText(int downStatus){
    QString downStatusText;
    switch (downStatus) {
    case 1:
        downStatusText = "排队中";
        break;
    case 2:
        downStatusText = "下载中";
        break;
    case 3:
        downStatusText = "下载失败";
        break;
    case 4:
        downStatusText = "下载完成";
        break;
    case 5:
        downStatusText = "校验中";
        break;
    case 6:
        downStatusText = "校验失败";
        break;
    case 7:
        downStatusText = "下载成功";
        break;
    default:
        downStatusText = "下载失败";
        break;
    }

    return downStatusText;
}

void DownloadMaxWidget::on_searchButton_clicked()
{
    QString text = ui->searchLineEdit->text();
    for (int i = 0; i < model->rowCount() - pageNumbers; ++i) {
        QString fileName = model->item(i, 0)->text(); // 假设文件名在第一列
        if (text.isEmpty() || fileName.contains(text, Qt::CaseInsensitive)) {
            ui->tableView->setRowHidden(i, false); // 显示匹配的行或全部行
        } else {
            ui->tableView->setRowHidden(i, true); // 隐藏不匹配的行
        }
    }

    // 计算可见行数
    // int visibleRowCount = 0;
    // for (int i = 0; i < model->rowCount(); ++i) {
    //     if (!ui->tableView->isRowHidden(i)) {
    //         visibleRowCount++;
    //     }
    // }

    // // 更新按钮状态
    // ui->prvBtn->setEnabled(currentPage > 0);
    // ui->nextBtn->setEnabled(currentPage < (visibleRowCount / pageNumbers) - (visibleRowCount % pageNumbers == 0 ? 1 : 0));

    // // 计算总页数并更新显示
    // totalPages = (visibleRowCount + pageNumbers - 1) / pageNumbers;
    // ui->pageShowLab->setText(QString("当前%1页/总共%2页").arg(currentPage + 1).arg(totalPages));
}




void DownloadMaxWidget::on_prvBtn_clicked()
{
    if(currentPage > 0){
        --currentPage;
        updateTableData();
    }else{}
}


void DownloadMaxWidget::on_nextBtn_clicked()
{
    if(currentPage < (totalRecords / pageNumbers)){
        ++currentPage;
        updateTableData();
    }else{

    }
}


void DownloadMaxWidget::on_jumpBtn_clicked()
{
    bool ok;
    int pageNumber = ui->inputLinEdit->text().toInt(&ok);

    if (ok && pageNumber > 0 && pageNumber <= totalPages) {
        currentPage = pageNumber - 1;
        updateTableData();
    } else {
    }
}

