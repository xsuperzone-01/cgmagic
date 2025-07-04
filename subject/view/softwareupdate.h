#ifndef SOFTWAREUPDATE_H
#define SOFTWAREUPDATE_H

#include "common/basewidget.h"
#include <QJsonObject>
#include <QTimer>

namespace Ui {
class SoftwareUpdate;
}

class SoftwareUpdate : public BaseWidget
{
    Q_OBJECT

public:
    explicit SoftwareUpdate(QWidget *parent = nullptr);
    ~SoftwareUpdate();

    void initDetail(const QJsonObject obj, bool autoStart = false, bool force = false);

    bool admin();

private:
    void removeDetail();

    void update();

    void showUpdateStatus(int status);

    void addShadow(QWidget *w);

signals:
    void nextClicked();

private slots:
    void timeOut();

    void on_okBtn_clicked();

    void force();

    void on_noBtn_clicked();

    void on_headMin_clicked();

    void on_headClose_clicked();

private:
    Ui::SoftwareUpdate *ui;

    QTimer m_timer;
    QList<int> m_updateSoftIds;
    bool m_admin;
    bool m_force;
    bool needMove(QMouseEvent *e);
};

#endif // SOFTWAREUPDATE_H
