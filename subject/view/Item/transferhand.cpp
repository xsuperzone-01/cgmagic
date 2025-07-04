#include "transferhand.h"
#include "ui_transferhand.h"

#include "transfer/transset.h"
#include "tool/xfunc.h"
#include "common/session.h"
#include "transfer/sessiontimer.h"

TransferHand::TransferHand(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TransferHand)
{
    ui->setupUi(this);

    setFocusPolicy(Qt::NoFocus);

    foreach (QPushButton *btn, findChildren<QPushButton *>()) {
        btn->setFocusPolicy(Qt::NoFocus);
    }

    ui->del->hide();
    ui->op->hide();
    ui->del->setEnabled(false);
}

TransferHand::~TransferHand()
{
    delete ui;
}

void TransferHand::showOp()
{
    ui->op->show();
}

void TransferHand::showDel()
{
    ui->del->show();
    BaseWidget::setProperty(ui->del, "type", 0);
}

void TransferHand::changeDel(int a)
{
    int b = 0;
    if (a == 13 || a == 30) {
        b = 1;
    } else {
        b = 0;
    }
    BaseWidget::setProperty(ui->del, "type", b);
    if (0 == b) {
        ui->del->setEnabled(false);
    } else {
        ui->del->setEnabled(true);
    }
}

void TransferHand::bindTableMenu(QPointer<QTableWidget> table, QPointer<QMenu> menu)
{
    connect(this, &TransferHand::op, this, [=]{
        QPoint pos = mapToGlobal(QPoint(0, 0));
        pos.setY(pos.y() + height());
        if (table) {
            //只选当前行
            if (QTableWidgetItem *item = table->itemAt(table->viewport()->mapFromGlobal(QCursor::pos()))) {
                table->clearSelection();
                table->selectRow(item->row());
            }
            emit table->customContextMenuRequested(pos);
        }
        if (menu)
            BaseWidget::adjustMove(menu, QCursor::pos().x(), QCursor::pos().y());
    });
}

void TransferHand::setData(int t, QVariant var)
{
    m_dataM.insert(t, var);
}

void TransferHand::on_del_clicked()
{
    emit op();
}

void TransferHand::on_op_clicked()
{
    emit op();
}
