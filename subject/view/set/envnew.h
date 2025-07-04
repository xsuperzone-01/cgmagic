#ifndef ENVNEW_H
#define ENVNEW_H

#include <QWidget>
#include <QMap>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>

class DefRendererUi {
public:
    DefRendererUi() {

    }

//    DefRendererUi(QLabel* l, QPushButton* b, QWidget* w) {
//        lab = l;
//        btn = b;
//        wid = w;
//    }

//    QLabel* lab;
//    QPushButton* btn;
//    QWidget* wid;
    DefRendererUi(QLabel* l, QRadioButton* b, QWidget* w) {
        lab = l;
        btn = b;
        wid = w;
    }

    QLabel* lab;
    QRadioButton* btn;
    QWidget* wid;
};

namespace Ui {
class EnvNew;
}

class EnvNew : public QWidget
{
    Q_OBJECT

public:
    explicit EnvNew(QWidget *parent = 0);
    ~EnvNew();

    void initEnvNew(int soft);

    void setSoft(QJsonObject obj);
    void setDefaultSoft(QString soft);
    void setPluginR(QJsonObject obj);
    int envId();
    void enableSoft();

    void setPlugin(QJsonObject obj);

    QStringList selectName();
    QStringList selectText();

    QString software();
    QJsonArray plugin();

    void clearEnvId();

    QString GetDefRenderer();

    void clearBtnEn();

private:
    bool DefaultRendererSet();
    bool eventFilter(QObject *o, QEvent *e);

signals:
    void softReceive();
private slots:
    void softChanged(int idx);
    void pluginChanged(int idx);
    void on_toR_clicked();

    void on_toL_clicked();

    void on_del_clicked();

private:
    Ui::EnvNew *ui;

    int m_envId;
    QMap<QString, QStringList> m_plgMap;
    QMap<QString/*插件名称*/, QStringList/*插件版本*/> m_selPlgMap;//已选的插件
    QMap<QString, DefRendererUi> m_defRendMap;
};

#endif // ENVNEW_H
