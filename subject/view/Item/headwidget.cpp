#include "headwidget.h"
#include "ui_headwidget.h"

#include "common/basewidget.h"

HeadWidget::HeadWidget(QWidget *parent) :
    Widget(parent),
    ui(new Ui::HeadWidget)
{
    ui->setupUi(this);
    {
        resizeChildrenMargin();
    }
}

HeadWidget::~HeadWidget()
{
    delete ui;
}

void HeadWidget::initHeadWidget(QString title, QWidget *base)
{
    ui->title->setText(title);
    connect(ui->headClose, &QPushButton::clicked, base, &QWidget::close, Qt::UniqueConnection);
}
