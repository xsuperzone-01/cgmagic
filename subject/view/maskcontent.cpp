#include "maskcontent.h"
#include "ui_maskcontent.h"

MaskContent::MaskContent(QWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::MaskContent)
{
    ui->setupUi(this);

    paintDropShadow = false;
    canMove = false;
    BaseWidget::setClass(ui->widget, "maskContent");
}

MaskContent::~MaskContent()
{
    delete ui;
}

void MaskContent::setContentWidget(QWidget *wid)
{
    content = wid;

    wid->setParent(this);
    connect(wid, &QWidget::destroyed, this, &MaskContent::deleteLater);

    ui->widget->layout()->addWidget(wid);
    QSize size = wid->size();
    size.setWidth(size.width()+2);
    size.setHeight(size.height()+2);
    setOriginalFixedSize(size);
}
