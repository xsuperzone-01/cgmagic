#ifndef PROGRESS_H
#define PROGRESS_H

#include <QWidget>

namespace Ui {
class Progress;
}

class Progress : public QWidget
{
    Q_OBJECT

public:
    enum State {
        Ing,
        Err,
        Succ
    };

    explicit Progress(QWidget *parent = nullptr);
    ~Progress();

    void initProgress(int progress, QString status, State state);
    void setProgress(int progress);
    void setStatus(QString status);
    void setState(State state);

    void setPercentage(int progress, QString status, State state);

private:
    Ui::Progress *ui;
};

#endif // PROGRESS_H
