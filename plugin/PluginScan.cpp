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
#include "base/HelperExecPath.h"

#include "checker/knownplugins.h"

#include <QMutex>
#include <QCoreApplication>

using std::string;

//#define DEBUG_PLUGIN_SCAN 1

class PluginScan::Logger : public PluginCandidates::LogCallback
{
protected:
    void log(std::string message) {
#ifdef DEBUG_PLUGIN_SCAN
        cerr << "PluginScan: " << message;
#endif
        SVDEBUG << "PluginScan: " << message;
    }
};

PluginScan *PluginScan::getInstance()
{
    static QMutex mutex;
    static PluginScan *m_instance = 0;
    mutex.lock();
    if (!m_instance) m_instance = new PluginScan();
    mutex.unlock();
    return m_instance;
}

PluginScan::PluginScan() : m_kp(0), m_succeeded(false), m_logger(new Logger) {
}

PluginScan::~PluginScan() {
    clear();
    delete m_logger;
}

void
PluginScan::scan()
{
    QStringList helperPaths =
        HelperExecPath::getHelperExecutables("plugin-checker-helper");

    clear();

    for (auto p: helperPaths) {
        try {
            KnownPlugins *kp = new KnownPlugins(p.toStdString(), m_logger);
            m_kp.push_back(kp);
            m_succeeded = true;
        } catch (const std::exception &e) {
            cerr << "ERROR: PluginScan::scan: " << e.what()
                 << " (with helper path = " << p << ")" << endl;
        }
    }
}

void
PluginScan::clear()
{
    for (auto &p: m_kp) delete p;
    m_kp.clear();
    m_succeeded = false;
}

QStringList
PluginScan::getCandidateLibrariesFor(PluginType type) const
{
    KnownPlugins::PluginType kpt;
    switch (type) {
    case VampPlugin: kpt = KnownPlugins::VampPlugin; break;
    case LADSPAPlugin: kpt = KnownPlugins::LADSPAPlugin; break;
    case DSSIPlugin: kpt = KnownPlugins::DSSIPlugin; break;
    default: throw std::logic_error("Inconsistency in plugin type enums");
    }
    
    QStringList candidates;

    for (auto kp: m_kp) {

        auto c = kp->getCandidateLibrariesFor(kpt);

        std::cerr << "PluginScan: helper \"" << kp->getHelperExecutableName()
                  << "\" likes " << c.size() << " libraries of type "
                  << kp->getTagFor(kpt) << std::endl;

        for (auto s: c) candidates.push_back(s.c_str());

        if (type != VampPlugin) {
            // We are only interested in querying multiple helpers
            // when dealing with Vamp plugins, for which we can use
            // external servers and so in some cases can support
            // additional architectures. Other plugin formats are
            // loaded directly and so must match the host, which is
            // what the first helper is supposed to handle -- so
            // break after the first one if not querying Vamp
            break;
        }
    }

    return candidates;
}

QString
PluginScan::getStartupFailureReport() const
{
    if (!m_succeeded) {
	return QObject::tr("<b>Failed to scan for plugins</b>"
			   "<p>Failed to scan for plugins at startup. Possibly "
                           "the plugin checker helper program was not correctly "
                           "installed alongside %1?</p>")
            .arg(QCoreApplication::applicationName());
    }
    if (m_kp.empty()) {
	return QObject::tr("<b>Did not scan for plugins</b>"
			   "<p>Apparently no scan for plugins was attempted "
			   "(internal error?)</p>");
    }

    QString report;
    for (auto kp: m_kp) {
        report += QString::fromStdString(kp->getFailureReport());
    }
    if (report == "") {
	return report;
    }

    return QObject::tr("<b>Failed to load plugins</b>"
		       "<p>Failed to load one or more plugin libraries:</p>")
	+ report
        + QObject::tr("<p>These plugins may be incompatible with the system, "
                      "and will be ignored during this run of %1.</p>")
        .arg(QCoreApplication::applicationName());
}

