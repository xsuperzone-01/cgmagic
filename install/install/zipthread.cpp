#include "zipthread.h"

#include "quazip/JlCompress.h"
#include <QObject>

ZipThread::ZipThread(QObject *parent) :
    QThread(parent)
{
}

void ZipThread::setFileDir(QString file, QString dir)
{
    m_file = file;
    m_dir = dir;
}

void ZipThread::run()
{
    QObject obj;
    connect(&obj, SIGNAL(objectNameChanged(QString)), this, SIGNAL(oneOk()));
    JlCompress::XGTextractDir(m_file, m_dir, &obj);

    exit();

    exec();
}
