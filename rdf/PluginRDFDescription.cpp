/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2008-2012 QMUL.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PluginRDFDescription.h"

#include "PluginRDFIndexer.h"

#include "base/Profiler.h"
#include "base/Debug.h"

#include "plugin/PluginIdentifier.h"

#include <dataquay/BasicStore.h>

namespace sv {

using Dataquay::Uri;
using Dataquay::Node;
using Dataquay::Nodes;
using Dataquay::Triple;
using Dataquay::Triples;
using Dataquay::BasicStore;

//#define DEBUG_PLUGIN_RDF_DESCRIPTION 1

PluginRDFDescription::PluginRDFDescription(QString pluginId) :
    m_pluginId(pluginId),
    m_haveDescription(false)
{
    PluginRDFIndexer *indexer = PluginRDFIndexer::getInstance();
    m_pluginUri = indexer->getURIForPluginId(pluginId);
    if (m_pluginUri == "") {
        SVDEBUG << "PluginRDFDescription: WARNING: No RDF description available for plugin ID \""
             << pluginId << "\"" << endl;
    } else {
        // All the data we need should be in our RDF model already:
        // if it's not there, we don't know where to find it anyway
        if (index()) {
            m_haveDescription = true;
        }
    }
}

PluginRDFDescription::~PluginRDFDescription()
{
}

bool
PluginRDFDescription::haveDescription() const
{
    return m_haveDescription;
}

QString
PluginRDFDescription::getPluginName() const
{
    return m_pluginName;
}

QString
PluginRDFDescription::getPluginDescription() const
{
    return m_pluginDescription;
}

QString
PluginRDFDescription::getPluginMaker() const
{
    return m_pluginMaker;
}

Provider
PluginRDFDescription::getPluginProvider() const
{
    return m_provider;
}

QStringList
PluginRDFDescription::getOutputIds() const
{
    QStringList ids;
    for (OutputDispositionMap::const_iterator i = m_outputDispositions.begin();
         i != m_outputDispositions.end(); ++i) {
        ids.push_back(i->first);
    }
    return ids;
}

QString
PluginRDFDescription::getOutputName(QString outputId) const
{
    if (m_outputNames.find(outputId) == m_outputNames.end()) {
        return "";
    } 
    return m_outputNames.find(outputId)->second;
}

PluginRDFDescription::OutputDisposition
PluginRDFDescription::getOutputDisposition(QString outputId) const
{
    if (m_outputDispositions.find(outputId) == m_outputDispositions.end()) {
        return OutputDispositionUnknown;
    }
    return m_outputDispositions.find(outputId)->second;
}

QString
PluginRDFDescription::getOutputEventTypeURI(QString outputId) const
{
    if (m_outputEventTypeURIMap.find(outputId) ==
        m_outputEventTypeURIMap.end()) {
        return "";
    }
    return m_outputEventTypeURIMap.find(outputId)->second;
}

QString
PluginRDFDescription::getOutputFeatureAttributeURI(QString outputId) const
{
    if (m_outputFeatureAttributeURIMap.find(outputId) ==
        m_outputFeatureAttributeURIMap.end()) {
        return "";
    }
    return m_outputFeatureAttributeURIMap.find(outputId)->second;
}

QString
PluginRDFDescription::getOutputSignalTypeURI(QString outputId) const
{
    if (m_outputSignalTypeURIMap.find(outputId) ==
        m_outputSignalTypeURIMap.end()) {
        return "";
    }
    return m_outputSignalTypeURIMap.find(outputId)->second;
}

QString
PluginRDFDescription::getOutputUnit(QString outputId) const
{
    if (m_outputUnitMap.find(outputId) == m_outputUnitMap.end()) {
        return "";
    }
    return m_outputUnitMap.find(outputId)->second;
}

QString
PluginRDFDescription::getOutputUri(QString outputId) const
{
    if (m_outputUriMap.find(outputId) == m_outputUriMap.end()) {
        return "";
    }
    return m_outputUriMap.find(outputId)->second;
}

bool
PluginRDFDescription::index() 
{
    Profiler profiler("PluginRDFDescription::index");

    bool success = true;
    if (!indexMetadata()) success = false;
    if (!indexOutputs()) success = false;

    return success;
}

bool
PluginRDFDescription::indexMetadata()
{
    Profiler profiler("PluginRDFDescription::index");

    PluginRDFIndexer *indexer = PluginRDFIndexer::getInstance();
    const BasicStore *index = indexer->getIndex();
    Uri plugin(m_pluginUri);

    Node n = index->complete
        (Triple(plugin, index->expand("vamp:name"), Node()));

    if (n.type == Node::Literal && n.value != "") {
        m_pluginName = n.value;
    }

    n = index->complete
        (Triple(plugin, index->expand("dc:description"), Node()));

    if (n.type == Node::Literal && n.value != "") {
        m_pluginDescription = n.value;
    }

    n = index->complete
        (Triple(plugin, index->expand("foaf:maker"), Node()));

    if (n.type == Node::URI || n.type == Node::Blank) {
        n = index->complete(Triple(n, index->expand("foaf:name"), Node()));
        if (n.type == Node::Literal && n.value != "") {
            m_pluginMaker = n.value;
        }
    }

    // If we have a more-information URL for this plugin, then we take
    // that.  Otherwise, a more-information URL for the plugin library
    // would do nicely.

    n = index->complete
        (Triple(plugin, index->expand("foaf:page"), Node()));

    if (n.type == Node::URI && n.value != "") {
        m_provider.infoUrl = n.value;
    }

    // There may be more than one library node claiming this
    // plugin. That's because older RDF descriptions tend to use a
    // library node URI derived from the description's own URI, so it
    // varies depending on where you read the description from. It's
    // common therefore to end up with both a file: URI (from an
    // installed older version) and an http: one (from an online
    // updated version). We have no way to pick an authoritative one,
    // but it's also common that only one of them will have the
    // resources we need anyway, so let's iterate through them all.
    
    Nodes libnodes = index->match
        (Triple(Node(), index->expand("vamp:available_plugin"), plugin))
        .subjects();

    for (Node libn: libnodes) {

        if (libn.type != Node::URI || libn.value == "") {
            continue;
        }
        
        n = index->complete
            (Triple(libn, index->expand("foaf:page"), Node()));

        if (n.type == Node::URI && n.value != "") {
            m_provider.infoUrl = n.value;
        }

        n = index->complete
            (Triple(libn, index->expand("doap:download-page"), Node()));

        if (n.type == Node::URI && n.value != "") {
            m_provider.downloadUrl = n.value;

            n = index->complete
                (Triple(libn, index->expand("vamp:has_source"), Node()));
            if (n.type == Node::Literal && n.value == "true") {
                m_provider.downloadTypes.insert(Provider::DownloadSourceCode);
            }

            Nodes binaries = index->match
                (Triple(libn, index->expand("vamp:has_binary"), Node()))
                .objects();

            for (Node bin: binaries) {
                if (bin.type != Node::Literal) continue;
                if (bin.value == "linux32") {
                    m_provider.downloadTypes.insert(Provider::DownloadLinux32);
                } else if (bin.value == "linux64") {
                    m_provider.downloadTypes.insert(Provider::DownloadLinux64);
                } else if (bin.value == "win32") {
                    m_provider.downloadTypes.insert(Provider::DownloadWindows);
                } else if (bin.value == "osx") {
                    m_provider.downloadTypes.insert(Provider::DownloadMac);
                }
            }
        }

        Nodes packs = index->match
            (Triple(Node(), index->expand("vamp:available_library"), libn))
            .subjects();

#ifdef DEBUG_PLUGIN_RDF_DESCRIPTION
        SVCERR << packs.size() << " matching pack(s) for library node "
               << libn << endl;
#endif

        for (Node packn: packs) {
            if (packn.type != Node::URI) continue;

            QString packName;
            QString packUrl;
            n = index->complete
                (Triple(packn, index->expand("dc:title"), Node()));
            if (n.type == Node::Literal) {
                packName = n.value;
            }
            n = index->complete
                (Triple(packn, index->expand("foaf:page"), Node()));
            if (n.type == Node::URI) {
                packUrl = n.value;
            }

            if (packName != "" && packUrl != "") {
                m_provider.foundInPacks[packName] = packUrl;
            }
        }
    }

#ifdef DEBUG_PLUGIN_RDF_DESCRIPTION
    SVCERR << "PluginRDFDescription::indexMetadata:" << endl;
    SVCERR << " * id: " << m_pluginId << endl;
    SVCERR << " * uri: <" << m_pluginUri << ">" << endl;
    SVCERR << " * name: " << m_pluginName << endl;
    SVCERR << " * description: " << m_pluginDescription << endl;
    SVCERR << " * maker: " << m_pluginMaker << endl;
    SVCERR << " * info url: <" << m_provider.infoUrl << ">" << endl;
    SVCERR << " * download url: <" << m_provider.downloadUrl << ">" << endl;
    SVCERR << " * download types:" << endl;
    for (auto t: m_provider.downloadTypes) {
        SVCERR << "   * " << int(t) << endl;
    }
    SVCERR << " * packs:" << endl;
    for (auto t: m_provider.foundInPacks) {
        SVCERR << "   * " << t.first
               << ", download url: <" << t.second << ">" << endl;
    }
    SVCERR << endl;
#endif    

    return true;
}

bool
PluginRDFDescription::indexOutputs()
{
    Profiler profiler("PluginRDFDescription::indexOutputs");
    
    PluginRDFIndexer *indexer = PluginRDFIndexer::getInstance();
    const BasicStore *index = indexer->getIndex();
    Uri plugin(m_pluginUri);

    Nodes outputs = index->match
        (Triple(plugin, index->expand("vamp:output"), Node())).objects();

    if (outputs.empty()) {
        SVDEBUG << "ERROR: PluginRDFDescription::indexURL: NOTE: No outputs defined for <"
             << m_pluginUri << ">" << endl;
        return false;
    }

    foreach (Node output, outputs) {

        if ((output.type != Node::URI && output.type != Node::Blank) ||
            output.value == "") {
            SVDEBUG << "ERROR: PluginRDFDescription::indexURL: No valid URI for output " << output << " of plugin <" << m_pluginUri << ">" << endl;
            return false;
        }
        
        Node n = index->complete(Triple(output, index->expand("vamp:identifier"), Node()));
        if (n.type != Node::Literal || n.value == "") {
            SVDEBUG << "ERROR: PluginRDFDescription::indexURL: No vamp:identifier for output <" << output << ">" << endl;
            return false;
        }
        QString outputId = n.value;

        m_outputUriMap[outputId] = output.value;

        n = index->complete(Triple(output, Uri("a"), Node()));
        QString outputType;
        if (n.type == Node::URI) outputType = n.value;

        n = index->complete(Triple(output, index->expand("vamp:unit"), Node()));
        QString outputUnit;
        if (n.type == Node::Literal) outputUnit = n.value;

        if (outputType.contains("DenseOutput")) {
            m_outputDispositions[outputId] = OutputDense;
        } else if (outputType.contains("SparseOutput")) {
            m_outputDispositions[outputId] = OutputSparse;
        } else if (outputType.contains("TrackLevelOutput")) {
            m_outputDispositions[outputId] = OutputTrackLevel;
        } else {
            m_outputDispositions[outputId] = OutputDispositionUnknown;
        }
//        SVDEBUG << "output " << output << " -> id " << outputId << ", type " << outputType << ", unit " 
//             << outputUnit << ", disposition " << m_outputDispositions[outputId] << endl;
            
        if (outputUnit != "") {
            m_outputUnitMap[outputId] = outputUnit;
        }

        n = index->complete(Triple(output, index->expand("dc:title"), Node()));
        if (n.type == Node::Literal && n.value != "") {
            m_outputNames[outputId] = n.value;
        }

        n = index->complete(Triple(output, index->expand("vamp:computes_event_type"), Node()));
//        SVDEBUG << output << " -> computes_event_type " << n << endl;
        if (n.type == Node::URI && n.value != "") {
            m_outputEventTypeURIMap[outputId] = n.value;
        }

        n = index->complete(Triple(output, index->expand("vamp:computes_feature"), Node()));
        if (n.type == Node::URI && n.value != "") {
            m_outputFeatureAttributeURIMap[outputId] = n.value;
        }

        n = index->complete(Triple(output, index->expand("vamp:computes_signal_type"), Node()));
        if (n.type == Node::URI && n.value != "") {
            m_outputSignalTypeURIMap[outputId] = n.value;
        }
    }

    return true;
}

} // end namespace sv

