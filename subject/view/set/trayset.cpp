#include "trayset.h"

#include <QFileDialog>
#include <QLibrary>
#include "view/Item/menu.h"
#include "view/leftnavitem.h"
#include "common/basewidget.h"
#include "common/widgetgroup.h"
#include "common/session.h"
#include "set.h"
#include "traysetaccount.h"
#include "common/trayicon.h"
#include "tool/childprocess.h"
#include "tool/network.h"
#include "common/protocol.h"
#include <QTimer>
#include <QtConcurrent>
#include "tool/webtool.h"

class MenuItem {
public:
    typedef std::function<void ()> FuncBack;
    MenuItem(QString icon, QString text, FuncBack func, bool loginUse = true, bool subMenu = false) {
        this->icon = icon;
        this->text = text;
        this->func = func;
        this->subMenu = subMenu;
        this->loginUse = loginUse;
        this->seprate = false;
    }

    LeftNavItem *createAction(QMenu *menu) {
        wa = new QWidgetAction(menu);
        LeftNavItem *left = new LeftNavItem(menu);
        QObject::connect(left, &LeftNavItem::clicked, wa, &QWidgetAction::triggered);
        left->hideSelect();
        left->setLeftMargin(16);
        left->setIconType(icon);
        left->initLeftNavItem(text);

        QWidget *back = new QWidget(menu);
        QGridLayout *lay = new QGridLayout(back);
        QMargins m(16, 0, 16, 0);
        if (seprate) {
            BaseWidget::setClass(back, "trayMenuSeparator");
        }
        lay->setContentsMargins(m);
        lay->addWidget(left);
        back->setLayout(lay);
        wa->setDefaultWidget(back);
        item = left;
        menu->addAction(wa);
        return left;
    }

    QString text;
    QString icon;
    bool subMenu;
    bool loginUse;
    bool seprate;
    FuncBack func;
    QPointer<QWidgetAction> wa;
    QPointer<LeftNavItem> item;
};

TraySet::TraySet(QObject *parent) :
    QObject(parent)
{

    workThread = new QThread;
    this->moveToThread(workThread);
    connect(workThread, &QThread::started, this, &TraySet::reviseMaxLanguage);
    connect(workThread, &QThread::finished, workThread, &QThread::deleteLater);
}

TraySet::~TraySet()
{

}

void TraySet::reviseMaxLanguage(){
    if (Set::changeLan() == "en_us") {
        mbUtil("MbUtil_ChangeENLanguage");
    } else {
        mbUtil("MbUtil_ChangeCNLanguage");
    }
}

QMenu *TraySet::newTrayMenu()
{
    Menu *menu = new Menu;
    menu->setFixedWidth(288);
    BaseWidget::setClass(menu, "trayMenu");

    TraySetAccount *acWid = new TraySetAccount(menu);
    QWidgetAction *acWa = new QWidgetAction(menu);
    acWa->setDefaultWidget(acWid);
    menu->addAction(acWa);
    acWa->setVisible(false);

    connect(menu, &QMenu::aboutToShow, this, [=]{
        if (m_autoRunItem) {
            bool ar = USERINFO->isAutoRun();
            m_autoRunItem->showDown(ar);
            BaseWidget::setProperty(m_autoRunItem->downBtn(), "selected", ar);
        }

        bool login = !USERINFO->validUserId().isEmpty();
        foreach (MenuItem item, m_miL) {
            if (item.wa) {
                item.wa->setEnabled(!item.loginUse || login);
                if (item.item) {
                    item.item->setSelected(false);
                    item.item->setEnabled(item.wa->isEnabled());
                    qDebug()<<item.wa->isEnabled();
                    if (!item.wa->isEnabled()) {
                        item.item->hide();
                    } else {
                        item.item->show();
                    }
                }
            }
        }
        qDebug()<<"是否已经登录"<<login<<m_account;
        if (login && m_account) {
            acWa->setVisible(true);
            m_account->initTraySetAccount();//用户名+余额部分只有登录才出现
        }
    });

    //账号信息
    m_account = acWid;

    QList<BaseClickWidget *> navClickL;
    {
        MenuItem m("maxLow", tr("模型降版本"), [=]{
            int ret = MsgTool::msgChooseLoop(tr("此功能是暴力降版本，将一次性将版本号降到2013，是否继续？"));
            if (MsgBox::msgNo == ret)
                return;
            // 不延迟的话QFileDialog显示后就被关闭
            QTimer::singleShot(100, this, [=]{
                QStringList files = QFileDialog::getOpenFileNames(NULL, "", "", "(*.max)");
                if (files.isEmpty())
                    return;
                // 过滤2013及以下版本
                QStringList toL, noL;
                foreach (QString file, files) {
                    QString v = mbUtil("MbUtil_QueryMaxVersion", QStringList()<< QString("-v=\"%1\"").arg(file));
                    if (v != "" && v.toInt() <= 2013) {
                        noL << file;
                    }
                }
                qDebug()<< "无需降版本" << noL;
                if (!noL.isEmpty()) {
                    MsgTool::msgOkLoop(tr("以下版本不高于2013版，无需处理：\r\n%1").arg(noL.join("\r\n")));
                }
                foreach (QString file, files) {
                    if (!noL.contains(file))
                        toL << file;
                }
                if (toL.isEmpty())
                    return;
                qDebug()<< "准备降版本" << toL;
                mbUtil("MbUtil_BatchDowngradeMaxFiles", QStringList()<< QString("-v=\"%1\"").arg(toL.join(";")));
            });

        });
        m_miL << m;
    }
    {
        MenuItem m("pngLow", tr("材质库降版本"), [=]{
            QStringList files = QFileDialog::getOpenFileNames(NULL, "", "", "(*.mat)");
            if (files.isEmpty())
                return;
            mbUtil("MbUtil_BatchDowngradeMatFiles", QStringList()<< QString("-v=\"%1\"").arg(files.join(";")));
        });
        m.seprate = true;
        m_miL << m;
    }
    {
        MenuItem m("openMax", tr("唤醒MAX窗口"), [=]{
            mbUtil("MbUtil_WakeupMax");
        });
        m_miL << m;
    }
    {
        MenuItem m("frame", tr("显示帧缓存"), [=]{

        });
        m.seprate = true;
        m_miL << m;
    }
    {
        MenuItem m("autoRun", tr("开机自动启动"), [=]{
            bool ar;
            ar = !USERINFO->isAutoRun();
            Set::setAutoRun(ar);
            qDebug()<<m_autoRunItem->downBtn()->property("selected");
            QTimer::singleShot(100, this, [=](){
                BaseWidget::setProperty(m_autoRunItem->downBtn(), "selected", ar);
            });
            menu->close();
        }, false);
        m_miL << m;
    }
    {
        MenuItem m("relogin", tr("切换账号"), [=]{
            showMainWindow();
            MsgBox *mb = MsgTool::msgChoose(tr("是否退出当前账号？"), Session::instance()->mainWid());
            mb->setBackgroundMask();
            connect(mb, &MsgBox::accepted, this, []{
                Session::instance()->reLogin(Login::UserReLogin);
            });
        });
        m_miL << m;
    }
    {
        MenuItem m("quit", tr("退出"), [=]{
            if (TrayIcon::instance()->mainWid() == NULL) {
                Session::instance()->proExit();
            } else {
                showMainWindow();
                MsgBox *mb = MsgTool::msgChoose(tr("是否退出 CG MAGIC？"), Session::instance()->mainWid());
                mb->setBackgroundMask();
                connect(mb, &MsgBox::accepted, this, []{
                    Session::instance()->proExit();
                });
            }
        }, false);
        m_miL << m;
    }

    Menu *frameMenu = newFrameMenu(menu);
    frameMenu->setFixedWidth(288);
    int len = m_miL.length();
    for (int i = 0; i < len; i++) {
        MenuItem item = m_miL.takeFirst();
        LeftNavItem *left = item.createAction(menu);
        connect(item.wa, &QWidgetAction::triggered, this, [=]{
            if (item.func)
                item.func();
        }, Qt::QueuedConnection);
        navClickL << left;
        m_miL << item;

        QString icon = item.icon;
        if (icon == "frame") {
            left->showDown();
            item.wa->setMenu(frameMenu);
            //触发二级菜单的显示
            connect(left, &LeftNavItem::hovered, this, [=]{
                QMouseEvent *event = new QMouseEvent(QEvent::MouseMove, QPointF(QPoint(1, 1)), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::postEvent(left, event);
                QMouseEvent *event2 = new QMouseEvent(QEvent::MouseMove, QPointF(QPoint(1, 1)), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::postEvent(left, event2);
            }, Qt::QueuedConnection);
            connect(frameMenu, &Menu::aboutToHide, left, &LeftNavItem::leave, Qt::QueuedConnection);
        } else if (icon == "autoRun")
            m_autoRunItem = left;
    }

    widgetGroup *acg = new widgetGroup(this);
    acg->addWidgets(navClickL, qApp->style(), NULL);
    return menu;
}

void TraySet::showMainWindow()
{
    if (MainWindow *mw = TrayIcon::instance()->mainWid()) {
        if (mw->isMinimized())
            mw->showNormal();
        mw->show();
    }
}

QString TraySet::mbUtil(QString cmd, QStringList args)
{
    QString exe = qApp->applicationDirPath() + "/mobao/mbUtil.exe";
    QProcess pro;
    pro.setWorkingDirectory(QFileInfo(exe).absolutePath());
    args.prepend(QString("-c=%1").arg(cmd));
    args.prepend(QString("-d=%1").arg("MbExt.dll"));
    pro.start(exe, QStringList()<< args);
    ChildProcess::loopProcess(&pro);

    QByteArray b = pro.readAllStandardOutput();

    QStringList bL = QString(b).split("\n");
    QString tag = "mbUtilReturn";
    foreach (QString s, bL) {
        if (s.contains(tag)) {
            QString m = s.mid(s.indexOf(tag) + tag.length() + 1);
            return m;
        }
    }
    return "";
}

Menu *TraySet::newFrameMenu(Menu *parent)
{
    Menu *menu = new Menu(parent);
    menu->setFixedWidth(288);
    BaseWidget::setClass(menu, "trayMenu");
    QList<MenuItem> miL;
    {
        MenuItem m("frameMax", tr("MAX"), [=]{
            mbUtil("MbUtil_ShowMaxFrameBuffer");
        });
        miL << m;
    }
    {
        MenuItem m("frameVray", tr("V-Ray"), [=]{
            mbUtil("MbUtil_ShowVRayFrameBuffer");
        });
        miL << m;
    }
    {
        MenuItem m("frameCorona", tr("Corona"), [=]{
            mbUtil("MbUtil_ShowCoronaFrameBuffer");
        });
        miL << m;
    }

    QList<BaseClickWidget *> navClickL;
    int len = miL.length();
    for (int i = 0; i < len; i++) {
        MenuItem item = miL.takeFirst();
        LeftNavItem *left = item.createAction(menu);
        connect(item.wa, &QWidgetAction::triggered, this, [=]{
            menu->hide();
            parent->hide();
            if (item.func)
                item.func();
        }, Qt::QueuedConnection);
        navClickL << left;
    }
    widgetGroup *acg = new widgetGroup(this);
    acg->addWidgets(navClickL, qApp->style(), NULL);

    return menu;
}

void TraySet::showNetworkErrorTip(QString errorMessage){
    //做窗口唯一性判断，防止一瞬间出现多个错误弹窗
    //TODO：未来可以迁移到WebTool类中进行封装
    if (!networkErrorWid) {
        qDebug()<<"show error widget";
        networkErrorWid = WebTool().showMessageError(errorMessage);
    }
}
