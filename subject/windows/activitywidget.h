#ifndef ACTIVITYWIDGET_H
#define ACTIVITYWIDGET_H

#include <QWidget>
#include <QList>
#include <QJsonObject>
#include <QMouseEvent>
#include "common/basewidget.h"

namespace Ui {
class ActivityWidget;
}

class ActivityWidget : public BaseWidget
{
    Q_OBJECT

public:
    explicit ActivityWidget(BaseWidget *parent = nullptr);
    ~ActivityWidget();

private:
    Ui::ActivityWidget *ui;

    QList<QJsonObject> imageData;

private:
    void adjustLabByImageNum(int num); //根据图片数量调整

signals:
    void jumpUrlInfo(QString url, int isInAppJump);

public slots:
    void setImageData(QList<QJsonObject> imageData);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // ACTIVITYWIDGET_H
