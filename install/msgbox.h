#pragma execution_character_set("utf-8")
#ifndef MSGBOX_H
#define MSGBOX_H

#include <QWidget>
#include <QEventLoop>
#include <QGraphicsDropShadowEffect>

#include "basewidget.h"
#include <QCloseEvent>

class MsgTool : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE static int msgOkLoop(QString info, QWidget *p = 0);
    Q_INVOKABLE static int msgChooseLoop(QString info, QWidget *p = 0);

//    Q_INVOKABLE static void msgOkSuccess(QString info, QWidget *p = 0);
//    Q_INVOKABLE static void msgOkWarn(QString info, QWidget *p = 0);
//    Q_INVOKABLE static void msgOkError(QString info, QWidget *p = 0);
//    Q_INVOKABLE static void msgOk(QString info, MsgBox::MsgType mt, QWidget *p = 0);

//    Q_INVOKABLE static QString msgInput(bool* ok, QString title, QString defMsg, QWidget *p = 0);
private:
    Q_INVOKABLE static int msgLoop(int type, QString info, QWidget *p = 0);
};

namespace Ui {
class MsgBox;
}

class MsgBox : public BaseWidget
{
    Q_OBJECT

public:
    enum MsgType {
        msgOk = 0,
        msgNo,
        Success,
        Warn,
        Error
    };

    explicit MsgBox(int type, QWidget *parent = 0);
    ~MsgBox();

    void setInfo(QString info);
    QString info();
    void msgExec();
    int ret();
    void setIcon(MsgType mt);

protected:
    void keyPressEvent(QKeyEvent *e);
    void closeEvent(QCloseEvent *event);

private slots:
    void on_okBtn_clicked();

    void on_noBtn_clicked();

    void on_headClose_clicked();

    void addShadow(QWidget *w);

private:
    Ui::MsgBox *ui;
    QEventLoop m_loop;
    int m_ret;
};

#endif // MSGBOX_H
