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

#include "Debug.h"
#include "ResourceFinder.h"

#include <QMutex>
#include <QDir>
#include <QUrl>
#include <QCoreApplication>
#include <QDateTime>

#include <stdexcept>
#include <sstream>
#include <memory>

static std::unique_ptr<SVDebug> svdebug = nullptr;
static std::unique_ptr<SVCerr> svcerr = nullptr;
static QMutex mutex;

SVDebug &getSVDebug() {
    mutex.lock();
    if (!svdebug) {
        svdebug = std::unique_ptr<SVDebug>(new SVDebug());
    }
    mutex.unlock();
    return *svdebug;
}

SVCerr &getSVCerr() {
    mutex.lock();
    if (!svcerr) {
        if (!svdebug) {
            svdebug = std::unique_ptr<SVDebug>(new SVDebug());
        }
        svcerr = std::unique_ptr<SVCerr>(new SVCerr(*svdebug));
    }
    mutex.unlock();
    return *svcerr;
}

bool SVDebug::m_silenced = false;
bool SVCerr::m_silenced = false;

SVDebug::SVDebug() :
    m_prefix(nullptr),
    m_ok(false),
    m_eol(true)
{
    if (m_silenced) return;

    m_timer.start();
    
    if (qApp->applicationName() == "") {
        std::cerr << "ERROR: Can't use SVDEBUG before setting application name" << std::endl;
        throw std::logic_error("Can't use SVDEBUG before setting application name");
    }
    
    QString pfx = sv::ResourceFinder().getUserResourcePrefix();
    QDir logdir(QString("%1/%2").arg(pfx).arg("log"));

    m_prefix = strdup(QString("[%1]")
                      .arg(QCoreApplication::applicationPid())
                      .toLatin1().data());

    //!!! what to do if mkpath fails?
    if (!logdir.exists()) logdir.mkpath(logdir.path());

    QString fileName = logdir.path() + "/sv-debug.log";

    m_stream.open(fileName.toLocal8Bit().data(), std::ios_base::out);

    if (!m_stream) {
        QDebug(QtWarningMsg) << (const char *)m_prefix
                             << "Failed to open debug log file "
                             << fileName << " for writing";
    } else {
        m_ok = true;
//        cerr << "Log file is " << fileName << endl;
        (*this) << "Debug log started at "
                << QDateTime::currentDateTime().toString() << endl;
    }
}

SVDebug::~SVDebug()
{
    if (m_stream) {
        (*this) << "Debug log ends" << endl;
        m_stream.close();
    }
}

QDebug &
operator<<(QDebug &dbg, const std::string &s)
{
    dbg << QString::fromUtf8(s.c_str());
    return dbg;
}

std::ostream &
operator<<(std::ostream &target, const QString &str)
{
    return target << str.toUtf8().data();
}

std::ostream &
operator<<(std::ostream &target, const QUrl &u)
{
    return target << "<" << u.toString() << ">";
}

static void
svDebugQtMessageHandler(QtMsgType type,
                        const QMessageLogContext &context,
                        const QString &msg)
{
    SVDEBUG << qPrintable(qFormatLogMessage(type, context, msg)) << endl;
}

void
SVDebug::installQtMessageHandler()
{
    (void)qInstallMessageHandler(svDebugQtMessageHandler);
}

static void
svCerrQtMessageHandler(QtMsgType type,
                        const QMessageLogContext &context,
                        const QString &msg)
{
    SVCERR << qPrintable(qFormatLogMessage(type, context, msg)) << endl;
}

void
SVCerr::installQtMessageHandler()
{
    (void)qInstallMessageHandler(svCerrQtMessageHandler);
}

static int funcLoggerDepth = 0;
static QMutex funcLoggerMutex;

FunctionLogger::FunctionLogger(const char *name) :
    m_name(name)
{
    QMutexLocker locker(&funcLoggerMutex);
    std::ostringstream os;
    for (int i = 0; i < funcLoggerDepth; ++i) {
        os << "  ";
    }
    os << "-[>] " << m_name;
    SVDEBUG << os.str() << endl;
    ++funcLoggerDepth;
}

FunctionLogger::~FunctionLogger()
{
    QMutexLocker locker(&funcLoggerMutex);
    --funcLoggerDepth;
    std::ostringstream os;
    for (int i = 0; i < funcLoggerDepth; ++i) {
        os << "  ";
    }
    os << "<[-] " << m_name;
    SVDEBUG << os.str() << endl;
}

