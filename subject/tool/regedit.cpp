#include "regedit.h"

#include <QSettings>

#define HKEY_CURRENT_USER "HKEY_CURRENT_USER\\"
#define HKEY_LOCAL_MACHINE "HKEY_LOCAL_MACHINE\\"
#define HKEY_USERS "HKEY_USERS\\"

RegEdit::RegEdit(QObject *parent) : QObject(parent)
{

}

QVariant RegEdit::CU(QString path, QString key)
{
    return get(HKEY_CURRENT_USER + path, key);
}

void RegEdit::setCU(QString path, QString key, QVariant value)
{
    set(HKEY_CURRENT_USER + path, key, value);
}

QVariant RegEdit::US(QString path, QString key)
{
    return get(HKEY_USERS + path, key);
}

void RegEdit::setUS(QString path, QString key, QVariant value)
{
    set(HKEY_USERS + path, key, value);
}

QVariant RegEdit::get(QString path, QString key)
{
    QSettings set(path, QSettings::NativeFormat);
    return set.value(key);
}

void RegEdit::set(QString path, QString key, QVariant value)
{
    QSettings set(path, QSettings::NativeFormat);
    set.setValue(key, value);
}
