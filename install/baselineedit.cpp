#include "baselineedit.h"

#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <QClipboard>
#include <QStyle>
#include "basewidget.h"

BaseLineEdit::BaseLineEdit(QWidget *parent) :
    QLineEdit(parent),
    m_paste(false)
{
//    EventFilter *ef = new EventFilter(this);
//    ef->inputMethod(this);

//    BaseWidget::setMacNoFocusRect(this);

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

void BaseLineEdit::setRightButton(QPushButton *btn, int marginRight)
{
    QHBoxLayout *lay = layout();
    QMargins m = lay->contentsMargins();
    m.setRight(marginRight);
    lay->setContentsMargins(m);

    btn->setFocusPolicy(Qt::NoFocus);
    lay->addWidget(btn);
}

void BaseLineEdit::setPlaceholderTextColor(QColor color)
{
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
    switch (event->type()) {
    case QEvent::HoverEnter:
    case QEvent::FocusIn:
        if (m_left) {
            BaseWidget::setProperty(m_left, "hover", true);
        }
        break;
    case QEvent::HoverLeave:
    case QEvent::FocusOut:
        if (m_left && !hasFocus())
            BaseWidget::setProperty(m_left, "hover", false);
        break;
    default:
        break;
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
