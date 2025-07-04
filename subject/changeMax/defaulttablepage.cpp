#include "defaulttablepage.h"
#include "ui_defaulttablepage.h"

#include "choosemaxversion.h"

DefaultTablePage::DefaultTablePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DefaultTablePage)
{
    ui->setupUi(this);

    BaseWidget::setClass(ui->defaultImage, "tableDefault");
    BaseWidget::setClass(ui->defaultText, "tableDefaultText");
}

DefaultTablePage::~DefaultTablePage()
{
    delete ui;
}
