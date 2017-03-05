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
#include "base/Preferences.h"
#include "base/HelperExecPath.h"

#ifdef HAVE_PLUGIN_CHECKER_HELPER
#include "checker/knownplugins.h"
#else
class KnownPlugins {};
#endif

#include <QMutex>
#include <QCoreApplication>

using std::string;

class PluginScan::Logger
#ifdef HAVE_PLUGIN_CHECKER_HELPER
    : public PluginCandidates::LogCallback
#endif
{
protected:
    void log(std::string message) {
        SVDEBUG << "PluginScan: " << message << endl;
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

PluginScan::PluginScan() : m_succeeded(false), m_logger(new Logger) {
}

PluginScan::~PluginScan() {
    QMutexLocker locker(&m_mutex);
    clear();
    delete m_logger;
    SVDEBUG << "PluginScan::~PluginScan completed" << endl;
}

void
PluginScan::scan()
{
#ifdef HAVE_PLUGIN_CHECKER_HELPER

    QMutexLocker locker(&m_mutex);

    bool inProcess = Preferences::getInstance()->getRunPluginsInProcess();

    HelperExecPath hep(inProcess ?
                       HelperExecPath::NativeArchitectureOnly :
                       HelperExecPath::AllInstalled);

    QString helperName("vamp-plugin-load-checker");
    auto helpers = hep.getHelperExecutables(helperName);

    clear();

    for (auto p: helpers) {
        SVDEBUG << "NOTE: PluginScan: Found helper: " << p.executable << endl;
    }
    
    if (helpers.empty()) {
        SVDEBUG << "NOTE: No plugin checker helpers found in installation;"
             << " found none of the following:" << endl;
        for (auto d: hep.getHelperCandidatePaths(helperName)) {
            SVDEBUG << "NOTE: " << d << endl;
        }
    }

    for (auto p: helpers) {
        try {
            KnownPlugins *kp = new KnownPlugins
                (p.executable.toStdString(), m_logger);
            if (m_kp.find(p.tag) != m_kp.end()) {
                SVDEBUG << "WARNING: PluginScan::scan: Duplicate tag " << p.tag
                     << " for helpers" << endl;
                continue;
            }
            m_kp[p.tag] = kp;
            m_succeeded = true;
        } catch (const std::exception &e) {
            SVDEBUG << "ERROR: PluginScan::scan: " << e.what()
                 << " (with helper path = " << p.executable << ")" << endl;
        }
    }

    SVDEBUG << "PluginScan::scan complete" << endl;
#endif
}

bool
PluginScan::scanSucceeded() const
{
    QMutexLocker locker(&m_mutex);
    return m_succeeded;
}

void
PluginScan::clear()
{
    for (auto &p: m_kp) {
        delete p.second;
    }
    m_kp.clear();
    m_succeeded = false;
}

QList<PluginScan::Candidate>
PluginScan::getCandidateLibrariesFor(PluginType
#ifdef HAVE_PLUGIN_CHECKER_HELPER
                                     type
#endif
    ) const
{
#ifdef HAVE_PLUGIN_CHECKER_HELPER
    
    QMutexLocker locker(&m_mutex);

    KnownPlugins::PluginType kpt;
    switch (type) {
    case VampPlugin: kpt = KnownPlugins::VampPlugin; break;
    case LADSPAPlugin: kpt = KnownPlugins::LADSPAPlugin; break;
    case DSSIPlugin: kpt = KnownPlugins::DSSIPlugin; break;
    default: throw std::logic_error("Inconsistency in plugin type enums");
    }
    
    QList<Candidate> candidates;

    for (auto rec: m_kp) {

        KnownPlugins *kp = rec.second;
        
        auto c = kp->getCandidateLibrariesFor(kpt);

        SVDEBUG << "PluginScan: helper \"" << kp->getHelperExecutableName()
                << "\" likes " << c.size() << " libraries of type "
                << kp->getTagFor(kpt) << endl;

        for (auto s: c) {
            candidates.push_back({ s.c_str(), rec.first });
        }

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
    
#else
    return {};
#endif
}

QString
PluginScan::getStartupFailureReport() const
{
#ifdef HAVE_PLUGIN_CHECKER_HELPER
    
    QMutexLocker locker(&m_mutex);

    if (!m_succeeded) {
	return QObject::tr("<b>Failed to scan for plugins</b>"
			   "<p>Failed to scan for plugins at startup. Possibly "
                           "the plugin checker program was not correctly "
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
        report += QString::fromStdString(kp.second->getFailureReport());
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

#else
    return "";
#endif
}

