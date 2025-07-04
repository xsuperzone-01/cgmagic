#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebEngineView>
#include "webtool.h"

class WebView : public QWebEngineView
{
    Q_OBJECT
public:
    explicit WebView(QWidget *parent = 0);
    ~WebView();

    void reloadIfError();

    void setCacheDir(QString dir);

protected:
    bool event(QEvent *e);

private slots:
    void onRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus status);

private:
    WebTool m_webtool;
    bool m_loadOk;
};

#endif // WEBVIEW_H
