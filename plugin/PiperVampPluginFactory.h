/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2016 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_PIPER_VAMP_PLUGIN_FACTORY_H
#define SV_PIPER_VAMP_PLUGIN_FACTORY_H

#ifdef HAVE_PIPER

#include "FeatureExtractionPluginFactory.h"

#include <QMutex>
#include <vector>
#include <map>

#include "base/Debug.h"
#include "base/HelperExecPath.h"

/**
 * FeatureExtractionPluginFactory type for Vamp plugins hosted in a
 * separate process using Piper protocol.
 */
class PiperVampPluginFactory : public FeatureExtractionPluginFactory
{
public:
    using ServerName = std::string;
    using DesiredSubset = std::vector<std::string>;
    struct DesiredExtractors 
    {
        DesiredExtractors() : allAvailable(false) {} // filtered, but invalid, by default (from should not be empty)
        
        DesiredExtractors(DesiredSubset subset) 
            : allAvailable(subset.empty()), from(subset) {} // if empty assume all available are wanted, can populate struct manually if not
        
        bool allAvailable; // used to disambiguate an empty filter list from wanting all available extractors, client code should inspect both to determine validity 
        DesiredSubset from; // this should only be populated if allAvailable is false
    };
    
    struct ServerDescription 
    {
        ServerDescription(ServerName n) 
            : name(n), hasDesiredExtractors(false) {} // fall back to populating using PluginScan internally
        ServerDescription(ServerName n, DesiredSubset desired) 
            : name(n), hasDesiredExtractors(true), extractors(desired) {}
        ServerName name;
        bool hasDesiredExtractors; // indicates whether to override the ListRequest made internally
        DesiredExtractors extractors; // for populating the from field in a ListRequest
    };
    
    PiperVampPluginFactory();
    PiperVampPluginFactory(std::initializer_list<ServerDescription> servers);
    
    virtual void setDesiredExtractors(ServerName name, DesiredExtractors extractors);
    
    virtual ~PiperVampPluginFactory();

    virtual std::vector<QString> getPluginIdentifiers(QString &errorMessage)
        override;

    virtual piper_vamp::PluginStaticData getPluginStaticData(QString identifier)
        override;
    
    virtual Vamp::Plugin *instantiatePlugin(QString identifier,
                                            sv_samplerate_t inputSampleRate)
        override;

    virtual QString getPluginCategory(QString identifier) override;

protected:
    QMutex m_mutex;
    QList<HelperExecPath::HelperExec> m_servers; // executable file paths
    std::map<QString, QString> m_origins; // plugin identifier -> server path
    std::map<QString, piper_vamp::PluginStaticData> m_pluginData; // identifier -> data
    std::map<QString, QString> m_taxonomy; // identifier -> category string
    std::map<ServerName, DesiredExtractors> m_overrideDesiredExtractors;

    void populate(QString &errorMessage);
    void populateFrom(const HelperExecPath::HelperExec &, QString &errorMessage);

    class Logger;
    Logger *m_logger;
};

#endif

#endif
