#ifndef MSGBOX_H
#define MSGBOX_H

#include <QWidget>
#include <QEventLoop>

#include "common/basewidget.h"



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

    void loginChange();
    void setText1(QString t1);
    void setText2(QString t2);
    void setText3(QString t3);
    void setText4(QString t4);
    void setInfo(QString info);
    void setPasswordChange(QString pass);
    void setBackground(bool set);
    QString info();
    int msgExec();
    int ret();
    void setIcon(MsgType mt);
    void setUrl(QString url);
    void setCloseOnClickAnchor();
    void showCheckBox();

    void setTitle(QString title);
    void setTitleIcon(MsgType mt);
    void setOkText(QString text);
    void setNoText(QString text);
    void setContentWidget(QWidget *wid);
    void setOkClose(bool close);
    void hideOkBtn();
    void hideText3();
    void prependButton(QPushButton *btn);
    void setButtonsCenter();

protected:
    void keyPressEvent(QKeyEvent *e);

signals:
    void closed();
    void accepted();

private slots:
    void on_okBtn_clicked();

    void on_noBtn_clicked();

    void on_headClose_clicked();

    void delayDelete();

private:
    Ui::MsgBox *ui;
    QEventLoop m_loop;
    int m_ret;
    bool m_background;
    QString url;
    bool m_okClose;
    void addShadow(QWidget *w);
};

#endif // MSGBOX_H
