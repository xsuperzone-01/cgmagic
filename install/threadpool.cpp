#include "threadpool.h"

#include <QDebug>
#define qD qDebug()<<

ThreadPool* ThreadPool::m_d = NULL;

ThreadPool::ThreadPool(QObject *parent) :
    QObject(parent)
{
}

ThreadPool *ThreadPool::inst()
{
    if (m_d == NULL) {
        m_d = new ThreadPool;
    }
    return m_d;
}

void ThreadPool::initThreadPool(QObject *object)
{
    QMutexLocker locker(&m_mutex);

    Thread* th = NULL;
    foreach (Thread* tmp, m_threadL) {
        if (!tmp->object()) {
            th = tmp;
            break;
        }
    }

    if (!th) {
        th = new Thread;
        m_threadL << th;
    }

    th->initThread(object);
}

QPointer<QObject> ThreadPool::threadObject(QThread *thread)
{
    QPointer<QObject> object;
    foreach (Thread* tmp, m_threadL) {
        if (tmp == (Thread*)thread) {
            object = tmp->object();
            break;
        }
    }
    return object;
}


Thread::Thread(QObject *parent) :
    QThread(parent),
    m_object(NULL)
{

}

void Thread::initThread(QObject *object)
{
    m_object = object;
    m_object->moveToThread(this);
    this->start();
}

QPointer<QObject> Thread::object()
{
    return m_object;
}
