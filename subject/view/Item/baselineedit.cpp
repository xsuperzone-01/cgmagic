#include "baselineedit.h"

#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <QClipboard>
#include "common/eventfilter.h"
#include "common/basewidget.h"
#include "view/login.h"
#include <QComboBox>

BaseLineEdit::BaseLineEdit(QWidget *parent) :
    QLineEdit(parent),
    m_paste(false)
{
    EventFilter *ef = new EventFilter(this);
    ef->inputMethod(this);

    BaseWidget::setMacNoFocusRect(this);

#ifdef Q_OS_MAC
    //mac上点击密码模式框，中文标点不会自动切换英文？
    QTimer::singleShot(100, this, [=]{
        if (echoMode() == QLineEdit::Password) {
            setReplaceChinesePunctuation(true);
        }
    });
#endif

    //结合qss，修改PlaceholderText颜色
    connect(this, &BaseLineEdit::textChanged, this, [=]{
        style()->polish(this);
    });

    this->installEventFilter(this);
    connect(this, &BaseLineEdit::textChanged, this, [=]{
        setFocusOutNoText(false);
    });
}

/*
    A: ]右边的键 B: .右边的键
    windows: 中文: A=、 B=、 英文: A=\ B=/
    mac: 中文: A=、 B=/ 英文: A=\ B=/
    所以在mac下，只需要处理A
*/
QString BaseLineEdit::replaceChinesePunctuation(QString text)
{
    QMap<QString, QString> m;
    m["·"] = "`"; m["！"] = "!"; m["￥"] = "$"; m["……"] = "^"; m["（"] = "("; m["）"] = ")"; m["——"] = "_";
    m["【"] = "["; m["】"] = "]"; m["｜"] = "|"; m["、"] = "\\";
    m["："] = ":"; m["；"] = ";"; m["“"] = "\""; m["”"] = "\""; m["‘"] = "'"; m["’"] = "'";
    m["《"] = "<"; m["，"] = ","; m["》"] = ">"; m["。"] = "."; m["？"] = "?";

    foreach (QString key, m.keys()) {
        text.replace(key, m.value(key));
    }
    return text;
}

void BaseLineEdit::setReplaceChinesePunctuation(bool on)
{
    qDebug()<< __FUNCTION__ << on << this;
    if (on)
        connect(this, &BaseLineEdit::textChanged, this, &BaseLineEdit::setChinesePunc, Qt::UniqueConnection);
    else
        disconnect(this, &BaseLineEdit::textChanged, this, &BaseLineEdit::setChinesePunc);
}

void BaseLineEdit::setRightIcon(QString qssClass, QString user, QSize size, int marginRight)
{

    QLabel *userIcon = this->findChild<QLabel *>("userIcon", Qt::FindDirectChildrenOnly);

    if(userIcon == nullptr){
        QHBoxLayout *lay = layout();
        QMargins m = lay->contentsMargins();
        m.setRight(marginRight);

        lay->setContentsMargins(m);

        QLabel *userIcon = new QLabel(this);
        userIcon->setFixedSize(size);
        BaseWidget::setClass(userIcon, qssClass);
        lay->insertWidget(-20, userIcon);
        BaseWidget::setProperty(userIcon, "userId", user);

        m_left = userIcon;
        m_left->installEventFilter(this);

        userIcon->setObjectName("userIcon");
    }
    else {
        BaseWidget::setProperty(userIcon, "userId", user);
    }

}

void BaseLineEdit::setToplabel(QString qssClass, int id, bool have)
{
    QLabel *userText = this->findChild<QLabel *>("userText", Qt::FindDirectChildrenOnly);
    //有一个问题，不同lineedit中建立的同名qlabel算不算同一个。
    QString text ;
    if(id == 0){
        text = tr("用户名");
    }
    if(id == 1){
        text = tr("手机号");
    }
    if(id == 2){
        text = tr("邮箱");
    }
    if(id == 5){
        text = tr("密码");
    }
    if(have){
        if(userText == nullptr){
            QVBoxLayout *vlay = new QVBoxLayout(this);
            QMargins m = vlay->contentsMargins();
            m.setLeft(20);
            vlay->setContentsMargins(m);

            QLabel *userText = new QLabel(this);
            userText->setText(text);
            BaseWidget::setClass(userText, qssClass);
            userText->show();
            m_top = userText;
            userText->setObjectName("userText");
        }
        else {
            userText->setText(text);
            userText->show();
        }
    }
    if(!have && userText != nullptr){
        userText->hide();
    }
}

void BaseLineEdit::setPassword()
{
    BaseWidget::setProperty(m_top, "focusOutNoText", false);
}

void BaseLineEdit::setPwdButton(QSize size, int marginRight)
{
    QHBoxLayout *lay = layout();
    QMargins m = lay->contentsMargins();
    m.setRight(marginRight);
    lay->setContentsMargins(m);

    QPushButton *pwd = new QPushButton(this);
    pwd->setFixedSize(size);
    pwd->setFocusPolicy(Qt::NoFocus);
    BaseWidget::setClass(pwd, "lineEditPwdBtn");
    lay->addWidget(pwd);

    connect(pwd, &QPushButton::pressed, this, [=]{
        setEchoMode(Normal);
        BaseWidget::setProperty(pwd, "show", true);
    });
    connect(pwd, &QPushButton::released, this, [=]{
        setEchoMode(Password);
        BaseWidget::setProperty(pwd, "show", false);
    });
}

void BaseLineEdit::setRightButton(QPushButton *btn, int marginRight)
{
    QHBoxLayout *lay = layout();
    QMargins m = lay->contentsMargins();
    m.setRight(marginRight);
    lay->setContentsMargins(m);

    btn->setFocusPolicy(Qt::NoFocus);
    lay->addWidget(btn);
}

void BaseLineEdit::setFocusOutNoText(bool noText)
{
    bool p = property("focusOutNoText").toBool();
    if (p != noText) {
        BaseWidget::setProperty(this, "focusOutNoText", noText);
        emit focusOutNoText(noText);
    }
}

void BaseLineEdit::setinit(bool Text)
{
    BaseWidget::setProperty(this, "TextError", Text);
}

void BaseLineEdit::setTextError(bool textError)
{
    bool p = property("TextError").toBool();
    if (p != textError) {
         BaseWidget::setProperty(this, "TextError", textError);
    }
}

void BaseLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        if (event->key() == Qt::Key_V) {
            m_paste = true;
        }
    }
    QLineEdit::keyPressEvent(event);
}

bool BaseLineEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this) {
        switch (event->type()) {
        case QEvent::FocusIn:
            this->setTextError(false);
            BaseWidget::setProperty(this, "focusOutNoText", false);
            BaseWidget::setProperty(m_top, "focusOutNoText", true);
            emit PasswordFocusIn();
            emit focusOutNoText(false);
            emit createTop();
            break;
        case QEvent::FocusOut:
            if (text().isEmpty()) {
                BaseWidget::setProperty(this, "focusOutNoText", true);
                BaseWidget::setProperty(this, "haveNoText", true);
                //在这个地方加个执行m_top（背后的qlabel）的删除。
                emit focusOutNoText(true);
                emit deleteTop();//出了几个问题
                emit PasswordFocusOut();
            } else {
                emit focusOut();
                BaseWidget::setProperty(m_top, "focusOutNoText", false);
            }
            break;
        default:
            break;
        }
    }

    return QLineEdit::eventFilter(watched, event);
}

QHBoxLayout *BaseLineEdit::layout()
{
    QHBoxLayout *lay = qobject_cast<QHBoxLayout *>(QLineEdit::layout());
    if (!lay) {
        lay = new QHBoxLayout(this);
        this->setLayout(lay);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);
        lay->addStretch();
    }
    return lay;
}

void BaseLineEdit::setChinesePunc()
{
    if (m_paste) {
        m_paste = false;
        return;
    }

    QString repl = BaseLineEdit::replaceChinesePunctuation(text());
    setText(repl);
}

DEFINE_RESIZE(BaseLineEdit, QLineEdit);

