#pragma execution_character_set("utf-8")
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <QObject>
#include <QThread>
#include <QPointer>
#include <QMutexLocker>

#define THP ThreadPool::inst()

class Thread : public QThread
{
    Q_OBJECT
public:
    explicit Thread(QObject *parent = 0);

    void initThread(QObject* object);
    QPointer<QObject> object();
private:
    QPointer<QObject> m_object;
};

class ThreadPool : public QObject
{
    Q_OBJECT
public:
    static ThreadPool* inst();

    void initThreadPool(QObject* object);
    QPointer<QObject> threadObject(QThread *thread);
private:
    explicit ThreadPool(QObject *parent = 0);
    static ThreadPool* m_d;

    QList<Thread*> m_threadL;
    QMutex m_mutex;
};

#endif // THREADPOOL_H
