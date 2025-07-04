#ifndef SINGLETON_H
#define SINGLETON_H

#include <QMutex>
#include <QMutexLocker>
#include <QObject>

/// 单件模式申明宏，放到类定义中
#undef PATTERN_SINGLETON_DECLARE
#define PATTERN_SINGLETON_DECLARE(classname)	\
public:											\
static classname * instance();					\
explicit classname(QObject *parent = 0);        \
~classname();                                     \



/// 单件模式实现宏，放到类实现处
#undef PATTERN_SINGLETON_IMPLEMENT
#define PATTERN_SINGLETON_IMPLEMENT(classname)	\
    static QMutex sm_mutex##classname;			\
classname * classname::instance()		        \
{												\
    static classname * _instance = NULL;		\
    if( NULL == _instance)						\
    {											\
        QMutexLocker lock(&sm_mutex##classname);		\
        if (NULL == _instance)					\
        {										\
            _instance = new classname;			\
        }										\
    }											\
    return _instance;							\
}

#endif // SINGLETON_H
