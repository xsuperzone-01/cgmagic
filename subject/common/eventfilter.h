#ifndef EVENTFILTER_H
#define EVENTFILTER_H

#include <QObject>
#include <QWidget>
#include <QEvent>
#include <QPushButton>
#include <QPointer>
#include <QEventLoop>

class EventFilter : public QObject
{
    Q_OBJECT
public:
    explicit EventFilter(QObject *parent = nullptr);

    void bindEvent(QWidget *wid1, QWidget *wid2, QPushButton *btn1, QPushButton *btn2);

    void inputMethod(QWidget *w);

protected:
    bool eventFilter(QObject* obj, QEvent *event);

private:
    QPointer<QWidget> w1;
    QPointer<QWidget> w2;
    QPointer<QPushButton> b1;
    QPointer<QPushButton> b2;

    QPointer<QWidget> inputMethodWid;
    QEvent::Type prevEventType;
};

class EFAlignCenter : public QObject
{
    Q_OBJECT

public:
    explicit EFAlignCenter(QObject *parent = nullptr);

    static void moveCenter(QWidget *wid, QWidget *parent);

protected:
    bool eventFilter(QObject *watched, QEvent *event);
};


class EFResizeToParent : public QObject
{
    Q_OBJECT

public:
    explicit EFResizeToParent(QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event);
};

#include <QTableWidget>
class EFTableRowHover : public QObject
{
    Q_OBJECT

public:
    explicit EFTableRowHover(QTableWidget *table, QColor rowColor, QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event);

private:
    QColor m_rowColor;
    QTableWidget *m_table;
};

class EFPaintDropShadow : public QObject
{
    Q_OBJECT

public:
    explicit EFPaintDropShadow(int x, int y, int radius, QColor color, QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event);

private:
    int m_x;
    int m_y;
    int m_radius;
    QColor m_color;
    QPointer<QWidget> m_wid;
};

#include <QGraphicsDropShadowEffect>
class GraphicsDropShadowEffect : public QGraphicsDropShadowEffect
{
    Q_OBJECT

public:
    explicit GraphicsDropShadowEffect(QObject *parent = nullptr);

    void setPaintCount(int c);

signals:
    void stopRequest();

protected:
    void draw(QPainter *painter);

private:
    int m_pc;
    int m_pcMax;
};

class EFDropShadow : public QObject
{
    Q_OBJECT
public:
    explicit EFDropShadow(QWidget *source, int x, int y, int radius, QColor color, QWidget *parent = nullptr);

    void startTargetEffect(int pc);

protected:
    bool eventFilter(QObject *watched, QEvent *event);

private:
    QPointer<QWidget> m_source;
    QPointer<QWidget> m_target;
};

class EFFileOpen : public QObject
{
    Q_OBJECT
public:
    explicit EFFileOpen(QApplication *app, QEventLoop *loop);

    QString scheme();

protected:
    bool eventFilter(QObject *watched, QEvent *event);

private:
    QPointer<QEventLoop> m_loop;
    QString m_scheme;
};

#include <QEventLoop>

class EventLoop : public QEventLoop
{
    Q_OBJECT
public:
    explicit EventLoop(int msec = 0, QObject *parent = nullptr);

    bool execUntilAppQuit();

private slots:
    void appQuit();

private:
    static bool m_appQuit;
};

#include <QScrollBar>
#include <QLayout>

class ScrollBarEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit ScrollBarEventFilter(QObject *parent = nullptr);

    void bindEvent(QScrollBar *bar, QLayout *lay, int rightHide, int rightShow = 0);
    void setMainWindow(QWidget *mainwindow);

protected:
    bool eventFilter(QObject* obj, QEvent *event);

private:
    QPointer<QScrollBar> sb;
    QPointer<QLayout> lay;
    QPointer<QWidget> main;
    int rh;
    int rs;
    QList<int> m_eventL;
};

class EFAcceptDrops : public QObject
{
    Q_OBJECT
public:
    explicit EFAcceptDrops(QWidget *parent = nullptr);

    void setAcceptSuffixs(QStringList sfxs);
    void setAcceptMaxUrls(int max);

protected:
    bool eventFilter(QObject* obj, QEvent *event);

signals:
    void dropFiles(QStringList files);

private:
    QStringList urls(const QMimeData *md);

    QStringList m_acceptSfxs;
    int m_max;
};

#endif // EVENTFILTER_H
