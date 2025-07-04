#ifndef PREVIEW_H
#define PREVIEW_H

#include <QWidget>
#include <QMouseEvent>
#include <QEvent>
#include <QPointer>

#include "common/basewidget.h"
#include "io/netdown.h"
#include "tool/xfunc.h"

namespace Ui {
class Preview;
}

class Preview : public BaseWidget
{
    Q_OBJECT

public:
    explicit Preview(BaseWidget *parent = 0);
    ~Preview();

    void reqView(int missionId, QString target);
    void loadView();

    void showError(QString err);
protected:
    void resizeEvent(QResizeEvent *e);
    bool event(QEvent *e);
private slots:
    void on_dingBtn_clicked();

    void on_right_clicked();

    void on_left_clicked();

    void loadPro(qint64 cur, qint64 total);

    void loadImage();

    void error(QNetworkReply::NetworkError err);
    void on_headMax_clicked();

private:
    Ui::Preview *ui;

    bool m_ding;
    QPointer<NetDown> m_netDown;
    QString m_save;

    int m_idx;
    int m_mid;
    QString m_target;

    QJsonObject m_preObj;

    int num = 0;
};

#endif // PREVIEW_H
