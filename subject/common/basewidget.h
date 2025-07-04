#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <QMouseEvent>
#include <QMainWindow>
#define NOMINMAX
#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif
#include <QPaintEvent>
#include <QPainter>
#include <QKeyEvent>
#include <QPointer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QLineEdit>
#include <QPushButton>
#include "view/Item/widget.h"
#include <QLabel>
#include <QTextEdit>

#define PADDING_MINUS "paddingMinus"

class BaseWidget : public Widget
{
    Q_OBJECT
public:
    explicit BaseWidget(QWidget *parent = 0);
    ~BaseWidget();

    virtual bool needMove(QMouseEvent *e);
    virtual void autoMax();

    void openHotKey(QString name, int num);
    void modaldel();
    void escClose();

    static void setLineEdit(QLineEdit* le, QString text);
    void setMinBtn(QPushButton* btn);

    static QStringList dropUrls(QDropEvent* event);

    static void showMenu(QMenu* menu);
    static void showMenuMid(QMenu* menu, QWidget *wid = NULL);

    static void setLabelText(QLabel *btn, QString text);

    static void setBtnLayout(QPushButton *btn);
    static void setMacLayout(QWidget *wid);
    static void setMacNoFocusRect(QWidget *wid);

    void moveCenter(QWidget *parent = NULL);

    static QWidget *topLevelWidget();

    static void setClass(QWidget *wid, QString className);
    static void setProperty(QWidget *wid, QString name, QVariant value);

    void setBackgroundMask(QWidget *parent = NULL);

    static void setDropShadow(QWidget *wid, int x, int y, int radius, QColor color);

    static void setLineHeight(QTextEdit *te, int height);
    static void setLetterSpacing(QWidget *wid, qreal space);

    static void adjustMove(QWidget *source, int x, int y);

    QWidget *blurEffectWidget();
    void setBlurEffectWidget(QWidget *wid);

    static void deleteLayout(QLayout *lay);

    void setResizeable(bool resizeable=true);       //设置是否可以通过鼠标调整窗口大小
    void setResizeableAreaWidth(int width = 5);     //设置可调整大小区域的宽度
    void setTitleBar(QWidget* titlebar);            //设置一个标题栏--（即鼠标光标可拖动区域，参数是Widget）

signals:
    void hotKey(int);
    void dropFileList(QStringList);
    void autoMaxChanged();

protected:
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);

    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result);
    void paintEvent(QPaintEvent *);

    void dragMoveEvent(QDragMoveEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

    void keyPressEvent(QKeyEvent *e);

    void headMaxHand(QPushButton* btn);

    void focusOutEvent(QFocusEvent *e);

    void setDropShadow();

    void setCloseOnFocusOut(bool close = true);

    bool eventFilter(QObject *watched, QEvent *event);

    void setDropShadow(QWidget *wid);

private:
    QPoint getCursorRelativePos();

public:
    bool canMove;
    bool keyMove;

protected:
    bool m_autoMax;
    bool closeOnFocusOut;
    bool paintDropShadow;
    bool paintTopTriangle;
    bool dragResize;

private:
    QPoint m_movePoint;
    bool m_shortCut;
    bool m_escClose;
    QPointer<QWidget> m_blurWid;

    QPointer<QWidget> m_titlebar;
    int m_borderWidth = 5;

    QMargins m_margins;
    QMargins m_frames;

    bool m_bJustMaximized = false;
    bool m_bResizeable = true;

public:
    static bool isMouseInWorkArea;  //鼠标是否在工作区
};

#endif // BASEWIDGET_H
