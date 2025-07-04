#ifndef BASEPROGRESSBAR_H
#define BASEPROGRESSBAR_H

#include <QProgressBar>
#include "common/BaseScale.h"

class BaseProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit BaseProgressBar(QWidget *parent = nullptr);

    DECLARE_RESIZE();

signals:

};

#endif // BASEPROGRESSBAR_H
