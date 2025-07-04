#ifndef WINUTIL_H
#define WINUTIL_H

#include <QString>

class WinUtil
{
public:
    enum FileInfoLang{
        zh_cn,
        en_us
    };

    WinUtil();

    static int createFixedSizeFile(QString name, qint64 size);
};

#endif // WINUTIL_H
