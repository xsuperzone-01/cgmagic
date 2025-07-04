#include "msgbox.h"
#include "ui_msgbox.h"
#include "common/session.h"

#include "config/userinfo.h"
#include "tool/network.h"
#include <QGraphicsDropShadowEffect>
#include "view/set/set.h"

MsgBox::MsgBox(int type, QWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::MsgBox),
    m_ret(msgNo),
    m_background(true),
    m_okClose(true)
{
    ui->setupUi(this);
    {
        selfAdaptionFixedSize();
        selfAdaptionMargins();
        ui->bodyWidget->selfAdaptionMargins();
        ui->bottomWidget->selfAdaptionMargins();
        ui->headWidget->selfAdaptionMargins();
    }

    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    setWindowTitle(qApp->applicationDisplayName());
    modaldel();

    if (1 == type)
        ui->noBtn->hide();

    setClass(ui->headClose, "close");
    setClass(ui->okBtn, "okBtn");
    setClass(ui->noBtn, "noBtn");

    addShadow(ui->okBtn);

    BaseWidget::setClass(ui->titleIcon, "msgBoxI");
    ui->titleIcon->show();

    ui->info->setOpenExternalLinks(true);

    raise();
    activateWindow();
    show();

    ui->widget_3->hide();
    ui->label_3->hide();
    ui->label_4->hide();
}

void MsgBox::loginChange()
{
    ui->info->hide();
    ui->widget_3->show();
    QString lan = Set::changeLan();
    if(lan == "en_us"){
        ui->label_3->show();
        ui->label_4->show();
    }
    connect(ui->passwordChange, &QPushButton::clicked, this, [=](){
        QDesktopServices::openUrl(QUrl(USERINFO->openForgetPwd()));
    });
}

void MsgBox::setText1(QString t1)
{
    if (t1.isEmpty())
        return;
    ui->label->setText(t1);
}

void MsgBox::setText2(QString t2)
{
    if (t2.isEmpty())
        return;
    ui->label_2->setText(t2);
}

void MsgBox::setText3(QString t3)
{
    if (t3.isEmpty())
        return;
    ui->label_3->setText(t3);
}

void MsgBox::setText4(QString t4)
{
    if (t4.isEmpty())
        return;
    ui->label_4->setText(t4);
}

void MsgBox::setPasswordChange(QString pass){
    if (pass.isEmpty())
        return;
    ui->passwordChange->setText(pass);
}

MsgBox::~MsgBox()
{
    m_loop.exit(m_ret);
    delete ui;
}

void MsgBox::addShadow(QWidget *w)
{
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 7);//水平位移，垂直位移
    QPalette pal = w->palette();
    QColor color = pal.color(QPalette::Window);
    effect->setColor(QColor(19, 125, 211, 51));
    effect->setBlurRadius(15);//阴影扩散半径
    w->setGraphicsEffect(effect);
}

void MsgBox::setInfo(QString info)
{
    if (info.isEmpty())
        return;

    ui->info->append(info);
    ui->info->moveCursor(QTextCursor::Start);
}

void MsgBox::setBackground(bool set)
{
    m_background = set;
}

QString MsgBox::info()
{
    return ui->info->toPlainText();
}

int MsgBox::msgExec()
{
    QPointer<MsgBox> ptr = this;
    int ret = m_loop.exec();
    //有可能MsgBox已经delete之后事件循环才到这里
    if (!ptr) {
        qDebug()<< "msgbox is null, return" << ret;
        return ret;
    }
    return m_ret;
}

int MsgBox::ret()
{
    return m_ret;
}

void MsgBox::setIcon(MsgBox::MsgType mt)
{
}

void MsgBox::setUrl(QString url)
{
    this->url = url;

}

/*
    关闭自动跳转，否则无法发送anchor信号
*/
void MsgBox::setCloseOnClickAnchor()
{
    ui->info->setOpenExternalLinks(false);
    connect(ui->info, &QTextBrowser::anchorClicked, this, [=](QUrl url) {
        QDesktopServices::openUrl(url);
        on_headClose_clicked();
    });
}

void MsgBox::showCheckBox()
{
}

void MsgBox::setTitle(QString title)
{
    ui->title->setText(title);
}

void MsgBox::setTitleIcon(MsgBox::MsgType mt)
{
    QString c;
    switch (mt) {
    case MsgType::Warn:
        c = "msgBoxI";
        break;
    case MsgType::Error:
        c = "msgBoxError";
        break;
    default:
        break;
    }
//修改提示窗icon样式
    BaseWidget::setClass(ui->titleIcon, c);
    ui->titleIcon->show();
}

void MsgBox::setOkText(QString text)
{
    ui->okBtn->setText(text);
    ui->okBtn->show();
}

void MsgBox::setNoText(QString text)
{
    ui->noBtn->setText(text);
    ui->noBtn->show();
}

void MsgBox::setContentWidget(QWidget *wid)
{
    ui->info->deleteLater();
    ui->bodyWidget->layout()->addWidget(wid);

    if (m_background)
        setBackgroundMask(this->parentWidget());
}

void MsgBox::setOkClose(bool close)
{
    m_okClose = close;
}

void MsgBox::hideOkBtn()
{
    ui->okBtn->hide();
}

void MsgBox::hideText3()
{
    ui->label_3->hide();
}


void MsgBox::prependButton(QPushButton *btn)
{
    if (QHBoxLayout *lay = qobject_cast<QHBoxLayout *>(ui->bottomWidget->layout())) {
        lay->insertWidget(2, btn);
        connect(btn, &QPushButton::clicked, this, &MsgBox::delayDelete);
    }
}

void MsgBox::setButtonsCenter()
{
    if (QHBoxLayout *lay = qobject_cast<QHBoxLayout *>(ui->bottomWidget->layout())) {
        lay->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    }
}

void MsgBox::on_okBtn_clicked()
{
    m_ret = msgOk;
    emit accepted();
    if (!this->url.isEmpty())
    {
        QDesktopServices::openUrl(url);
    }

    delayDelete();
}

void MsgBox::on_noBtn_clicked()
{
    m_ret = msgNo;
    delayDelete();
}
void MsgBox::on_headClose_clicked()
{
    m_ret = msgNo;
    delayDelete();
}

void MsgBox::delayDelete()
{
    m_loop.exit(m_ret);
    if (!m_okClose && msgOk == m_ret)
        return;
    emit closed();
    QTimer::singleShot(20, this, SLOT(deleteLater()));
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
