#ifndef RENDERSET_H
#define RENDERSET_H

#include <QWidget>
#include <QLabel>

namespace Ui {
class RenderSet;
}

class RenderSet : public QWidget
{
    Q_OBJECT

public:
    explicit RenderSet(QWidget *parent = nullptr);
    ~RenderSet();

    enum PushTo {
        Cache,
        Src
    };

    enum SameFile {
        Confirm,
        Cover,
        Rename,
        Ignore
    };

    void initRenderSet();

    static void initRenderSetIni();
    static bool autoPush();
    static void setAutoPush(bool v);
    static bool autoOpenDir();
    static void setAutoOpenDir(bool v);
    static PushTo pushTo();
    static void setPushTo(PushTo v);
    static QString cacheDir();
    static SameFile sameFile();
    static void setSameFile(SameFile v);
    static bool psd();
    static void setPsd(bool v);
    static bool channel();
    static void setChannel(bool v);
    static bool prefix();
    static void setPrefix(bool v);

private slots:
    void setWarn();

    void on_warnTime_valueChanged(const QString &arg1);

private:
    Ui::RenderSet *ui;

    static bool m_autoPush;
    static bool m_autoOpenDir;
    static SameFile m_sameFile;
    static bool m_psd;
    static bool m_channel;
    static bool m_prefix;

};

#endif // RENDERSET_H
