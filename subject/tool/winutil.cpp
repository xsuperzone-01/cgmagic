#include "winutil.h"

#include <QStringList>
#include <QDebug>
#include <stdio.h>
#define qD qDebug()<<

#ifdef Q_OS_WIN
#include <Windows.h>
#include <tchar.h>
#include <WinVer.h>
#include <aclapi.h>
#include <dbghelp.h>
#include <wbemidl.h>
#include <comdef.h>
#endif
WinUtil::WinUtil()
{
}

#ifdef Q_OS_WIN
struct {
    WORD wLanguage;
    WORD wCodePage;
} *lpTranslate;
#endif

int WinUtil::createFixedSizeFile(QString name, qint64 size)
{
    int ret = 0;
#ifdef Q_OS_WIN
    HANDLE hFile = CreateFile(name.toStdWString().c_str(),GENERIC_READ | GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        qD "CreateFile err:" << GetLastError();
        return -1;
    }
    LARGE_INTEGER liDistanceToMove;
    liDistanceToMove.QuadPart = size;
    if (!SetFilePointerEx(hFile, liDistanceToMove, NULL, FILE_BEGIN)) {
        qD "SetFilePointerEx err:" << GetLastError();
        ret = -2;
    }
    if (!SetEndOfFile(hFile)) {
        qD "SetEndOfFile err:" << GetLastError();
        ret = -2;
    }
    CloseHandle(hFile);
    if (ret == -2)
        DeleteFile(name.toStdWString().c_str());
#endif
    return ret;
}
