#ifndef REGEDIT_H
#define REGEDIT_H

#include <QObject>

class RegEdit : public QObject
{
    Q_OBJECT
public:
    explicit RegEdit(QObject *parent = nullptr);

    static QVariant CU(QString path, QString key);
    static void setCU(QString path, QString key, QVariant value);
    static QVariant US(QString path, QString key);
    static void setUS(QString path, QString key, QVariant value);
    static QVariant get(QString path, QString key);
    static void set(QString path, QString key, QVariant value);

signals:

};

#endif // REGEDIT_H
