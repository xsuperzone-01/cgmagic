#include "baseprogressbar.h"

BaseProgressBar::BaseProgressBar(QWidget *parent) :
    QProgressBar(parent)
{

}

DEFINE_RESIZE(BaseProgressBar, QProgressBar);
