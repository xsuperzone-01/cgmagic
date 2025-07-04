#include "loading.h"
#include "ui_loading.h"

#include <QFile>

Loading::Loading(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Loading),
    m_idx(0),
    m_timerId(0)
{
    ui->setupUi(this);
}

Loading::~Loading()
{
    delete ui;
}

void Loading::initLoading(QString png)
{
    if (png.isEmpty())
        png = ":/status/load/1 (%1).png";

    for (int j = 1; j < 80; j++) {
        QString file = png.arg(j);
        if (QFile::exists(file)) {
            m_pngL << file;
        } else {
            break;
        }
    }

    m_timerId = startTimer(40);
}

void Loading::stopTimer()
{
    killTimer(m_timerId);
}

//TODO CPU占用较高
void Loading::timerEvent(QTimerEvent *event)
{
    if (m_idx < m_pngL.length()) {
        ui->frame->setStyleSheet((QString("QLabel {border-image: url(%1);}").arg(m_pngL.at(m_idx))));
    }

    m_idx++;
    if (m_idx >= m_pngL.length()) {
        m_idx = 0;
    }
}
