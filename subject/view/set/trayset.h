#ifndef TRAYSET_H
#define TRAYSET_H

#include <QObject>
#include <QWidgetAction>
#include <QPointer>
#include <QThread>

class Menu;
class MenuItem;
class LeftNavItem;
class TraySetAccount;

class TraySet : public QObject
{
    Q_OBJECT
public:
    explicit TraySet(QObject *parent = nullptr);
    ~TraySet();

    QMenu *newTrayMenu();

    void showMainWindow();

    QString mbUtil(QString cmd, QStringList args = QStringList());

    Q_INVOKABLE void showNetworkErrorTip(QString errorMessage);  //显示网络错误的弹窗提示

private:
    Menu *newFrameMenu(Menu *parent);
    void reviseMaxLanguage();           //修改插件中英文

signals:

private:
    QPointer<LeftNavItem> m_autoRunItem;
    QList<MenuItem> m_miL;
    QPointer<TraySetAccount> m_account;
    QVariant m_pluginBlack;

    QPointer<QWidget> networkErrorWid;  //网络错误弹窗
    QString freeTime;
    QStringList vipModelsList;
    QStringList nonVipModelsList;
    int status = 0;  //0表示还没有执行插件逻辑, 1表示授权用户身份,2表示无授权身份
    bool isVipListExist = false;
    bool isNonVipListExist = false;
    QPointer<QThread> workThread;
};

#endif // TRAYSET_H
