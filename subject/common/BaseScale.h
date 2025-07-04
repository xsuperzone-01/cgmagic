#ifndef BASESCALE_H
#define BASESCALE_H
#include "config/userinfo.h"
#include <QLayout>
#include <QtMath>

static int doubleFontToInt(double d)
{
    double intPart = qFloor(d);
    if ((d - intPart) >= (double)0.01){
        return (intPart + 1);
    }
    else{
        return intPart;
    }
}

//函数声明
#define DECLARE_RESIZE()\
public:\
    void resize(int w, int h); \
    void resize(QSize size); \
    void setFixedHeight(int h); \
    void setOriginalFixedHeight(int h);\
    void setFixedWidth(int w); \
    void setOriginalFixedWidth(int w); \
    void setFixedSize(int w, int h); \
    void setOriginalFixedSize(int w, int h); \
    void setFixedSize(QSize size);\
    void setOriginalFixedSize(QSize size); \
    void setMinimumSize(QSize size);\
    void setMinimumSize(int minw, int minh);\
    void setMinimumHeight(int minh);\
    void setMinimumWidth(int minw);\
    void setMaximumSize(QSize size);\
    void setMaximumSize(int maxw, int maxh);\
    void setMaximumHeight(int maxh);\
    void setMaximumWidth(int maxw);\
    QSize sizeHint();\
    QSize minimumSizeHint();\
    void selfAdaptionSize();\
    void selfAdaptionFixedSize();\
    void selfAdaptionFixedWidth();\
    void selfAdaptionFixedHeight();\
    double m_scaleW = USERINFO->instance()->getScaleW();\
    double m_scaleH = USERINFO->instance()->getScaleH();\

#define DEFINE_RESIZE(classname, base)\
    void classname::resize(int w, int h){\
        if (m_scaleW > 0 && m_scaleW < 1 && m_scaleH > 0 && m_scaleH < 1) {\
            w = w * m_scaleW;\
            h = h * m_scaleH;\
        }\
        base::resize(w, h);\
    }\
    void classname::resize(QSize size){\
        if (m_scaleW > 0 && m_scaleW < 1 && m_scaleH > 0 && m_scaleH < 1) {\
            size.setWidth(size.width() * m_scaleW);\
            size.setHeight(size.height() * m_scaleH);\
        }\
        base::resize(size);\
    }\
    void classname::setFixedHeight(int h) {\
        if (m_scaleH > 0 && m_scaleH < 1) {\
            h = h * m_scaleH;\
        }\
        base::setFixedHeight(h);\
    }\
    void classname::setOriginalFixedHeight(int h){\
        base::setFixedHeight(h);\
    }\
    void classname::setFixedWidth(int w) {\
        if (m_scaleW > 0 && m_scaleW < 1) {\
            w = w * m_scaleW;\
        }\
        base::setFixedWidth(w);\
    }\
    void classname::setOriginalFixedWidth(int w){\
        base::setFixedWidth(w);\
    } \
    void classname::setFixedSize(int w, int h){\
        if (m_scaleW > 0 && m_scaleW < 1 && m_scaleH > 0 && m_scaleH < 1) {\
            w = w * m_scaleW;\
            h = h * m_scaleH;\
        }\
        base::setFixedSize(w, h);\
    }\
    void classname::setOriginalFixedSize(int w, int h){\
        base::setFixedSize(w, h);\
    } \
    void classname::setFixedSize(QSize size){\
        if (m_scaleW > 0 && m_scaleW < 1 && m_scaleH > 0 && m_scaleH < 1) {\
            size.setWidth(size.width() * m_scaleW);\
            size.setHeight(size.height() * m_scaleH);\
        }\
        base::setFixedSize(size);\
    }\
    void classname::setOriginalFixedSize(QSize size){\
        base::setFixedSize(size);\
    } \
    void classname::setMinimumSize(QSize size){\
        if (m_scaleW > 0 && m_scaleW < 1 && m_scaleH > 0 && m_scaleH < 1) {\
            size.setWidth(size.width() * m_scaleW);\
            size.setHeight(size.height() * m_scaleH);\
        }\
        base::setMinimumSize(size);\
    }\
    void classname::setMinimumSize(int minw, int minh){\
        if (m_scaleW > 0 && m_scaleW < 1 && m_scaleH > 0 && m_scaleH < 1) {\
            minw = minw * m_scaleW;\
            minh = minh * m_scaleH;\
        }\
        base::setMinimumSize(minw, minh);\
    }\
    void classname::setMinimumHeight(int minh){\
        if (m_scaleH > 0 && m_scaleH < 1) {\
            minh = minh * m_scaleH;\
        }\
        base::setMinimumHeight(minh);\
    }\
    void classname::setMinimumWidth(int minw){\
        if (m_scaleW > 0 && m_scaleW < 1 && m_scaleH > 0 && m_scaleH < 1) {\
            minw = minw * m_scaleW;\
        }\
        base::setMinimumWidth(minw);\
    }\
    void classname::setMaximumSize(QSize size){\
        if (m_scaleW > 0 && m_scaleW < 1 && m_scaleH > 0 && m_scaleH < 1) {\
            size.setWidth(size.width() * m_scaleW);\
            size.setHeight(size.height() * m_scaleH);\
        }\
        base::setMaximumSize(size);\
    }\
    void classname::setMaximumSize(int maxw, int maxh){\
        if (m_scaleW > 0 && m_scaleW < 1 && m_scaleH > 0 && m_scaleH < 1) {\
            maxw = maxw * m_scaleW;\
            maxh = maxh * m_scaleH;\
        }\
        base::setMaximumSize(maxw, maxh);\
    }\
    void classname::setMaximumHeight(int maxh){\
        if (m_scaleH > 0 && m_scaleH < 1) {\
            maxh = maxh * m_scaleH;\
        }\
        base::setMaximumHeight(maxh);\
    }\
    void classname::setMaximumWidth(int maxw){\
        if (m_scaleW > 0 && m_scaleW < 1) {\
            maxw = maxw * m_scaleW;\
        }\
        base::setMaximumWidth(maxw);\
    }\
    QSize classname::sizeHint(){\
        QSize size = base::sizeHint();\
        if (m_scaleW > 0 && m_scaleW < 1) {\
            size.setWidth((float)size.width() / m_scaleW);\
        }\
        if (m_scaleH > 0 && m_scaleH < 1) {\
            size.setHeight((float)size.height() / m_scaleH);\
        }\
        return size;\
    }\
    QSize classname::minimumSizeHint(){\
        QSize size = base::minimumSizeHint();\
        if (m_scaleW > 0 && m_scaleW < 1) {\
            size.setWidth((float)size.width() / m_scaleW);\
        }\
        if (m_scaleH > 0 && m_scaleH < 1) {\
            size.setHeight((float)size.height() / m_scaleH);\
        }\
        return size;\
    }\
    void classname::selfAdaptionSize(){\
        return;\
        resize(width(), height());\
    }\
    void classname::selfAdaptionFixedSize(){\
        return;\
        setFixedSize(width(), height());\
    }\
    void classname::selfAdaptionFixedWidth(){\
        return;\
        setFixedWidth(width());\
    }\
    void classname::selfAdaptionFixedHeight(){\
        return;\
        setFixedHeight(height());\
    }\

#define MARGINS_RESIZE()\
public:\
    void selfAdaptionMargins();\

#define MARGINS_RESIZE_NAME(classname)\
    void classname::selfAdaptionMargins(){\
        return;\
        if (layout() == NULL) {\
            return;\
        }\
        QMargins marg = layout()->contentsMargins();\
        if (m_scaleW > 0 && m_scaleW < 1 && m_scaleH > 0 && m_scaleH < 1) {\
            marg.setTop(doubleFontToInt(marg.top() * m_scaleH));\
            marg.setBottom(doubleFontToInt(marg.bottom() * m_scaleH));\
            marg.setLeft(doubleFontToInt(marg.left() * m_scaleW));\
            marg.setRight(doubleFontToInt(marg.right() * m_scaleW));\
            if (QVBoxLayout* ly = qobject_cast<QVBoxLayout*>(layout())) {\
                int spacing = doubleFontToInt(ly->spacing() * m_scaleH);\
                ly->setSpacing(spacing);\
            }\
            if (QHBoxLayout* ly = qobject_cast<QHBoxLayout*>(layout())) {\
                int spacing = doubleFontToInt(ly->spacing() * m_scaleW);\
                ly->setSpacing(spacing);\
            }\
            if (QGridLayout* ly = qobject_cast<QGridLayout*>(layout())) {\
                int vspacing = doubleFontToInt(ly->verticalSpacing() * m_scaleH);\
                int hspacing = doubleFontToInt(ly->horizontalSpacing() * m_scaleW);\
                ly->setVerticalSpacing(vspacing);\
                ly->setHorizontalSpacing(hspacing);\
            }\
        }\
        layout()->setContentsMargins(marg);\
    }\

#endif // BASESCALE_H
