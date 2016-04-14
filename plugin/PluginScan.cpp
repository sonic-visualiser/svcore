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

PluginScan::PluginScan() : m_kp(0), m_succeeded(false) {
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
    m_succeeded = false;
    try {
	m_kp = new KnownPlugins("./helper", this); //!!!
	m_succeeded = true;
    } catch (const std::exception &e) {
	cerr << "ERROR: PluginScan::scan: " << e.what() << endl;
	m_kp = 0;
    }
}

QStringList
PluginScan::getCandidateLibrariesFor(KnownPlugins::PluginType type) const
{
    QStringList candidates;
    if (!m_kp) return candidates;
    auto c = m_kp->getCandidateLibrariesFor(type);
    for (auto s: c) candidates.push_back(s.c_str());
    return candidates;
}

QStringList
PluginScan::getCandidateVampLibraries() const
{
    return getCandidateLibrariesFor(KnownPlugins::VampPlugin);
}

QStringList
PluginScan::getCandidateLADSPALibraries() const
{
    return getCandidateLibrariesFor(KnownPlugins::LADSPAPlugin);
}

QStringList
PluginScan::getCandidateDSSILibraries() const
{
    return getCandidateLibrariesFor(KnownPlugins::DSSIPlugin);
}

QString
PluginScan::getStartupFailureReport() const
{
    if (!m_succeeded) {
	return QObject::tr("<b>Failed to scan for plugins</b>"
			   "<p>Failed to scan for plugins at startup "
			   "(application installation problem?)</p>");
    }
    if (!m_kp) {
	return QObject::tr("<b>Did not scan for plugins</b>"
			   "<p>Apparently no scan for plugins was attempted "
			   "(internal error?)</p>");
    }

    string report = m_kp->getFailureReport();
    if (report == "") {
	return QString(report.c_str());
    }

    return QObject::tr("<b>Failed to load plugins</b>"
		       "<p>Failed to load one or more plugin libraries:</p>")
	+ QString(report.c_str());
}

