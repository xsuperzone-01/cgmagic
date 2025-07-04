#ifndef ENVSET_H
#define ENVSET_H

#include <QWidget>
#include <QButtonGroup>
#include <QModelIndex>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QMouseEvent>

namespace xy {
enum RenderSoft {
    rsMaya = 1,
    rsMax = 3,
    rsHoudini = 4,
    rsC4D = 5,
    rsSketchUp = 6,
    rsClarisse = 7,
    rsTerragen = 8
};
}
using namespace xy;

class EnvBlock;

namespace Ui {
class EnvSet;
}

class EnvSet : public QWidget
{
    Q_OBJECT

public:
    explicit EnvSet(QWidget *parent = 0);
    ~EnvSet();

    void initEnvSet(int project, int type = -1);
    void clearEnvSet();

    void addEnv();
    void updateEnv(int id);
    void delEnv(int id, QString software);
    void showEnv(QJsonObject obj);
    bool validateEnv();

    void setBtnStyle(QAbstractButton *btn);

    void updatePath();

    void getEnv();
    void defaultEnv(int id, QString software);

    void removeEnvBlock();

    static QJsonObject readProject();
private slots:
    void buttonClicked(QAbstractButton* btn);

    void on_cancel_clicked();
    void envSetHand(int type, QString json);
    void on_submit_clicked();
    void on_projectOpen_clicked();

    void on_resultOpen_clicked();

    void on_advance_clicked();
    void resultRule(QString rule);
    void on_backEnvset_clicked();

    void on_backEnvset_2_clicked();

signals:
    void refreshEnvPre(int projectId);
    void refreshResultDir(QString dir);
private:
    int saveCfg(QJsonObject obj);
    int delCfg(QString software);
    int defaultCfg(QString software);
    void SaveEn(bool state);
    void setProject(QJsonObject &project);

    Ui::EnvSet *ui;

    int m_type;
    QButtonGroup m_bg;

    QList<EnvBlock*> m_envBlockL;
    int m_projectId;
    QString m_rule;
    QMutex m_mutex;

protected:
    bool eventFilter(QObject *watched, QEvent *event);
};

#endif // ENVSET_H
