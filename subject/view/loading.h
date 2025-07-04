#ifndef LOADING_H
#define LOADING_H

#include <QWidget>

namespace Ui {
class Loading;
}

class Loading : public QWidget
{
    Q_OBJECT

public:
    explicit Loading(QWidget *parent = nullptr);
    ~Loading();

    void initLoading(QString png = QString());
    void stopTimer();

protected:
    void timerEvent(QTimerEvent *event);

private:
    Ui::Loading *ui;

    QList<QString> m_pngL;
    int m_idx;
    int m_timerId;
};

#endif // LOADING_H
