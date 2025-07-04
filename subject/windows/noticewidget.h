#ifndef NOTICEWIDGET_H
#define NOTICEWIDGET_H

#include <QWidget>
#include "common/basewidget.h"
#include "tool/webview.h"
#include <QPointer>

namespace Ui {
class NoticeWidget;
}

class NoticeWidget : public BaseWidget
{
    Q_OBJECT

public:
    ~NoticeWidget();

private:
    Ui::NoticeWidget *ui;

public:
    static NoticeWidget* getInstance();

    void showWindow();   //设置窗口显示

    void addWebPage(QString url);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

public slots:
    void on_closeBtn_clicked();

public:
    explicit NoticeWidget(BaseWidget *parent = nullptr);

    static NoticeWidget* noticeWidget;

private:
    QPoint mousePoint;
    bool mousePress;
};

#endif // NOTICEWIDGET_H
