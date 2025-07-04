#ifndef BASELINEEDIT_H
#define BASELINEEDIT_H

#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "common/BaseScale.h"

#define lineEditUser "lineEditUser"
#define lineEditPwd "lineEditPwd"

#define lineEditlabel "lineEditlabel"

class BaseLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit BaseLineEdit(QWidget *parent = nullptr);

    DECLARE_RESIZE();

    static QString replaceChinesePunctuation(QString text);
    void setReplaceChinesePunctuation(bool on);

    void setRightIcon(QString qssClass, QString user, QSize size = QSize(20, 20), int marginRight = 20);
    void setPwdButton(QSize size = QSize(20, 20), int marginRight = 20);

    void setRightButton(QPushButton *btn, int marginRight = 16);

    void setToplabel(QString qssClass, int id, bool have);

    void setPassword();

    void setFocusOutNoText(bool noText);

    void setinit(bool Text);

    void setTextError(bool textError);

protected:
    void keyPressEvent(QKeyEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    QHBoxLayout *layout();

signals:
    void focusOutNoText(bool noText);
    void focusOut();
    void deleteTop();
    void createTop();

    void PasswordFocusIn();
    void PasswordFocusOut();


private slots:
    void setChinesePunc();

private:
    bool m_paste;
    QPointer<QLabel> m_left;
    QPointer<QLabel> m_top;
};

#endif // BASELINEEDIT_H
