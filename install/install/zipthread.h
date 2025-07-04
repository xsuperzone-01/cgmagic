#pragma execution_character_set("utf-8")
#ifndef ZIPTHREAD_H
#define ZIPTHREAD_H

#include <QThread>

class ZipThread : public QThread
{
    Q_OBJECT
public:
    explicit ZipThread(QObject *parent = 0);

    void setFileDir(QString file, QString dir);
signals:
    void oneOk();
protected:
    void run();
private:
    QString m_file;
    QString m_dir;
};

#endif // ZIPTHREAD_H
