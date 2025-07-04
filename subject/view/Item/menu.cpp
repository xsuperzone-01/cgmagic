#include "menu.h"
#include <QPainter>

Menu::Menu(QWidget *parent):
    QMenu(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
}
