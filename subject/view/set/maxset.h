#ifndef MAXSET_H
#define MAXSET_H

#include <QWidget>

namespace Ui {
class MaxSet;
}

class MaxSet : public QWidget
{
    Q_OBJECT

public:
    explicit MaxSet(QWidget *parent = nullptr);
    ~MaxSet();

    enum PushTo {
        Cache,
        Src
    };

    void initMaxSet();

    static void initMaxSetIni();
    static bool autoPush();
    static void setAutoPush(bool v);
    static bool autoOpenDir();
    static void setAutoOpenDir(bool v);
    static PushTo pushTo();
    static void setPushTo(PushTo v);
    static QString cacheDir();

signals:
    void updateSet();

private:
    Ui::MaxSet *ui;
};

#endif // MAXSET_H
