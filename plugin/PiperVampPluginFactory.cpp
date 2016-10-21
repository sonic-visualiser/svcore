/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PiperVampPluginFactory.h"
#include "PluginIdentifier.h"

#include "system/System.h"

#include "PluginScan.h"

#ifdef _WIN32
#undef VOID
#undef ERROR
#define CAPNP_LITE 1
#endif

#include "vamp-client/AutoPlugin.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <iostream>

#include "base/Profiler.h"

#include "vamp-client/ProcessQtTransport.h"
#include "vamp-client/CapnpRRClient.h"

using namespace std;

//#define DEBUG_PLUGIN_SCAN_AND_INSTANTIATE 1

PiperVampPluginFactory::PiperVampPluginFactory() :
    m_serverName("piper-cpp/bin/piper-vamp-server") //!!!
{
}

vector<QString>
PiperVampPluginFactory::getPluginIdentifiers()
{
    Profiler profiler("PiperVampPluginFactory::getPluginIdentifiers");

    QMutexLocker locker(&m_mutex);

    if (m_pluginData.empty()) {
        populate();
    }

    vector<QString> rv;

    for (const auto &d: m_pluginData) {
        rv.push_back(QString("vamp:") + QString::fromStdString(d.second.pluginKey));
    }

    return rv;
}

Vamp::Plugin *
PiperVampPluginFactory::instantiatePlugin(QString identifier,
                                          sv_samplerate_t inputSampleRate)
{
    Profiler profiler("PiperVampPluginFactory::instantiatePlugin");

    auto psd = getPluginStaticData(identifier);
    if (psd.pluginKey == "") {
        return 0;
    }
    
    auto ap = new piper_vamp::client::AutoPlugin
        (m_serverName, psd.pluginKey, float(inputSampleRate), 0);
    if (!ap->isOK()) {
        delete ap;
        return 0;
    }

    return ap;
}

piper_vamp::PluginStaticData
PiperVampPluginFactory::getPluginStaticData(QString identifier)
{
    if (m_pluginData.find(identifier) != m_pluginData.end()) {
        return m_pluginData[identifier];
    } else {
        return {};
    }
}

QString
PiperVampPluginFactory::getPluginCategory(QString identifier)
{
    if (m_taxonomy.find(identifier) != m_taxonomy.end()) {
        return m_taxonomy[identifier];
    } else {
        return {};
    }
}

void
PiperVampPluginFactory::populate()
{
    piper_vamp::client::ProcessQtTransport transport(m_serverName);
    piper_vamp::client::CapnpRRClient client(&transport);
    piper_vamp::ListResponse lr = client.listPluginData();

    for (const auto &pd: lr.available) {

        QString identifier =
            QString("vamp:") + QString::fromStdString(pd.pluginKey);

        m_pluginData[identifier] = pd;

        QStringList catlist;
        for (const auto &cs: pd.category) {
            catlist.push_back(QString::fromStdString(cs));
        }

        m_taxonomy[identifier] = catlist.join(" > ");
    }
}

