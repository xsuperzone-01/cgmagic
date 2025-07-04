#ifndef BASELABEL_H
#define BASELABEL_H

#include <QEvent>
#include <QLabel>
#include "common/BaseScale.h"
#include <QMouseEvent>

class BaseLabel : public QLabel
{
    Q_OBJECT
public:
    explicit BaseLabel(QWidget *parent = nullptr);

    DECLARE_RESIZE();

    void bindTextInteraction();

public Q_SLOTS:
    void setText(const QString &text);

signals:
    void pressHover(bool);

    void clicked();

    void enter();
    void leave();

protected:
    bool eventFilter(QObject* obj, QEvent *event);

private:
    void setText2(const QString &text);

private:
    bool m_hover;
    bool m_mouse_press;
    bool textInteraction;
    bool textClick;
};

#endif // BASELABEL_H
