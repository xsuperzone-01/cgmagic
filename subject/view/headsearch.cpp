#include "headsearch.h"
#include "ui_headsearch.h"

HeadSearch::HeadSearch(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HeadSearch)
{
    ui->setupUi(this);

    ui->name->setPlaceholderText(tr("输入任务名"));
    this->setFixedWidth(this->sizeHint().width());
    ui->name->installEventFilter(this);
    ui->name->setFocusPolicy(Qt::StrongFocus);

    connect(ui->name, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

    m_timer.setInterval(500);
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(searchOut()));
}

HeadSearch::~HeadSearch()
{
    delete ui;
}

void HeadSearch::hideMenu()
{
    ui->menu->hide();
}

void HeadSearch::hideRefresh()
{
    ui->refresh->hide();
}

void HeadSearch::emptyText(QString text)
{
    if (text.isEmpty()) {
        hideSearch(true);
    }
    emit searchName(0, text);
}

void HeadSearch::hideSearch(bool hide)
{
    ui->widget->setHidden(hide);
    ui->search->setHidden(!hide);
    this->setFixedWidth(this->sizeHint().width());
    emit searchMove();
}

void HeadSearch::hideSearch()
{
    ui->widget->hide();
    ui->search->hide();
    this->setFixedWidth(this->minimumSizeHint().width());
    emit searchMove();
}

bool HeadSearch::eventFilter(QObject *o, QEvent *e)
{
    if (o == ui->name) {
        if (e->type() == QEvent::FocusOut) {
            if (ui->name->text().isEmpty()) {
                hideSearch(true);
            }
        }
    }
    return QWidget::eventFilter(o, e);
}

void HeadSearch::on_search_clicked()
{
    hideSearch(false);
    ui->name->setFocus();
}

void HeadSearch::on_name_returnPressed()
{
    emit searchName(0, ui->name->text());
}

void HeadSearch::on_refresh_clicked()
{
    emit searchName(1, ui->name->text());
}

void HeadSearch::on_menu_clicked()
{
    emit searchName(2, "");
}

void HeadSearch::textChanged(QString text)
{
    m_timer.start();
}

void HeadSearch::searchOut()
{
    emptyText(ui->name->text());
}
