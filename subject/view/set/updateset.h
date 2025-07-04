#ifndef UPDATESET_H
#define UPDATESET_H

#include <QWidget>
#include <QPointer>
#include "updatecontent.h"
#include "ui_updatecontent.h"

#include "versions/versionmanager.h"


class UpdateContent;

namespace Ui {
class UpdateSet;
}

class UpdateSet : public QWidget
{
    Q_OBJECT

public:
    explicit UpdateSet(QWidget *parent = nullptr);
    ~UpdateSet();

    void initUpdateSet(int c, int p);

    void update();

    void updated();

    void clearModule();

    void insertLay();

    void fileRead();

    void on_client_clicked();

    void on_module_clicked();

    void clientClear();
    void clientShow();

    void moduleClear();
    void moduleShow();
    void moduleAdd();

    void classSet(int c, int p);

signals:
    void btnEnabled();

    void haveUpdated();

public slots:
    void enabled(bool o);

private:
    void removePlugins();

private:
    Ui::UpdateSet *ui;

    QPointer<UpdateContent> m_client;
    QList<QPointer<UpdateContent>> m_module;

    QString v;
    int num_p;
    int num_t;

};

#endif // UPDATESET_H
