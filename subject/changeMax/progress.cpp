#include "progress.h"
#include "ui_progress.h"

#include "common/basewidget.h"

Progress::Progress(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Progress)
{
    ui->setupUi(this);

    setFocusPolicy(Qt::NoFocus);
    ui->widget_2->hide();
}

Progress::~Progress()
{
    delete ui;
}

void Progress::initProgress(int progress, QString status, State state)
{
    setProgress(progress);
    setStatus(status);
    setState(state);
}

void Progress::setProgress(int progress)
{
    ui->pb->setValue(progress);
}

void Progress::setStatus(QString status)
{
    ui->status->setText(status);
}

void Progress::setState(Progress::State state)
{
    QString p;
    switch (state) {
    case Ing:
        p = "ing";
        break;
    case Err:
        p = "err";
        break;
    case Succ:
        p = "succ";
        break;
    default:
        break;
    }
    BaseWidget::setProperty(ui->pb, "status", p);
    BaseWidget::setProperty(ui->point, "status", p);
}

void Progress::setPercentage(int progress, QString status, State state)
{
    if (status == "")
        return;

    QString str = QString::number(progress);
    QString(pro)=QString(" (%1%)").arg(str);
    if(str=="")
    {
        pro=QString(" (0%)");
    }
    if (progress < 0)
        pro = "";

    QString p;
    switch (state) {
    case Ing:
        p = "ing";
        break;
    case Err:
        p = "err";
        pro = "";
        break;
    case Succ:
        p = "succ";
        pro = "";
        break;
    default:
        break;
    }
    ui->status->setText(status + pro);
    BaseWidget::setProperty(ui->pb, "status", p);
    BaseWidget::setProperty(ui->point, "status", p);
}
