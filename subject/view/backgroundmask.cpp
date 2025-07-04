#include "backgroundmask.h"
#include "ui_backgroundmask.h"

#include <QBitmap>
#include "common/session.h"
#include "common/eventfilter.h"

#define BLUR_COUNT "blurCount"

QList<QPointer<BackgroundMask>> BackgroundMask::maskL;

BackgroundMask::BackgroundMask(QWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::BackgroundMask),
    firstMove(0)
{
    ui->setupUi(this);

    paintDropShadow = false;

    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose);

    QWidget *base = parent;
    if (!base) {
        base = Session::instance()->mainWid();
    }

    setClass(ui->widget, "backgroundMask");

    setSize(base);
    new EFResizeToParent(this);

    if (BaseWidget *bw = qobject_cast<BaseWidget *>(base)) {
        connect(bw, &BaseWidget::autoMaxChanged, this, [=]{
            setSize(bw);
        });
    }

    foreach (QPointer<BackgroundMask> mask, maskL) {
        if (!mask) {
            maskL.removeOne(mask);
        }
    }
    maskL << this;

    if (parent) {
        connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
        QPoint gp = parent->mapToGlobal(QPoint(0,0));
        move(gp.x(), gp.y());
    }

}

BackgroundMask::~BackgroundMask()
{
    if (BaseWidget *bw = qobject_cast<BaseWidget *>(this->parentWidget())) {
        if (QWidget *bew = bw->blurEffectWidget()) {
            int c = bew->property(BLUR_COUNT).toInt() - 1;
            bew->setProperty(BLUR_COUNT, c);
            if (c <= 0) {
                bew->setGraphicsEffect(NULL);
                bw->clearMask();
            }
        }
    }

    maskL.removeOne(this);
    keepOneMask();

    //EFDropShadow接受处理
    QEvent *event = new QEvent(QEvent::ApplicationActivate);
    QApplication::postEvent(qApp, event);

    delete ui;
}

void BackgroundMask::setContentWidget(BaseWidget *wid)
{
    //MsgBox未设置固定大小，置入后会被拉大
    wid->setFixedSize(wid->size());

    content = wid;
    ui->widget_2->layout()->addWidget(content);

    setAttribute(Qt::WA_ShowModal);
    show();
    keepOneMask();

    //EFDropShadow接受处理
    QEvent *event = new QEvent(QEvent::ApplicationActivate);
    QApplication::postEvent(qApp, event);
}

QWidget *BackgroundMask::background()
{
    return ui->widget;
}

void BackgroundMask::resizeEvent(QResizeEvent *event)
{
    setBlurMask();
    BaseWidget::resizeEvent(event);
}

void BackgroundMask::moveEvent(QMoveEvent *event)
{
        if (parentWidget())
            parentWidget()->move(pos());
        for (int i = 0; i < maskL.length(); i++) {
            QPointer<BackgroundMask> mask = maskL.at(i);
            if (mask) {
                mask->move(pos());
            }
        }
    BaseWidget::moveEvent(event);
}
/*
    处于最上层的mask保持灰色，其他全部取消灰色
*/
void BackgroundMask::keepOneMask()
{
    for (int i = 0; i < maskL.length(); i++) {
        QPointer<BackgroundMask> mask = maskL.at(i);
        if (mask) {
            QString tab = i != maskL.length()-1 ? "0" : "1";
            setProperty(mask->background(), "tab", tab);
        }
    }
}

void BackgroundMask::setSize(QWidget *parent)
{
    layout()->setContentsMargins(parent->layout()->contentsMargins());
    setOriginalFixedSize(parent->size());
}

//QGraphicsBlurEffect会导致圆角变不圆
void BackgroundMask::setBlurMask()
{
    if (BaseWidget *bw = qobject_cast<BaseWidget *>(parentWidget())) {
        if (QWidget *bew = bw->blurEffectWidget()) {
            if (qobject_cast<QGraphicsBlurEffect *>(bew->graphicsEffect())) {
                QBitmap bmp(size());
                bmp.fill(Qt::color0);
                QPainter p(&bmp);
                p.setPen(Qt::NoPen);
                p.setBrush(Qt::color1);
                p.drawRoundedRect(QRect(0, 0, size().width(), size().height()), 8, 8);
                bw->setMask(bmp);
            }
        }
    }
}
