#include "webview.h"

#include <QWebChannel>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QDir>
#include <QTcpServer>
#include <QDialog>
#include <QGridLayout>

int debugPort = 0;

int randomPort()
{
    int port = 0;
    QTcpServer ts;
    bool ok = ts.listen(QHostAddress::LocalHost);
    if (ok) {
        port = ts.serverPort();
        ts.close();
    } else {
        qDebug()<< __FUNCTION__ << ts.serverError() << ts.errorString();
    }
    return port;
}

WebView::WebView(QWidget *parent):
    QWebEngineView(parent),
    m_loadOk(false)
{
    // 很多属性默认是enabled的
    QWebEngineSettings *set = page()->settings();
    set->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    set->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    set->setAttribute(QWebEngineSettings::SpatialNavigationEnabled, true);
    set->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, true);
    set->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    set->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    set->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    set->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
    set->setAttribute(QWebEngineSettings::TouchIconsEnabled, true);
    set->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
    set->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
    set->setAttribute(QWebEngineSettings::AllowGeolocationOnInsecureOrigins, true);
    set->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, true);
    set->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, true);//需要用户手势来播放媒体
    set->setAttribute(QWebEngineSettings::JavascriptCanPaste, true);
    set->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    set->setAttribute(QWebEngineSettings::ShowScrollBars, true);
    set->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    set->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    set->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);

    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject("XrenderLoginHelper", &m_webtool);
    page()->setWebChannel(channel);

    // 背景透明
    page()->setBackgroundColor(Qt::transparent);

    connect(this, &WebView::loadFinished, this, [=](bool ok){
        qDebug()<< "WebView load" << ok << this->url().toString();
        m_loadOk = ok;
    });

    connect(this, &QWebEngineView::renderProcessTerminated, this, &WebView::onRenderProcessTerminated);

    setAcceptDrops(true);
}

WebView::~WebView()
{
    QWebEngineProfile *prof = page()->profile();
    prof->clearHttpCache();
}

void WebView::onRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus status){
    switch (status) {
    case QWebEnginePage::NormalTerminationStatus:
        qDebug() << "渲染进程正常终止";
        break;
    case QWebEnginePage::AbnormalTerminationStatus:
        qDebug() << "渲染进程异常终止";
        break;
    case QWebEnginePage::CrashedTerminationStatus:
        qDebug() << "渲染进程崩溃";
        break;
    case QWebEnginePage::KilledTerminationStatus:
        qDebug() << "渲染进程被终止";
        reload();
        break;
    default:
        qDebug() << "未知渲染进程终止状态";
        break;
    }
}

void WebView::reloadIfError()
{
    if (!m_loadOk)
        reload();
}

void WebView::setCacheDir(QString dir)
{
    QDir().mkpath(dir);
    QWebEngineProfile *prof = page()->profile();
    prof->setCachePath(dir);
    prof->setPersistentStoragePath(dir);
}

bool WebView::event(QEvent *e)
{
    if (e->type() == QEvent::ShortcutOverride) {
        if (QKeyEvent *event = static_cast<QKeyEvent *>(e)) {
            if (event->key() == Qt::Key_F12) {
                QWebEngineView *dev = new QWebEngineView;
                dev->setAttribute(Qt::WA_DeleteOnClose);
                dev->resize(800, 600);
                dev->show();
                page()->setDevToolsPage(dev->page());
            }
        }
    }

    return QWebEngineView::event(e);
}


