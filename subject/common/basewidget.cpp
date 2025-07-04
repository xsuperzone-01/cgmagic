#include "basewidget.h"

#include <qmath.h>
#include "config/userinfo.h"
#include "tool/xfunc.h"
#include "common/session.h"

#include <QLayout>
#include <QMenu>
#include <QGraphicsDropShadowEffect>
#include "view/backgroundmask.h"
#include <QScreen>
#include <view/set/set.h>
#include <QPainterPath>

#include <Windows.h>                //Windows平台的头文件，包含Windows API的声明和定义，允许编写与Windowss操作系统交互的程序（如窗口创建、消息处理、图形绘制、文件操作和设备通信等）
#include <WinUser.h>                //是Windows.h的一部分，包含用于用户界面（UI）编程的声明和定义（如窗口、消息和输入处理）
#include <windowsx.h>               //是用于简化和Widnows底层的交互，强化程序的安全性（包含很多有用的宏）
#include <dwmapi.h>                 //是Windows桌面管理器（DWM）API的一部分，负责管理和渲染桌面窗口，提供视觉效果（如透明窗口边框、窗口阴影和Aero主题）
#include <objidl.h>                 //是定义了与COM和OLE相关的接口和数据结构，允许应用程序和组件之间的进行复杂数据传输和对象管理
namespace Gdiplus
{
using std::min;
using std::max;
};
#include <gdiplus.h>                //是GDI库的一部分，用于Windows图形编程（如绘制图形、处理图像、绘制文本和处理复杂的图形操作）
#include <GdiPlusColor.h>           //是GDI+库的一部分，用于处理颜色相关的功能（如颜色的创建、转换和操作）
#pragma comment( lib, "gdiplus.lib" )
#pragma comment (lib,"Dwmapi.lib")
#pragma comment (lib,"user32.lib")

bool BaseWidget::isMouseInWorkArea = false;

BaseWidget::BaseWidget(QWidget *parent) :
    Widget(parent),
    m_autoMax(false),
    m_movePoint(QPoint(0,0)),
    m_shortCut(false),
    m_escClose(false),
    closeOnFocusOut(false),
    paintDropShadow(false),
    paintTopTriangle(false),
    canMove(true),
    keyMove(false),
    dragResize(false)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint);


    //mac多一圈阴影
#ifdef Q_OS_MAC
    setWindowFlag(Qt::NoDropShadowWindowHint);
#endif
}

BaseWidget::~BaseWidget()
{
}

bool BaseWidget::needMove(QMouseEvent *e)
{
    return canMove;
}

void BaseWidget::autoMax()
{
    emit autoMaxChanged();
}

void BaseWidget::openHotKey(QString name, int num)
{
    m_shortCut = true;
    this->grabShortcut(QKeySequence(Qt::Key_Control, Qt::Key_Alt, Qt::Key_9), Qt::ApplicationShortcut);
    this->grabShortcut(QKeySequence(Qt::Key_Control, Qt::Key_Alt, Qt::Key_8), Qt::ApplicationShortcut);
    this->grabShortcut(QKeySequence(Qt::Key_Control, Qt::Key_Alt, Qt::Key_7), Qt::ApplicationShortcut);
    this->grabShortcut(QKeySequence(Qt::Key_Control, Qt::Key_Alt, Qt::Key_0), Qt::ApplicationShortcut);
}

void BaseWidget::modaldel()
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_ShowModal, true);
}

void BaseWidget::escClose()
{
    modaldel();
    m_escClose = true;
}

void BaseWidget::setLineEdit(QLineEdit *le, QString text)
{
    le->setText(text);
    le->setToolTip(text);
    le->setCursorPosition(0);
}

void BaseWidget::setMinBtn(QPushButton *btn)
{
    connect(btn, &QPushButton::clicked, [=]{
#ifdef Q_OS_MAC
//        this->hide();
        MacUtil::showMinimized(window());
#else
        this->showMinimized();
#endif
    });
}

QStringList BaseWidget::dropUrls(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    QStringList list;
    foreach (QUrl url, urls) {
        QString file_path = url.toLocalFile();
#ifdef OS_WIN
        std::wstring ws = file_path.toStdWString();
        TCHAR achLongPath[1000] = { 0 };
        GetLongPathName(ws.data(), achLongPath, sizeof(achLongPath)/sizeof(TCHAR));
        file_path = QString::fromStdWString(achLongPath);
#endif

#ifdef Q_OS_MAC
//        MacUtil mu;
//        mu.qUrlToCFURLRef(url);
//        file_path = url.toLocalFile();
#endif
        if (!file_path.isEmpty())
            list.append(file_path);
    }
    qDebug() <<  __FUNCTION__ << list;
    return list;
}

void BaseWidget::showMenu(QMenu *menu)
{
    if (menu && !menu->actions().isEmpty()) {
        QPoint pos = XFunc::widScrPos(menu);
        pos.setX(pos.x() + (pos.x() < QCursor::pos().x() ? -3 : 3));
        pos.setY(pos.y() + (pos.y() < QCursor::pos().y() ? -3 : 3));

        menu->move(pos);
        menu->show();
    }
}

void BaseWidget::showMenuMid(QMenu *menu, QWidget *wid)
{
    if (menu && !menu->actions().isEmpty() && wid) {
        QPoint gp = wid->mapToGlobal(QPoint(0,0));
        menu->move(gp.x()+(wid->width()- menu->width())/2, gp.y() + wid->height() + 5);
        menu->show();
    }
}

void BaseWidget::setLabelText(QLabel *btn, QString text)
{
    btn->setText(text);
    btn->setToolTip(text);

    //TODO 能否自动获取padding 替代传入的plus
    int paddingMinus = 0 - btn->property(PADDING_MINUS).toInt();

    QTimer::singleShot(300, btn, [=]{
        QFontMetrics fm(btn->font());
        if (fm.horizontalAdvance(text) > btn->width() + paddingMinus) {
            btn->setText(fm.elidedText(text, Qt::ElideRight, btn->width() + paddingMinus));
        }
    });
}

void BaseWidget::setBtnLayout(QPushButton *btn)
{
#ifdef Q_OS_MAC
   btn->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
}

void BaseWidget::setMacLayout(QWidget *wid)
{
#ifdef Q_OS_MAC
   wid->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
}

void BaseWidget::setMacNoFocusRect(QWidget *wid)
{
#ifdef Q_OS_MAC
   wid->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
}

void BaseWidget::moveCenter(QWidget *parent)
{
    if (!parent) {
        parent = qobject_cast<QWidget *>(this->parent());
    }
    if (parent) {
        connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()), Qt::UniqueConnection);
        QPoint gp = parent->mapToGlobal(QPoint(0,0));
        move((parent->width()-width())/2, (parent->height()-height())/2);
    }
}

QWidget *BaseWidget::topLevelWidget()
{
    QWidget *w = qApp->activeWindow();
    while (w && w->parentWidget()) {
        w = w->parentWidget();
    }
    return w;
}

void BaseWidget::setClass(QWidget *wid, QString className)
{
    setProperty(wid, "class", className);
}

void BaseWidget::setProperty(QWidget *wid, QString name, QVariant value)
{
    if (!wid)
        return;
    wid->setProperty(name.toStdString().c_str(), value);
    wid->setStyle(qApp->style());
}

/*
    所有的parent都被强制转成 Login or MainWindow or 其他主界面
*/
void BaseWidget::setBackgroundMask(QWidget *parent)
{
    if (!parent || (parent != Session::instance()->LoginWid() && parent != Session::instance()->mainWid())) {
        parent = Session::instance()->mainWid();
    }

    canMove = false;
    BackgroundMask *mask = new BackgroundMask(parent);
    connect(this, &BaseWidget::destroyed, mask, &BackgroundMask::close);
    mask->setContentWidget(this);
}

void BaseWidget::setDropShadow(QWidget *wid, int x, int y, int radius, QColor color)
{
    if (QGraphicsEffect *ef = wid->graphicsEffect()) {
        ef->deleteLater();
    }
    QGraphicsDropShadowEffect *shadow_effect = new QGraphicsDropShadowEffect(wid);
    shadow_effect->setOffset(x, y);
    shadow_effect->setColor(color);
    shadow_effect->setBlurRadius(radius);
    wid->setGraphicsEffect(shadow_effect);
}

void BaseWidget::setLineHeight(QTextEdit *te, int height)
{
    QTextCursor tc = te->textCursor();
    QTextBlockFormat tb = tc.blockFormat();
    tb.setLineHeight(height, QTextBlockFormat::FixedHeight);//设置固定行高
    tc.setBlockFormat(tb);
    te->setTextCursor(tc);
}

void BaseWidget::setLetterSpacing(QWidget *wid, qreal space)
{
    QFont f = wid->font();
    f.setLetterSpacing(QFont::AbsoluteSpacing, space);
    wid->setFont(f);
}

/*
    x,y 必须是全局坐标
    1. 屏幕2在左，屏幕1在右，屏幕1为主显示器，当app出现在屏幕2时，x为负数
*/
void BaseWidget::adjustMove(QWidget *source, int x, int y)
{
    QPoint pos(x, y);
    QRect rect;
    if (QScreen *sat = qApp->screenAt(pos)) {
        rect = sat->availableGeometry();
    }
    if (!rect.isNull()) {
        while (pos.x() <= rect.x())
            pos.setX(pos.x() + 1);
        while (pos.x() + source->width() >= rect.width() + rect.x())
            pos.setX(pos.x() - 1);
        while (pos.y() <= rect.y())
            pos.setY(pos.y() + 1);
        while (pos.y() + source->height() >= rect.height() + rect.y())
            pos.setY(pos.y() - 1);
    }
    qDebug()<< __FUNCTION__ << QPoint(x, y) << "->" << pos << rect;
    source->move(pos);
}

QWidget *BaseWidget::blurEffectWidget()
{
    return m_blurWid;
}

void BaseWidget::setBlurEffectWidget(QWidget *wid)
{
    m_blurWid = wid;
}

void BaseWidget::deleteLayout(QLayout *lay)
{
    if (lay == nullptr)
        return;

    QLayoutItem *item = nullptr;
    while ((item = lay->takeAt(0)) != nullptr) {
        QLayout *subLay = item->layout();
        if (subLay != nullptr) {
            deleteLayout(subLay);
        } else {
            QSpacerItem *sp = item->spacerItem();
            if (sp != nullptr) {
                delete sp;
                sp = nullptr;
            } else {
                QWidget *wid = item->widget();
                if (wid != nullptr) {
                    QLayout *widLay = wid->layout();
                    if (widLay != nullptr) {
                        deleteLayout(widLay);
                    }
                    wid->setParent(nullptr);
                    wid->deleteLater();
                }
            }
        }
    }

    delete lay;
    lay = nullptr;
}

void BaseWidget::mouseMoveEvent(QMouseEvent *e)
{
    // if (m_movePoint.x() > 0) {
    //     if (m_autoMax && isMaximized()) {
    //         this->autoMax();
    //     }
    //     move(e->globalPos()-m_movePoint);
    // }
    // return QWidget::mouseMoveEvent(e);
}

void BaseWidget::mousePressEvent(QMouseEvent *e)
{
    // if (e->button() == Qt::LeftButton && needMove(e) && m_movePoint.x() != -1) {
    //     m_movePoint = e->globalPos() - pos();//最大化的时候pos坐标为(0,0)
    // }
    // return QWidget::mousePressEvent(e);
}

void BaseWidget::mouseReleaseEvent(QMouseEvent *e)
{
    // m_movePoint = QPoint(0,0);
    // if (m_autoMax) {
    //     if (e->globalY() < 5) {
    //         this->autoMax();
    //     }
    // }
    // return QWidget::mouseReleaseEvent(e);
}

void BaseWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    // if (m_autoMax) {
    //     m_movePoint = QPoint(-1, -1);
    //     this->autoMax();
    // }
    // return QWidget::mouseDoubleClickEvent(e);
}

#define ON_EDGE(v1, v2) v1 > v2 && v1 < v2 + 5
#define ON_EDGE2(v1, v2) v1 > v2 - 5 && v1 < v2 + 5


void BaseWidget::setResizeable(bool resizeable)
{
    m_bResizeable = resizeable;
    if (m_bResizeable){
        HWND hwnd = (HWND)this->winId();
        DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
        ::SetWindowLong(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);
    }else{
        HWND hwnd = (HWND)this->winId();
        DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
        ::SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME | WS_CAPTION);
    }

    const MARGINS shadow = {1, 1, 1, 1};
    ::DwmExtendFrameIntoClientArea(HWND(this->winId()), &shadow);
}

//设置窗口可调整大小的区域宽度
void BaseWidget::setResizeableAreaWidth(int width)
{
    if(width < 1){
        m_borderWidth = 1;
    }else{
        m_borderWidth = width;
    }
}

//设置窗口标题栏高度--（即鼠标的可拖动区域）
void BaseWidget::setTitleBar(QWidget* titlebar)
{
    m_titlebar = titlebar;
    m_titlebar->setAttribute(Qt::WA_Hover,true);
    m_titlebar->installEventFilter(this);
}

bool BaseWidget::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    MSG* msg = reinterpret_cast<MSG*>(message);
    switch (msg->message){
    case WM_NCCALCSIZE:{
        NCCALCSIZE_PARAMS& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
        if(params.rgrc[0].top != 0){
            params.rgrc[0].top -= 1;
        }else{}
        *result = WVR_REDRAW;
        return true;
    }
    case WM_NCHITTEST:{
        isMouseInWorkArea = false;
        *result = 0;

        const LONG borderWidth = m_borderWidth;
        RECT winrect;
        GetWindowRect(HWND(winId()), &winrect);

        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        if(m_bResizeable){
            bool resizeWidth = minimumWidth() != maximumWidth();
            bool resizeHeight = minimumHeight() != maximumHeight();

            if(resizeWidth){
                if(x >= winrect.left && x < winrect.left + borderWidth){
                    *result = HTLEFT;
                }else if(x < winrect.right && x >= winrect.right - borderWidth){
                    *result = HTRIGHT;
                }else{}
            }

            if(resizeHeight){
                if(y < winrect.bottom && y >= winrect.bottom - borderWidth){
                    *result = HTBOTTOM;
                }else if(y >= winrect.top && y < winrect.top + borderWidth){
                    *result = HTTOP;
                }
            }

            if(resizeWidth && resizeHeight){
                if(x >= winrect.left && x < winrect.left + borderWidth && y < winrect.bottom && y >= winrect.bottom - borderWidth){
                    *result = HTBOTTOMLEFT;
                }else if(x < winrect.right && x >= winrect.right - borderWidth && y < winrect.bottom && y >= winrect.bottom - borderWidth){
                    *result = HTBOTTOMRIGHT;
                }else if(x >= winrect.left && x < winrect.left + borderWidth && y >= winrect.top && y < winrect.top + borderWidth){
                    *result = HTTOPLEFT;
                }else if(x < winrect.right && x >= winrect.right - borderWidth && y >= winrect.top && y < winrect.top + borderWidth){
                    *result = HTTOPRIGHT;
                }else{}
            }
        }

        if(0 != *result){
            return true;
        }

        if(!m_titlebar){
            return false;
        }

        QPoint relativePosCursor = getCursorRelativePos();
        if(relativePosCursor.isNull()){
            return false;
        }else{
            if(!m_titlebar->rect().contains(relativePosCursor)){
                isMouseInWorkArea = true;
                return false;
            }else if(!m_titlebar->childAt(relativePosCursor)){
                *result = HTCAPTION;
                return true;
            }else{
                return false;
            }
        }

        return false;
    }
    case WM_GETMINMAXINFO:{
        if (::IsZoomed(msg->hwnd)){
            RECT frame = {0, 0, 0, 0};
            ::AdjustWindowRectEx(&frame, WS_THICKFRAME, FALSE, 0);

            double dpr = this->devicePixelRatioF();
            m_frames.setLeft(abs(frame.left)/dpr + 0.5);
            m_frames.setTop(abs(frame.bottom)/dpr + 0.5);
            m_frames.setRight(abs(frame.right)/dpr + 0.5);
            m_frames.setBottom(abs(frame.bottom)/dpr + 0.5);

            QWidget::setContentsMargins(m_frames.left() + m_margins.left(),
                                        m_frames.top() + m_margins.top(),
                                        m_frames.right() + m_margins.right(),
                                        m_frames.bottom() + m_margins.bottom());
            m_bJustMaximized = true;
        }else{
            if (m_bJustMaximized){
                QWidget::setContentsMargins(m_margins);
                m_frames = QMargins();
                m_bJustMaximized = false;
            }else{
            }
        }
        return false;
    }
    default:
        return QWidget::nativeEvent(eventType, message, result);
    }
}

QPoint BaseWidget::getCursorRelativePos(){
    QRect globalRect = this->frameGeometry();
    if(!globalRect.isValid()){
        return QPoint();
    }

    QPoint globalPos = QCursor::pos();
    QScreen* screenA = QGuiApplication::screenAt(globalPos);
    if(!screenA){
        return QPoint();
    }
    QRect screenRectA = screenA->geometry();
    QPoint relativePosA = globalPos - screenRectA.topLeft();

    QScreen* screen = QGuiApplication::screenAt(this->pos());
    if(!screen){
        return QPoint();
    }

    QRect screenRect = screen->geometry();
    QRect relativeRect = globalRect.translated(-screenRect.topLeft());

    QPoint globalPosTitle = m_titlebar->mapToGlobal(QPoint(0, 0));
    QScreen* screenTitle = QGuiApplication::screenAt(globalPosTitle);
    if(!screenTitle){
        return QPoint();
    }

    QRect screenRectTitle = screenTitle->geometry();
    QPoint relativePosTitle = globalPosTitle - screenRectTitle.topLeft();
    QPoint relativePosCursor = relativePosA - relativePosTitle;

    return relativePosCursor;
}

void BaseWidget::paintEvent(QPaintEvent *)
{
    //绘制边框阴影
    if (paintDropShadow) {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.addRect(4, 4, this->width()-20, this->height()-20);


        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        int shadowWidth = 10;
        QColor color("transparent");
        for (int i = 0; i < shadowWidth; i++) {
            QPainterPath path;
            path.setFillRule(Qt::WindingFill);

            path.addRoundedRect(shadowWidth-i, shadowWidth-i, this->width()-(shadowWidth-i)*2, this->height()-(shadowWidth-i)*2, 1, 1);


            color.setAlpha(30 - i * 3);
            painter.setPen(color);
            painter.drawPath(path);
        }
    }

    //绘制顶部三角
    if (paintTopTriangle) {
        QPainter painter(this);
        QPainterPath path;
        int m_startX = this->width()/2;
        int m_startY =  5;
        // 小三角区域;
        QPolygon trianglePolygon;
        trianglePolygon << QPoint(m_startX, m_startY);
        trianglePolygon << QPoint(m_startX + 5, m_startY + 5);
        trianglePolygon << QPoint(m_startX - 5, m_startY + 5);

        path.addPolygon(trianglePolygon);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#ffffff"));
        painter.drawPath(path);
    }
}

void BaseWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void BaseWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void BaseWidget::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if(urls.isEmpty())
        return;
    QStringList list = dropUrls(event);
    emit dropFileList(list);
}

void BaseWidget::keyPressEvent(QKeyEvent *e)
{
    if (m_escClose) {
        switch (e->key()) {
        case Qt::Key_Escape:
            this->close();
            break;
        }
    }

    if (m_shortCut) {
        Qt::KeyboardModifiers modifiers = e->modifiers();
        if((modifiers & Qt::ControlModifier) && (modifiers & Qt::AltModifier)) {
            int num = e->key();
            if (num == Qt::Key_8) {
                if (!Session::instance()->m_debugText) {
                    QTextEdit* dt = new QTextEdit;
                    if (dt) {
                        dt->setObjectName("debugTextEdit");
                        dt->show();dt->resize(600, 400);
                        Session::instance()->m_debugText = dt;
                    }
                } else
                    Session::instance()->m_debugText->deleteLater();
            }
            if (num == Qt::Key_9) {
                XFunc::openDir(UserInfo::instance()->allUserPath());
            }
            if (Qt::Key_7 == num) {
                XFunc::openDir(UserInfo::instance()->getUserPath());
            }
        }
    }
    return QWidget::keyPressEvent(e);
}

void BaseWidget::headMaxHand(QPushButton *btn)
{
    btn->setProperty("maximize", isMaximized() ? "maxmax" : "maxmin");
    btn->setStyle(qApp->style());

    if (btn->property("maximize") == "maxmax") {
        QString s;
        s = QObject::tr("最大化");
        btn->setToolTip(s);
    } else {
        QString s;
        s = QObject::tr("还原");
        btn->setToolTip(s);
    }

    QLayout* lay = this->layout();
    //最大化时消除阴影
    if (this->property("previousMargin").isNull()) {
        QMargins m = lay->contentsMargins();
        setProperty(this, "previousMargin", QString("%1|%2|%3|%4").arg(m.left()).arg(m.top()).arg(m.right()).arg(m.bottom()));
    }

    QList<QString> mgL = this->property("previousMargin").toString().split("|");
    QMargins mg;
    if (isMaximized()) {
        if (mgL.length() >= 4) {
            mg = QMargins(mgL.at(0).toInt(), mgL.at(1).toInt(), mgL.at(2).toInt(), mgL.at(3).toInt());
        }
    }
    lay->setContentsMargins(mg);

    //处于屏幕左上角时无法最大化？因为设置了FramelessWindowHint、WA_TranslucentBackground
    if (!isMaximized()) {
        foreach (QScreen *sc, qApp->screens()) {
            if (pos() == sc->availableGeometry().topLeft()) {
                move(pos().x(), pos().y() + 1);
                break;
            }
        }
        showMaximized();
    } else {
        showNormal();
    }
}

void BaseWidget::focusOutEvent(QFocusEvent *e)
{
    return Widget::focusOutEvent(e);
}

void BaseWidget::setDropShadow()
{
    QGraphicsDropShadowEffect *shadow_effect = new QGraphicsDropShadowEffect(this);
    shadow_effect->setOffset(0, 0);
    shadow_effect->setColor(QColor("#999999"));
    shadow_effect->setBlurRadius(4);
    this->setGraphicsEffect(shadow_effect);
}

void BaseWidget::setCloseOnFocusOut(bool close)
{
    closeOnFocusOut = close;
    installEventFilter(this);
}

bool BaseWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (closeOnFocusOut) {
        if (event->type() == QEvent::ActivationChange) {
            QWidget *aw = QApplication::activeWindow();
            if (aw != this) {
#ifdef Q_OS_MAC
                if (aw && aw->objectName() != "PackageRenewalTips") {
#else
                if (1) {
#endif
                    close();
                }
            }
        }
    }
    if (keyMove && watched == this) {
        if (event->type() == QEvent::KeyRelease) {
            QPoint pos = this->mapToGlobal(QPoint(0, 0));
            QKeyEvent *ke = (QKeyEvent *)event;
            if (ke->key() == Qt::Key_Left) {
                pos.setX(pos.x()-1);
            } else if (ke->key() == Qt::Key_Up) {
                pos.setY(pos.y()-1);
            } else if (ke->key() == Qt::Key_Right) {
                pos.setX(pos.x()+1);
            } else if (ke->key() == Qt::Key_Down) {
                pos.setY(pos.y()+1);
            }
            this->move(pos);
        }
    }
    return Widget::eventFilter(watched, event);
}

void BaseWidget::setDropShadow(QWidget *wid)
{
    paintDropShadow = false;
    QGraphicsDropShadowEffect *shadow_effect = new QGraphicsDropShadowEffect(this);
    shadow_effect->setOffset(0, 2);
    shadow_effect->setColor(QColor(0, 0, 0, 35));
    shadow_effect->setBlurRadius(10);
    wid->setGraphicsEffect(shadow_effect);
}
