#include "basegroupbox.h"

BaseGroupBox::BaseGroupBox(QWidget *parent) :
    QGroupBox(parent)
{

}

DEFINE_RESIZE(BaseGroupBox, QGroupBox);
MARGINS_RESIZE_NAME(BaseGroupBox);
