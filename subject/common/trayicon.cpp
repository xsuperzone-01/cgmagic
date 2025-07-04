#include "trayicon.h"

#include "config/userinfo.h"
#include "tool/xfunc.h"
#include "common/session.h"
#include "view/login.h"
#include "view/set/trayset.h"

PATTERN_SINGLETON_IMPLEMENT(TrayIcon);

TrayIcon::TrayIcon(QObject *parent) :
    QObject(parent),
    m_sysTrayIcon(NULL),
    m_curWid(NULL),
    m_loginM(NULL),
    m_timer(NULL)
{
    m_sysTrayIcon = new QSystemTrayIcon(this);
    connect(m_sysTrayIcon, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason reason){
        switch (reason){
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            if (m_curWid) {
                    showCurWid();
            }
            break;
        case QSystemTrayIcon::Context:
            break;
        default:
            break;
        }
        m_timer->stop();
        setIconOnline("");
    });

    m_traySet = new TraySet(this);
    m_loginM = m_traySet->newTrayMenu();

    m_sysTrayIcon->setIcon(QIcon(QString(":/third/%1/offline.ico").arg(USERINFO->thirdGroup())));
    m_sysTrayIcon->setToolTip(USERINFO->thirdTrayName());

    m_timer = new QTimer(this);
    m_timer->setInterval(500);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout()));

    connect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(applicationStateChanged(Qt::ApplicationState)));
}

MainWindow *TrayIcon::mainWid()
{
    QString on = m_curWid->objectName();
    if (on.contains("MainWindow")) {
        MainWindow* mw = (MainWindow*)m_curWid.data();
        return mw;
    }
    return NULL;
}

void TrayIcon::showWindow()
{
    if (m_curWid) {
#ifdef Q_OS_MAC
   MacUtil::showWindow(m_curWid->window());
#endif
    }
}

TraySet *TrayIcon::traySet()
{
    return m_traySet;
}

TrayIcon::~TrayIcon()
{
    qDebug() << __FUNCTION__;
    disconnect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(applicationStateChanged(Qt::ApplicationState)));

    if (m_loginM)
        delete m_loginM;

    if (m_timer) {
        if (m_timer->isActive())
            m_timer->stop();

        delete m_timer;
    }

    if (m_sysTrayIcon) {
        disconnect(m_sysTrayIcon, 0, 0, 0);
        delete m_sysTrayIcon;
    }
}

void TrayIcon::setCurWid(QWidget *wid)
{
    m_sysTrayIcon->show();
    m_sysTrayIcon->setContextMenu(NULL);
    m_curWid = wid;
        m_sysTrayIcon->setContextMenu(m_loginM);
}

void TrayIcon::showInfoMsg(QString title, const QString &text, int msecs)
{
    if (title.isEmpty())
        title = tr("提示");
    m_sysTrayIcon->showMessage(title, text, QSystemTrayIcon::Information, msecs);
}

void TrayIcon::setIconOnline(const QString& tip)
{
    m_sysTrayIcon->setIcon(QIcon(QString(":/third/%1/online.ico").arg(USERINFO->instance()->thirdGroup())));
}

void TrayIcon::setIconOffline(const QString &tip)
{
    m_sysTrayIcon->setIcon(QIcon(QString(":/third/%1/offline.ico").arg(USERINFO->instance()->thirdGroup())));
}

void TrayIcon::setEmptyIcon()
{
    m_timer->start();
}

void TrayIcon::showCurWid()
{
    if (m_curWid) {
        m_curWid->activateWindow();
        m_curWid->showNormal();
    }
}

void TrayIcon::timeout()
{
    if (!m_sysTrayIcon->icon().isNull())
        m_sysTrayIcon->setIcon(QIcon());
    else
        setIconOnline("");
}

void TrayIcon::applicationStateChanged(Qt::ApplicationState state)
{
    if (m_timer->isActive()) {
        setIconOnline("");
        m_timer->stop();
    }
}
