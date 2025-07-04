#pragma execution_character_set("utf-8")
#ifndef SOFTWAREUPDATEITEM_H
#define SOFTWAREUPDATEITEM_H

#include <QJsonObject>
#include "view/Item/widget.h"

namespace Ui {
class SoftwareUpdateItem;
}

class SoftwareUpdateItem : public Widget
{
    Q_OBJECT

public:
    explicit SoftwareUpdateItem(QWidget *parent = nullptr);
    ~SoftwareUpdateItem();

    void initItem(const QJsonObject item);

private:
    Ui::SoftwareUpdateItem *ui;
};

#endif // SOFTWAREUPDATEITEM_H
