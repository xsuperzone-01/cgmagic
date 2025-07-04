#include "eventfilter.h"

#include "basewidget.h"

#include <QApplication>
#include <QInputMethodEvent>
#include <QDebug>
#include <QTimer>

EventFilter::EventFilter(QObject *parent) : QObject(parent)
{

}

void EventFilter::bindEvent(QWidget *wid1, QWidget *wid2, QPushButton *btn1, QPushButton *btn2)
{
    w1 = wid1;
    w2 = wid2;
    b1 = btn1;
    b2 = btn2;

    w1->installEventFilter(this);
    w1->setCursor(Qt::PointingHandCursor);
    w2->installEventFilter(this);
    w2->setCursor(Qt::PointingHandCursor);
}

void EventFilter::inputMethod(QWidget *w)
{
    inputMethodWid = w;
    inputMethodWid->installEventFilter(this);
}

bool EventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (inputMethodWid) {
        if (event->type() == QEvent::InputMethod) {
            QInputMethodEvent *e = dynamic_cast<QInputMethodEvent *>(event);
            if (QEvent::FocusAboutToChange == prevEventType) {
                prevEventType = QEvent::None;
                e->setCommitString("");
            }
        } else {
            prevEventType = event->type();
        }
    } else {
        QPointer<QPushButton> btn;
        if (obj == w1) {
            btn = b1;
        } else if (obj == w2) {
            btn = b2;
        }
        if (btn) {
            if (event->type() == QEvent::Enter) {
                btn->setProperty("tab", "1");
                btn->setStyleSheet(qApp->styleSheet());
            } else if (event->type() == QEvent::Leave) {
                btn->setProperty("tab", "0");
                btn->setStyleSheet(qApp->styleSheet());
            } else if (event->type() == QEvent::MouseButtonRelease) {
                btn->click();
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

/*
    居中于父窗口
*/
EFAlignCenter::EFAlignCenter(QObject *parent) :
    QObject(parent)
{
    if (QWidget *wid = static_cast<QWidget *>(this->parent())) {
        moveCenter(wid, NULL);
        wid->parentWidget()->installEventFilter(this);
    }
}

void EFAlignCenter::moveCenter(QWidget *wid, QWidget *parent)
{
    if (!parent) {
        parent = qobject_cast<QWidget *>(wid->parent());
    }
    if (parent) {
        connect(parent, SIGNAL(destroyed()), wid, SLOT(deleteLater()), Qt::UniqueConnection);
        QPoint gp = parent->mapToGlobal(QPoint(0,0));
        wid->move((parent->width() - wid->width())/2, (parent->height() - wid->height())/2);
    }
}

bool EFAlignCenter::eventFilter(QObject *watched, QEvent *event)
{
    if (QWidget *wid = static_cast<QWidget *>(parent())) {
        if (watched == wid->parentWidget()) {
            if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
                moveCenter(wid, NULL);
            }
        }
    }

    return QObject::eventFilter(watched, event);
}


/*
    父子窗口同尺寸
*/
EFResizeToParent::EFResizeToParent(QObject *parent) :
    QObject(parent)
{
    if (QWidget *wid = static_cast<QWidget *>(this->parent())) {
        wid->parentWidget()->installEventFilter(this);
    }
}

bool EFResizeToParent::eventFilter(QObject *watched, QEvent *event)
{
    if (BaseWidget *wid = static_cast<BaseWidget *>(parent())) {
        if (watched == wid->parentWidget()) {
            if (event->type() == QEvent::Resize) {
                wid->setOriginalFixedSize(wid->parentWidget()->size());
            }
        }
    }

    return QObject::eventFilter(watched, event);
}

/*
    QTableWidget 滑动时按行变色
*/
EFTableRowHover::EFTableRowHover(QTableWidget *table, QColor rowColor, QObject *parent) :
    QObject(parent)
{
    m_table = table;
    m_rowColor = rowColor;

    m_table->viewport()->installEventFilter(this);
}

bool EFTableRowHover::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_table->viewport()) {
        int curRow = -2;
        if (event->type() == QEvent::MouseMove) {
            curRow = -1;
            if (QMouseEvent *me = static_cast<QMouseEvent *>(event)) {
                QTableWidgetItem *item = m_table->itemAt(me->pos());
                if (item) {
                    curRow = item->row();
                }
            }
        } else if (event->type() == QEvent::Leave) {
            curRow = -1;
        }

        if (curRow > -2) {
            //行数会变，若记住上一次row无意义，只能实时计算
            QAbstractItemModel *model = m_table->model();
            for (int row = 0; row < model->rowCount(); row++) {
                if (!model->data(model->index(row, 0), Qt::BackgroundRole).isNull()) {
                    for (int col = 0; col < model->columnCount(); col++) {
                        model->setData(model->index(row, col), QVariant(), Qt::BackgroundRole);
                    }
                    break;
                }
            }
            if (curRow > -1) {
                for (int col = 0; col < model->columnCount(); col++) {
                    model->setData(model->index(curRow, col), m_rowColor, Qt::BackgroundRole);
                }
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

//
EFPaintDropShadow::EFPaintDropShadow(int x, int y, int radius, QColor color, QObject *parent) :
    QObject(parent)
{
    m_x = x;
    m_y = y;
    m_radius = radius;
    m_color = color;
    m_wid = qobject_cast<QWidget *>(parent);
    m_wid->installEventFilter(this);
}

bool EFPaintDropShadow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Paint) {
        QColor color = m_color;
        qreal a = color.alphaF();
        qreal d = a/m_radius;
        qDebug()<< "asdasd" << a << d;

        QPainter painter(m_wid);
        painter.setRenderHint(QPainter::Antialiasing, true);
        for (int i = 0; i < m_radius; i++) {
            qDebug()<< a - i*d;
            color.setAlphaF(a - i*3*d);
            painter.setPen(color);
            painter.drawRoundedRect(m_radius-i, m_radius-i, m_wid->width()-(m_radius-i)*2, m_wid->height()-(m_radius-i)*2, 2, 2);
        }
    }

    return QObject::eventFilter(watched, event);
}

#include <QGraphicsDropShadowEffect>
/*
    减少QGraphicsDropShadowEffect的副作用
    如父辈窗口有shadow导致子窗口QOpenglWidget无法刷新
*/
EFDropShadow::EFDropShadow(QWidget *source, int x, int y, int radius, QColor color, QWidget *parent) :
    QObject(parent)
{
    m_source = source;
    m_target = new QWidget(parent);
    m_target->setObjectName("EFDropShadow");
    m_target->setStyleSheet("QWidget {background: #ffffff;}");

    GraphicsDropShadowEffect *ds = new GraphicsDropShadowEffect(m_target);
    ds->setOffset(x, y);
    ds->setColor(color);
    ds->setBlurRadius(radius);
    m_target->setGraphicsEffect(ds);
    connect(ds, &GraphicsDropShadowEffect::stopRequest, m_target, [=]{
        m_target->setUpdatesEnabled(false);
    });

    m_target->stackUnder(m_source);
    m_target->move(m_source->pos());
    m_source->installEventFilter(this);

    m_target->installEventFilter(this);
    qApp->installEventFilter(this);
}

void EFDropShadow::startTargetEffect(int pc)
{
    if (GraphicsDropShadowEffect *ef = qobject_cast<GraphicsDropShadowEffect *>(m_target->graphicsEffect())) {
        ef->setPaintCount(pc);
    }
    m_target->setUpdatesEnabled(true);
}

bool EFDropShadow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_source) {
        if (event->type() == QEvent::Resize) {
            m_target->move(m_source->pos());
            m_target->resize(m_source->size());
        }
    } else if (watched == m_target) {
//        qDebug()<< event->type();
        QEvent::Type t = event->type();
        if (t == QEvent::WindowActivate || t == QEvent::WindowDeactivate ||
                t == QEvent::Resize || t == QEvent::ScreenChangeInternal) {
            startTargetEffect(t == QEvent::Resize ? 50 : 10);
        }
    } else if (watched == qApp) {
        if (event->type() == QEvent::ApplicationActivate) {
            startTargetEffect(10);
        }
    }

    return QObject::eventFilter(watched, event);
}

/*
    MainWidget和NODE用了此阴影，会导致StatusView的40ms刷新占用较高CPU
    原因是，底层重复刷新阴影，约10ms就调一次draw
*/
GraphicsDropShadowEffect::GraphicsDropShadowEffect(QObject *parent) :
    QGraphicsDropShadowEffect(parent),
    m_pc(0),
    m_pcMax(10)
{

}

void GraphicsDropShadowEffect::setPaintCount(int c)
{
    m_pc = 0;
    m_pcMax = c;
}

void GraphicsDropShadowEffect::draw(QPainter *painter)
{
//    qDebug()<< __FUNCTION__ << m_pc;

    if (m_pc > m_pcMax) {
        emit stopRequest();
        m_pc = 0;
        return;
    }

    //painter会为0？
    if (painter)
        QGraphicsDropShadowEffect::draw(painter);
    else
        qDebug()<< __FUNCTION__ << "painter is nullptr";

    m_pc++;
}

/*
    监听mac下的url scheme启动参数
*/
#include <QFileOpenEvent>
EFFileOpen::EFFileOpen(QApplication *app, QEventLoop *loop)
{
    qDebug()<< __FUNCTION__;
    m_loop = loop;
    app->installEventFilter(this);
}

QString EFFileOpen::scheme()
{
    return m_scheme;
}

bool EFFileOpen::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        if (QFileOpenEvent *open = static_cast<QFileOpenEvent *>(event)) {
            m_scheme = open->url().toString();
            qDebug()<< event->type() << m_scheme;
        }
        m_loop->quit();
        watched->removeEventFilter(this);
    }

    return QObject::eventFilter(watched, event);
}

bool EventLoop::m_appQuit = false;
EventLoop::EventLoop(int msec, QObject *parent) :
    QEventLoop(parent)
{
    connect(qApp, &QApplication::aboutToQuit, this, &EventLoop::appQuit);
    if (msec > 0)
        QTimer::singleShot(msec, this, &EventLoop::quit);
}

bool EventLoop::execUntilAppQuit()
{
    if (m_appQuit)
        return true;
    exec();
    return m_appQuit;
}

void EventLoop::appQuit()
{
    m_appQuit = true;
    quit();
}


ScrollBarEventFilter::ScrollBarEventFilter(QObject *parent) :
    QObject(parent)
{

}

/*
    滚动区域垂直滚动条出现时右边距rightShow
*/
void ScrollBarEventFilter::bindEvent(QScrollBar *bar, QLayout *lay, int rightHide, int rightShow)
{
    this->sb = bar;
    this->lay = lay;
    this->rh = rightHide;
    if (0 == rightShow) {
        rightShow = rightHide - bar->width();
    }
    this->rs = rightShow;
    bar->installEventFilter(this);
}

void ScrollBarEventFilter::setMainWindow(QWidget *mainwindow)
{
    this->main = mainwindow;
}

/*
    当水平滚动条也会出现的某个临界点，setContentsMargins会导致重复进入Hide和Show，卡界面
    加入m_eventL，阻止重复
*/
bool ScrollBarEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (m_eventL.contains(event->type())) {
        m_eventL.removeAll(event->type());
        return QObject::eventFilter(obj, event);
    }

    if (obj == sb) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        {
            //当处于主窗口边缘时，鼠标会变成拖动形状，此时不能让滚动条抓取鼠标
            QWidget *main = qApp->activeWindow();
            if (main && main->cursor().shape() != Qt::ArrowCursor) {
                QMouseEvent *me = new QMouseEvent(event->type(), QCursor::pos(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                QApplication::postEvent(main, me);
                return true;
            }
        }
            break;
        case QEvent::Hide:
            if (!main || !(main->isMinimized() || main->isHidden())) {
                QMargins m = lay->contentsMargins();
                m.setRight(rh);
                lay->setContentsMargins(m);
                m_eventL << QEvent::Show;
            }
            break;
        case QEvent::Show:
        {
            QMargins m = lay->contentsMargins();
            m.setRight(rs);
            lay->setContentsMargins(m);
            m_eventL << QEvent::Hide;
        }
            break;
        default:
            break;
        }
    }
    return QObject::eventFilter(obj, event);
}

#include <QFileInfo>

EFAcceptDrops::EFAcceptDrops(QWidget *parent) :
    QObject(parent),
    m_max(0)
{
    parent->setAcceptDrops(true);
    parent->installEventFilter(this);
}

void EFAcceptDrops::setAcceptSuffixs(QStringList sfxs)
{
    m_acceptSfxs = sfxs;
}

void EFAcceptDrops::setAcceptMaxUrls(int max)
{
    m_max = max;
}

bool EFAcceptDrops::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::DragEnter:
    {
        QDragEnterEvent *de = static_cast<QDragEnterEvent *>(event);
        QStringList sl = urls(de->mimeData());
        sl.isEmpty() ? event->ignore(): event->accept();
        return true;
    }
        break;
    case QEvent::DragMove:
        event->accept();
        break;
    case QEvent::Drop:
    {
        QDropEvent *de = static_cast<QDropEvent *>(event);
        QStringList sl = urls(de->mimeData());
        sl.isEmpty() ? event->ignore(): event->accept();
        if (sl.isEmpty())
            event->ignore();
        else {
            event->accept();
            emit dropFiles(sl);
        }
        return true;
    }
        break;
    default:
        break;
    }

    return QObject::eventFilter(obj, event);
}

// 需要处理符号 # -> %23等
QStringList EFAcceptDrops::urls(const QMimeData *md)
{
    QStringList files;
    QString prefix = "file:///";
    foreach (QUrl url, md->urls()) {
        QString s = QUrl::fromPercentEncoding(url.toString().toUtf8());
        s.remove(prefix);
        QFileInfo fi(s);
        if (!m_acceptSfxs.isEmpty() && !m_acceptSfxs.contains(fi.suffix().toLower()))
            return QStringList();

        files << s;
    }
    if (0 != m_max && files.length() > m_max)
        return QStringList();
    return files;
}
