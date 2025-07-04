#ifndef TRANSFER_H
#define TRANSFER_H

#include <QWidget>
#include <QResizeEvent>
#include <QTimer>

#include "common/headview.h"
#include "transfer/transset.h"
#include <QMenu>
#include <QPointer>
#include <QTableWidgetItem>


class MsgBox;

namespace Ui {
class Transfer;
}

class Transfer : public QWidget
{
    Q_OBJECT

public:
    enum UploadTableCol {
        Check,
        Name,
        Size,
        Speed,
        State,
        Op
    };
    enum DownloadTableCol {
        DCheck,
        DOrder,
        DName,
        DCamera,
        DOut,
        DSize,
        DSpeed,
        DState,
        DOp
    };

    explicit Transfer(QWidget *parent = 0);
    ~Transfer();

    void initUpload();
    void initDownload();

    void refresh();

    void hideDelBatchBtn(bool isHide);  //是否隐藏批量删除按钮

protected:
    void resizeEvent(QResizeEvent *e);

private slots:
    void allFile(QList<db_upfile> fl);
    void allFile(QList<db_downfile> fl);
    void refreshUp();

    void on_delBatchBtn_clicked();

public slots:
    void resizeDownTableItemWidth(double ratio);
    void resizeUpTableItemWidth(double ratio);

private:
    QString headSecSize(int idx);
    QColor stateColor(int state);

    QList<db_downfile> selectedDownFile();
    void checkAction(QAction* ac, QList<int> stateL);

    int progressState(int state);

private:
    Ui::Transfer *ui;

    HeadView* m_head;
    QList<int> m_headSize;
    QStringList m_fileHead;
    QMenu* m_pageMenu;
    int m_curPage; //0共同 1上传 2下载
    int m_curState;

    QMenu* m_menu;
    QAction* m_delAc;
    QAction* m_startAc;
    QAction* m_pauseAc;

    QTimer m_timer;
};
#endif // TRANSFER_H
