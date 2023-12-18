/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2010-2011 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_DEBUG_H
#define SV_DEBUG_H

#include <QDebug>
#include <QTextStream>
#include <QElapsedTimer>
#include <QThread>

#include "RealTime.h"

#include <string>
#include <iostream>
#include <fstream>

class QString;
class QUrl;

QDebug &operator<<(QDebug &, const std::string &);
std::ostream &operator<<(std::ostream &, const QString &);
std::ostream &operator<<(std::ostream &, const QUrl &);

class SVDebug {
public:
    SVDebug();
    ~SVDebug();

    template <typename T>
    SVDebug &operator<<(const T &t) {
        if (m_silenced) return *this;
        if (m_ok) {
            if (m_eol) {
                m_stream << m_prefix << "/"
                         << (int)QThread::currentThreadId() << ":"
                         << m_timer.elapsed() << ": ";
            }
            m_stream << t;
            m_eol = false;
        }
        return *this;
    }

    SVDebug &operator<<(QTextStreamFunction) {
        if (m_silenced) return *this;
        m_stream << std::endl;
        m_eol = true;
        return *this;
    }

    static void silence() { m_silenced = true; }

    static void installQtMessageHandler();
    
private:
    std::fstream m_stream;
    char *m_prefix;
    bool m_ok;
    bool m_eol;
    QElapsedTimer m_timer;
    static bool m_silenced;
};

class SVCerr {
public:
    SVCerr(SVDebug &d) : m_d(d) { }
    
    template <typename T>
    inline SVCerr &operator<<(const T &t) {
        if (m_silenced) return *this;
        m_d << t;
        std::cerr << t;
        return *this;
    }

    inline SVCerr &operator<<(QTextStreamFunction f) {
        if (m_silenced) return *this;
        m_d << f;
        std::cerr << std::endl;
        return *this;
    }

    static void silence() { m_silenced = true; }

    static void installQtMessageHandler();
    
private:
    SVDebug &m_d;
    static bool m_silenced;
};

extern SVDebug &getSVDebug();
extern SVCerr &getSVCerr();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using Qt::endl;
#elif QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using QTextStreamFunctions::endl;
#endif

// Writes to debug log only
#define SVDEBUG getSVDebug()

// Writes to both SVDEBUG and cerr
#define SVCERR getSVCerr()

class FunctionLogger
{
public:
    FunctionLogger(const char *name);
    ~FunctionLogger();
private:
    const char *m_name;
    static int m_depth;
};

#ifdef NDEBUG
#define FUNCLOG
#else
#define FUNCLOG FunctionLogger functionLogger(__FUNCTION__)
#endif

#endif /* !_DEBUG_H_ */

