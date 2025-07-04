#ifndef Change_H
#define Change_H

#include <QWidget>
#include <QResizeEvent>

#include "common/headview.h"
#include <QMenu>
#include <QPointer>
#include <QTableWidgetItem>
#include <QJsonObject>

class MsgBox;

namespace Ui {
class Change;
}

class Change : public QWidget
{
    Q_OBJECT

public:
    enum TableCol {
        Empty,
        Version,
        Name,
        Time,
        State,
        Op
    };

    explicit Change(QWidget *parent = 0);
    ~Change();

    bool init();

    Q_INVOKABLE void downCover(int mid);
    Q_INVOKABLE void downErrorTip(QString tip);

    Q_INVOKABLE void listFile(QJsonObject obj);
    Q_INVOKABLE void listFileCache(QJsonObject obj);

    static void setDefaultPage(QTableWidget *table, QString text);

protected:
    void resizeEvent(QResizeEvent *e);

public slots:
    void updateSet();
    void resizeTableItemWidth(double ratio);

private slots:
    void refreshUp();
    void refreshCache();
    void postJobSlt(QStringList files, QString srcVer, QString tarVer);
    void downloadJobSlt(int jobId);
    void openDirSlt(int jobId);

    void on_selectFile_clicked();

private:
    QString headSecSize(int idx);
    QString stateText(int state);

    void checkAction(QAction* ac, QList<int> stateL);

    void showChooseMaxVersion(QStringList files);

    void mousePressEvent(QMouseEvent* event);



    int progressState(int state);
    QList<int> selectedIds();

private:
    Ui::Change *ui;

    HeadView* m_head;
    QList<int> m_headSize;
    QStringList m_fileHead;
    QMenu* m_pageMenu;
    int m_curState;

    QPointer<MsgBox> m_errMsgBox;

    QMenu* m_menu;
    QAction* m_cancelAc;
    QAction *m_downloadAc;
    QAction *m_openDownloadDir;
    QAction* m_delAc;
};

#endif // Change_H
