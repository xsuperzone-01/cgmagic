#ifndef MSGTOOL_H
#define MSGTOOL_H
#include <QObject>
#include <view/msgbox.h>

class MsgTool : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE static int msgOkLoop(QString info, QWidget *p = 0);
    Q_INVOKABLE static int msgChooseLoop(QString info, QWidget *p = 0);
    static MsgBox *msgChoose(QString info, QWidget *p = 0);

    Q_INVOKABLE static void msgOkSuccess(QString info, QWidget *p = 0);
    Q_INVOKABLE static MsgBox *msgOkWarn(QString info, QWidget *p = 0);
    Q_INVOKABLE static MsgBox *msgOkError(QString info, QWidget *p = 0);
    Q_INVOKABLE static MsgBox *msgOk(QString info, MsgBox::MsgType mt, QWidget *p = 0);
    Q_INVOKABLE static void msgOk(QString info, QWidget *p = 0);

private:
    Q_INVOKABLE static int msgLoop(int type, QString info, QWidget *p = 0);
};
#endif // MSGTOOL_H
