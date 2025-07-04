#include "msgtool.h"

int MsgTool::msgLoop(int type, QString info, QWidget* p)
{
    MsgBox* mb = new MsgBox(type, p);
    mb->setInfo(info);
    return mb->msgExec();
}

int MsgTool::msgOkLoop(QString info, QWidget* p)
{
    return msgLoop(1, info, p);
}

int MsgTool::msgChooseLoop(QString info, QWidget* p)
{
    return msgLoop(2, info, p);
}

MsgBox *MsgTool::msgChoose(QString info, QWidget *p)
{
    MsgBox* mb = new MsgBox(2, p);
    mb->setInfo(info);
    return mb;
}

void MsgTool::msgOkSuccess(QString info, QWidget *p)
{
    msgOk(info, MsgBox::Success, p);
}

MsgBox *MsgTool::msgOkWarn(QString info, QWidget *p)
{
    return msgOk(info, MsgBox::Warn, p);
}

MsgBox *MsgTool::msgOkError(QString info, QWidget *p)
{
    return msgOk(info, MsgBox::Error, p);
}

MsgBox *MsgTool::msgOk(QString info, MsgBox::MsgType mt, QWidget* p)
{
    MsgBox* mb = new MsgBox(1, p);
    mb->setInfo(info);
    mb->setTitleIcon(mt);
    return mb;
}

void MsgTool::msgOk(QString info, QWidget *p)
{
    MsgBox* mb = new MsgBox(1, p);
    mb->setInfo(info);
}
