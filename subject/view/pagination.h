#ifndef PAGINATION_H
#define PAGINATION_H

#include "common/debounce.h"
#include "view/Item/widget.h"

namespace Ui {
class Pagination;
}

class Pagination : public Widget
{
    Q_OBJECT

public:
    explicit Pagination(QWidget *parent = 0);
    ~Pagination();

    void restore(bool needChange = false);
    void setMaxPage(int max);
    int page();
signals:
    void pageChanged(int page);
public slots:
    void setPage(int page);
    void on_pre_clicked();
    void on_next_clicked();
private slots:
    void on_first_clicked();
    void on_last_clicked();
    void pageChange(int arg1);

private:
    void setBtnEnable();
private:
    Ui::Pagination *ui;

    int m_page;
    Debounce m_debounce;
};

#endif // PAGINATION_H
