#ifndef BUTTONGROUP_H
#define BUTTONGROUP_H

#include <QButtonGroup>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>

class ButtonGroup : public QButtonGroup
{
    Q_OBJECT

public:
    explicit ButtonGroup(QObject* parent = 0);

    void addButtons(QList<QPushButton*> buttonL, QStyle* style, QStackedWidget* stack = NULL);
    int clickedId();

signals:
    void btnIdClick(int id);

private:
    int m_clickedId;
};

#endif // BUTTONGROUP_H
