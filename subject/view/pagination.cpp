#include "pagination.h"
#include "ui_pagination.h"

#include "common/basewidget.h"

Pagination::Pagination(QWidget *parent) :
    Widget(parent),
    ui(new Ui::Pagination)
{
    ui->setupUi(this);

    ui->page->setMinimum(1);
    restore();
    connect(&m_debounce, &Debounce::debout, [=]{
        m_page = page();
        emit pageChanged(m_page);
    });
    connect(ui->page, SIGNAL(valueChanged(int)), this, SLOT(pageChange(int)));

    BaseWidget::setClass(ui->first, "pageFirst");
    BaseWidget::setClass(ui->last, "pageLast");
    BaseWidget::setClass(ui->pre, "pagePrev");
    BaseWidget::setClass(ui->next, "pageNext");
}

Pagination::~Pagination()
{
    delete ui;
}

void Pagination::restore(bool needChange)
{
    if (!needChange)
        disconnect(ui->page, 0, 0, 0);
    setMaxPage(100000);
    setPage(0);
    if (!needChange)
        connect(ui->page, SIGNAL(valueChanged(int)), this, SLOT(pageChange(int)));
}

void Pagination::setMaxPage(int max)
{
    max = qMax(1, max);
    ui->page->setMaximum(max);
    if (100000 != max) {
        ui->sum->setMaximum(max);
        ui->sum->setValue(max);
    }
    setBtnEnable();
}

int Pagination::page()
{
    return ui->page->value() - 1;
}

void Pagination::setPage(int page)
{
    m_page = page;
    m_page = qMax(0, m_page);
    m_page = qMin(m_page, ui->page->maximum());
    ui->page->setValue(m_page + 1);

    setBtnEnable();

    this->setFocus();
}

void Pagination::on_pre_clicked()
{
    m_page = page() - 1;
    setPage(m_page);
}

void Pagination::on_next_clicked()
{
    m_page = page() + 1;
    setPage(m_page);
}

void Pagination::on_first_clicked()
{
    m_page = ui->page->minimum() - 1;
    setPage(m_page);
}

void Pagination::on_last_clicked()
{
    m_page = ui->page->maximum() - 1;
    setPage(m_page);
}

void Pagination::pageChange(int arg1)
{
    m_debounce.startDeb(300);
}

void Pagination::setBtnEnable()
{
    ui->first->setEnabled(true);
    ui->pre->setEnabled(true);
    ui->next->setEnabled(true);
    ui->last->setEnabled(true);

    int page = ui->page->value();
    if (ui->page->minimum() == page) {
        ui->first->setEnabled(false);
        ui->pre->setEnabled(false);
    }
    if (ui->page->maximum() == page) {
        ui->next->setEnabled(false);
        ui->last->setEnabled(false);
    }
}
