/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PluginScan.h"

#include "base/Debug.h"

#include <QMutex>

using std::string;

PluginScan *PluginScan::getInstance() {
    static QMutex mutex;
    static PluginScan *m_instance = 0;
    mutex.lock();
    if (!m_instance) m_instance = new PluginScan();
    mutex.unlock();
    return m_instance;
}

PluginScan::PluginScan() : m_kp(0) {
}

PluginScan::~PluginScan() {
    delete m_kp;
}

void
PluginScan::log(string message)
{
    SVDEBUG << "PluginScan: " << message;
}

void
PluginScan::scan()
{
    delete m_kp;
    m_kp = new KnownPlugins("./helper", this); //!!!
}

QStringList
PluginScan::getCandidateVampLibraries() const
{
    QStringList candidates;
    if (!m_kp) return candidates;
    auto c = m_kp->getCandidateLibrariesFor(KnownPlugins::VampPlugin);
    for (auto s: c) candidates.push_back(s.c_str());
    return candidates;
}

QStringList
PluginScan::getCandidateLADSPALibraries() const
{
    QStringList candidates;
    if (!m_kp) return candidates;
    auto c = m_kp->getCandidateLibrariesFor(KnownPlugins::LADSPAPlugin);
    for (auto s: c) candidates.push_back(s.c_str());
    return candidates;
}

QStringList
PluginScan::getCandidateDSSILibraries() const
{
    QStringList candidates;
    if (!m_kp) return candidates;
    auto c = m_kp->getCandidateLibrariesFor(KnownPlugins::DSSIPlugin);
    for (auto s: c) candidates.push_back(s.c_str());
    return candidates;
}

QString
PluginScan::getStartupFailureReport() const
{
    if (!m_kp) return ""; //!!!???
    string report = m_kp->getFailureReport();
    return report.c_str(); //!!! wrap?
}

