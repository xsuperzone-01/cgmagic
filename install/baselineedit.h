#ifndef BASELINEEDIT_H
#define BASELINEEDIT_H

#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QPointer>
#include <QPushButton>

#define lineEditUser "lineEditUser"
#define lineEditPwd "lineEditPwd"

class BaseLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit BaseLineEdit(QWidget *parent = nullptr);

    static QString replaceChinesePunctuation(QString text);
    void setReplaceChinesePunctuation(bool on);

    void setRightButton(QPushButton *btn, int marginRight = 16);
    void setPlaceholderTextColor(QColor color = QColor(255, 255, 255, 0.3));

protected:
    void keyPressEvent(QKeyEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    QHBoxLayout *layout();

signals:

private slots:
    void setChinesePunc();

private:
    bool m_paste;
    QPointer<QLabel> m_left;
    QString m_leftClass;
};

#endif // BASELINEEDIT_H
