#ifndef HEADSEARCH_H
#define HEADSEARCH_H

#include <QWidget>
#include <QTimer>

namespace Ui {
class HeadSearch;
}

class HeadSearch : public QWidget
{
    Q_OBJECT

public:
    explicit HeadSearch(QWidget *parent = 0);
    ~HeadSearch();

    void hideMenu();
    void hideRefresh();
    void emptyText(QString text);
    void hideSearch(bool hide);
    void hideSearch();
protected:
    bool eventFilter(QObject *o, QEvent *e);
signals:
    void searchName(int type, QString name);
    void searchMove();
private slots:
    void on_search_clicked();

    void on_name_returnPressed();

    void on_refresh_clicked();

    void on_menu_clicked();

    void textChanged(QString text);
    void searchOut();
private:
    Ui::HeadSearch *ui;

    QTimer m_timer;
};

#endif // HEADSEARCH_H
