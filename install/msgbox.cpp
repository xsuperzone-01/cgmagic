#include "msgbox.h"
#include "ui_msgbox.h"

MsgBox::MsgBox(int type, QWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::MsgBox),
    m_ret(0)
{
    ui->setupUi(this);

    if (1 == type)
        ui->noBtn->hide();

    raise();
    activateWindow();

    setClass(ui->okBtn, "okBtn");
    setClass(ui->noBtn, "noBtn");
    setClass(ui->headClose, "close");

    show();
    setDropShadow(this, 0, 0, 40, QColor(0, 0, 0, 70));
}

void MsgBox::addShadow(QWidget *w)
{
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 7);//水平位移，垂直位移
    QPalette pal = w->palette();
    QColor color = pal.color(QPalette::Window);
    effect->setColor(QColor(19, 125, 211, 102));
    effect->setBlurRadius(15);//阴影扩散半径
    w->setGraphicsEffect(effect);
}

MsgBox::~MsgBox()
{
    m_loop.exit();
    delete ui;
}

void MsgBox::setInfo(QString info)
{
    ui->info->append(info);
    ui->info->moveCursor(QTextCursor::Start);

    QTextCursor cursor = ui->info->textCursor();
    QTextBlockFormat textBlockFormat;
    textBlockFormat.setLineHeight(20,QTextBlockFormat::FixedHeight);
    cursor.setBlockFormat(textBlockFormat);
    ui->info->setTextCursor(cursor);
}

QString MsgBox::info()
{
    return ui->info->toPlainText();
}

void MsgBox::msgExec()
{
    m_loop.exec();
}

int MsgBox::ret()
{
    return m_ret;
}

void MsgBox::setIcon(MsgBox::MsgType mt)
{
}

void MsgBox::on_okBtn_clicked()
{
    m_ret = msgOk;
    m_loop.exit();
    this->hide();
    this->deleteLater();
}

void MsgBox::on_noBtn_clicked()
{
    m_ret = msgNo;
    m_loop.exit();
    this->deleteLater();
}
void MsgBox::on_headClose_clicked()
{
    m_ret = msgNo;
    m_loop.exit();
    this->deleteLater();
}

void MsgBox::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Escape:
        on_noBtn_clicked();
        break;
    }

    BaseWidget::keyPressEvent(e);
}

void MsgBox::closeEvent(QCloseEvent *event){
    m_ret = msgNo;
    m_loop.exit();
    this->deleteLater();
}

int MsgTool::msgOkLoop(QString info, QWidget *p)
{
    return msgLoop(1, info, p);
}

int MsgTool::msgChooseLoop(QString info, QWidget *p)
{
    return msgLoop(2, info, p);
}

int MsgTool::msgLoop(int type, QString info, QWidget *p)
{
    MsgBox* mb = new MsgBox(type, p);
    mb->setInfo(info);
    mb->msgExec();
    return mb->ret();
}
