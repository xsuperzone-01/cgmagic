#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QPointer>
#include <QWidgetAction>

#include "view/mainwindow.h"
#include "Singleton.h"
#include "view/set/trayset.h"

class QSystemTrayIcon;

class TrayIcon : public QObject
{
    Q_OBJECT

    PATTERN_SINGLETON_DECLARE(TrayIcon);
public:
    void setCurWid(QWidget* wid);
    Q_INVOKABLE void showInfoMsg(QString title, const QString& text, int msecs = 2000);
    void setIconOnline(const QString &tip);
    void setIconOffline(const QString &tip);
    void setEmptyIcon();
    MainWindow* mainWid();
    void showWindow();

    TraySet *traySet();

private slots:
    void showCurWid();
    void timeout();
    void applicationStateChanged(Qt::ApplicationState state);

private:
    QPointer<QSystemTrayIcon> m_sysTrayIcon;
    QPointer<QWidget> m_curWid;
    QPointer<QMenu> m_loginM;
    QPointer<QTimer> m_timer;
    QPointer<TraySet> m_traySet;
};

#endif // TRAYICON_H
