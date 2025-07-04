#include "basespinbox.h"

BaseSpinBox::BaseSpinBox(QWidget *parent) :
    QSpinBox(parent)
{

}

DEFINE_RESIZE(BaseSpinBox, QSpinBox);
