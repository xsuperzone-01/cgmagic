#ifndef DEFAULTTABLEPAGE_H
#define DEFAULTTABLEPAGE_H

#include <QWidget>
#include <QPointer>

namespace Ui {
class DefaultTablePage;
}

class DefaultTablePage : public QWidget
{
    Q_OBJECT

public:
    explicit DefaultTablePage(QWidget *parent = nullptr);
    ~DefaultTablePage();

private:
    Ui::DefaultTablePage *ui;
};

#endif // DEFAULTTABLEPAGE_H
