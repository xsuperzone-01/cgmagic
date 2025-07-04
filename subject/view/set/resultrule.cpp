#include "resultrule.h"
#include "ui_resultrule.h"

#include <QDebug>
#define qD qDebug()<<

ResultRule::ResultRule(QWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::ResultRule)
{
    ui->setupUi(this);

    show();

    m_bg.setExclusive(false);
    m_bg.addButton(ui->project, project);
    m_bg.addButton(ui->submitTime, submitTime);
    m_bg.addButton(ui->orderNum, orderNum);
    connect(&m_bg, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(checkChanged(QAbstractButton*)));
}

ResultRule::~ResultRule()
{
    delete ui;
}

void ResultRule::initResultRule(QString rule)
{
    if (!rule.isEmpty())
        m_selCheck = rule.split(",");

    qD __FUNCTION__ << rule << m_selCheck;

    foreach (QString id, m_selCheck) {
        if (QAbstractButton* btn = m_bg.button(id.toInt()))
            btn->setChecked(true);
    }
}

void ResultRule::on_submit_clicked()
{
    qD  __FUNCTION__ << m_selCheck.join(",");

    emit resultRule(m_selCheck.join(","));
    this->close();
}

void ResultRule::checkChanged(QAbstractButton *cb)
{
    QString id = QString::number(m_bg.id(cb));
    if (cb->isChecked())
        m_selCheck << id;
    else
        m_selCheck.removeOne(id);
    qD __FUNCTION__ << id << m_selCheck;
}
