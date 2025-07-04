#include "choosemaxversion.h"
#include "ui_choosemaxversion.h"

#include <QDebug>
#include <QTimer>
#include <atlbase.h>
#include <QListView>

ChooseMaxVersion::ChooseMaxVersion(QWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::ChooseMaxVersion)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_ShowModal, true);
    setAttribute(Qt::WA_DeleteOnClose, true);

    setClass(ui->headClose, "close");
    setClass(ui->okBtn, "okBtn");
    setClass(ui->noBtn, "noBtn");

    connect(ui->note, &QPushButton::clicked, this, [=](){
        if(ui->widget_4->isVisible()){
            ui->widget_4->hide();
            BaseWidget::setProperty(ui->note, "type", "down");
        }else {
            ui->widget_4->show();
            BaseWidget::setProperty(ui->note, "type", "up");
        }
    });

    ui->toVersion->setEditable(false);
    ui->toVersion->setMaxVisibleItems(5);
    ui->toVersion->addItem("请选择需要转换的版本", QVariant(0));//将提示置于最顶部，后面加不可选择代码
    qobject_cast<QListView *>(ui->toVersion->view())->setRowHidden(0, true);

//    针对这个控件在样式表里写属性，用type属性区分默认和修改
    connect(ui->toVersion, &QComboBox::currentTextChanged, this, [=](){
        if (ui->toVersion->currentText() != "请选择需要转换的版本") {
            setProperty(ui->toVersion, "type", "1");
            ui->okBtn->setEnabled(true);
    }
    });
    setProperty(ui->toVersion, "type", "0");
    show();
}

ChooseMaxVersion::~ChooseMaxVersion()
{
    delete ui;
}

void ChooseMaxVersion::init(QStringList files)
{
    qDebug()<< __FUNCTION__ << files;

    m_files = files;
    ui->version->setText("解析中...");
    ui->version2->setText("解析中...");
    ui->okBtn->setEnabled(false);

    QTimer::singleShot(10, this, SLOT(initSlt()));

    QTimer::singleShot(1, this, [=]{
        ui->label_3->setFixedWidth(ui->label_2->width());
    });
}

void ChooseMaxVersion::on_headClose_clicked()
{
    this->close();
    QTimer::singleShot(5000, this, SLOT(deleteLater()));
}

void ChooseMaxVersion::on_noBtn_clicked()
{
    on_headClose_clicked();
}

void ChooseMaxVersion::on_okBtn_clicked()
{
    emit postJobSig(m_files, ui->version->text().mid(ui->version->text().length()-4),
                    QString::number(ui->toVersion->currentData().toInt()));

    on_headClose_clicked();
}

void ChooseMaxVersion::initSlt()
{
    int version = 0;
    QStringList versionPatternL;
    versionPatternL << "Saved As Version: " << "另存为版本: "
                    << "3ds Max Version: " << "3ds Max 版本: ";

    //结构化读取
    {
        for (int i = 0; i < m_files.length(); i++) {
            QString file = m_files.at(i);

            int ver = 0;
            CComPtr<IStorage> pStorage;
            HRESULT hr = StgOpenStorageEx(file.toStdWString().c_str(),
                                  STGM_DIRECT | STGM_READ | STGM_PRIORITY,
                                  STGFMT_STORAGE, 0, 0, 0, IID_IStorage, (void**)&pStorage);
            if (SUCCEEDED(hr)) {
                CComPtr<IStream> pStream;
                hr = pStorage->OpenStream(L"\005DocumentSummaryInformation", NULL, STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                                          0, &pStream);
                if (SUCCEEDED(hr)) {
                    STATSTG stat;
                    pStream->Stat(&stat, STATFLAG_DEFAULT);
                    ULONG len = stat.cbSize.QuadPart;
                    qDebug()<<"pStream len:" << len/* << QString::fromWCharArray(stat.pwcsName)*/;

                    QByteArray ba;
                    ba.resize(len);
                    pStream->Read(ba.data(), ba.size(), NULL);

                    foreach (QString patt, versionPatternL) {
                        int idx3 = ba.indexOf(patt.toUtf8());
                        if (idx3 != -1) {
                            // 使用 QRegularExpression 替代 QRegExp
                            QRegularExpression reg(patt + "(\\d+)", QRegularExpression::CaseInsensitiveOption);
                            QRegularExpressionMatch match = reg.match(ba.mid(idx3));

                            if(match.hasMatch()){
                                qDebug()<<match.captured(0); // 获取完整的匹配
                                ver = match.captured(1).toInt(); // 获取第一个捕获组并转换为整数
                                break;
                            }
                        }
                    }

                    qDebug()<< __FUNCTION__ << ver << file;
                    version = qMax(version, ver);
                    pStream.Release();
                } else {
                    qDebug()<< "OpenStream" << hr;
                }
                pStorage.Release();
            } else {
                qDebug()<< "StgOpenStorageEx" << hr;
            }
        }
    }

    if (version == 0) {
        emit error(tr("当前Max文件解析异常，文件可能损坏或加密"));
    } else {
        setBackgroundMask();
        version = version <= 9 ? version : 2008 + version - 10;
        qDebug()<< __FUNCTION__ << "max version:" << version;

        for (int i = version - 1 ; i >2009 ; i--) {
            ui->toVersion->addItem(QString("3ds Max %1").arg(i), QVariant(i));
        }

        ui->version->setText(QString("3ds Max %1").arg(version));
        ui->version2->setText(QString("3ds Max %1").arg(version));
        if (2010 == version) {
            ui->toVersion->addItem(tr("不支持降版本"));
            ui->toVersion->setEnabled(false);
        }
    }
}
