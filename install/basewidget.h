#pragma execution_character_set("utf-8")
#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QMainWindow>
#include <QSettings>
#include <QPaintEvent>
#include <QPainter>
#define NOMINMAX
#include <Windows.h>
#include <comdef.h>
#include <stdio.h>
#include <tchar.h>
#include <QProcess>
#include <QPointer>
#include <QLabel>

struct ICONDIRENTRY
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
};

struct ICONDIR
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
//    ICONDIRENTRY idEntries;
};

struct GRPICONDIRENTRY
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    WORD nID;
};
struct GRPICONDIR
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
    GRPICONDIRENTRY idEntries;
};

class BaseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BaseWidget(QWidget *parent = 0);

    virtual bool needMove(QMouseEvent *e);
    bool langType();
    QString lang();
    static QString stdAppPath();
    static QString stdDeskPath();
    static bool existProcess(QString proName);
    static void killProcess(const QString &proName);
    static bool loopProcess(QProcess *pro, int msecs = 30000);
    static void veryDel(const QString& dir);
    static QString readReg(QString path, QString keyName);
    static QString thirdInfo();
    static QString thirdGroup();
    static bool copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist);
    static QSettings *clientSet();
    void removeClientSet(bool uninstall = true);

    void removeLnk(QString target);

    int ChangeFileInfo(QString file, QString FileDescription, QString value);

    static void setDropShadow(QWidget *wid, int x, int y, int radius, QColor color);
    static QString globalStyle(bool lan = false);
    static void setClass(QWidget *wid, QString className);
    static void setProperty(QWidget *wid, QString name, QVariant value);
    void setLetterSpacing(QWidget *wid, qreal space);



    void setIngText(QPointer<QLabel> label, QString text);

    static QSettings *clientSetNewWrite();    //注册表重新写入

protected:
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    bool ChangeExeIcon(LPWSTR IconFile, LPWSTR ExeFile);

private:
    QPoint m_movePoint;


public:
    static QString m_exe;
    QString m_name;
    QString m_nameEN;
    static QString m_third;
    QString m_mark;
    QString m_lang;

    QString m_appPath;
    QString m_instPath;

    static QSettings* m_uninstSet;
    static QSettings* m_autoRunSet;
    static QSettings *m_xrootSet;
    static QSettings *m_xszSet;
    static QSettings* m_clientSet;

    static bool m_cn;
};

#endif // BASEWIDGET_H
