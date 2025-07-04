#ifndef CHOOSEMAXVERSION_H
#define CHOOSEMAXVERSION_H

#include <QWidget>
#include "common/basewidget.h"

namespace Ui {
class ChooseMaxVersion;
}

class ChooseMaxVersion : public BaseWidget
{
    Q_OBJECT

public:
    explicit ChooseMaxVersion(QWidget *parent = 0);
    ~ChooseMaxVersion();

    void init(QStringList files);

signals:
    void postJobSig(QStringList files, QString srcVer, QString tarVer);
    void error(QString text);

private slots:
    void on_headClose_clicked();

    void on_noBtn_clicked();

    void on_okBtn_clicked();

    void initSlt();
private:
    Ui::ChooseMaxVersion *ui;

    QStringList m_files;
};

#endif // CHOOSEMAXVERSION_H
