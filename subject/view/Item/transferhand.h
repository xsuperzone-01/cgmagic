#ifndef TRANSFERHAND_H
#define TRANSFERHAND_H

#include <QWidget>
#include <QMap>
#include <QPushButton>
#include <QTableWidget>

namespace Ui {
class TransferHand;
}

class TransferHand : public QWidget
{
    Q_OBJECT

public:
    explicit TransferHand(QWidget *parent = 0);
    ~TransferHand();

    void showOp();
    void showDel();

    void changeDel(int a);

    void bindTableMenu(QPointer<QTableWidget> table, QPointer<QMenu> menu);

    void setData(int t, QVariant var);

signals:
    void downloadJobSig(int jobId);
    void openDirSig(int jobId);
    void op();

private slots:
    void on_del_clicked();

    void on_op_clicked();

private:
    Ui::TransferHand *ui;

    QMap<int, QVariant> m_dataM;
};

#endif // TRANSFERHAND_H
