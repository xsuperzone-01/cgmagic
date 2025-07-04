#ifndef ENVBLOCK_H
#define ENVBLOCK_H

#include <QWidget>

namespace Ui {
class EnvBlock;
}

class EnvBlock : public QWidget
{
    Q_OBJECT

public:
    explicit EnvBlock(QWidget *parent = 0);
    ~EnvBlock();

    void initEnvBlock(bool add, QString soft = QString(), QStringList rdL = QStringList(), QStringList plgL = QStringList(), QString json = QString(), bool isDefault = false);
    void removeStack();
signals:
    void envBlockHand(int type, QString json);
private slots:
    void on_setDefault_clicked();

    void on_mod_clicked();

    void on_del_clicked();

    void on_addEnv_clicked();

    void on_addEnvIcon_clicked();

private:
    Ui::EnvBlock *ui;
    QString m_json;

protected:
    bool eventFilter(QObject *watched, QEvent *event);

};

#endif // ENVBLOCK_H
