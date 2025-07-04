#ifndef MASKCONTENT_H
#define MASKCONTENT_H

#include "common/basewidget.h"

namespace Ui {
class MaskContent;
}

class MaskContent : public BaseWidget
{
    Q_OBJECT

public:
    explicit MaskContent(QWidget *parent = nullptr);
    ~MaskContent();

    void setContentWidget(QWidget *wid);

private:
    Ui::MaskContent *ui;
    QPointer<QWidget> content;
};

#endif // MASKCONTENT_H
