/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2008 QMUL.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PluginRDFDescription.h"

#include "PluginRDFIndexer.h"
#include "SimpleSPARQLQuery.h"

#include "data/fileio/FileSource.h"
#include "data/fileio/CachedFile.h"

#include "base/Profiler.h"

#include "plugin/PluginIdentifier.h"

#include <iostream>
using std::cerr;
using std::endl;

PluginRDFDescription::PluginRDFDescription(QString pluginId) :
    m_pluginId(pluginId),
    m_haveDescription(false)
{
    PluginRDFIndexer *indexer = PluginRDFIndexer::getInstance();
    QString url = indexer->getDescriptionURLForPluginId(pluginId);
    if (url == "") {
        cerr << "PluginRDFDescription: WARNING: No RDF description available for plugin ID \""
             << pluginId.toStdString() << "\"" << endl;
    } else {
        if (!indexURL(url)) {
            cerr << "PluginRDFDescription: ERROR: Failed to query RDF description for plugin ID \""
                 << pluginId.toStdString() << "\"" << endl;
        } else {
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

QString
PluginRDFDescription::getPluginInfoURL() const
{
    return m_pluginInfoURL;
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

bool
PluginRDFDescription::indexURL(QString url) 
{
    Profiler profiler("PluginRDFDescription::indexURL");

    QString type, soname, label;
    PluginIdentifier::parseIdentifier(m_pluginId, type, soname, label);

    bool success = true;

    QString local = url;

    if (FileSource::isRemote(url) &&
        FileSource::canHandleScheme(url)) {
        
        CachedFile cf(url);
        if (!cf.isOK()) {
            return false;
        }

        local = QUrl::fromLocalFile(cf.getLocalFilename()).toString();
    }
    
    if (!indexMetadata(local, label)) success = false;
    if (!indexOutputs(local, label)) success = false;

    return success;
}

bool
PluginRDFDescription::indexMetadata(QString url, QString label)
{
    Profiler profiler("PluginRDFDescription::indexMetadata");

    QString queryTemplate =
        QString(
            " PREFIX vamp: <http://purl.org/ontology/vamp/> "
            " PREFIX foaf: <http://xmlns.com/foaf/0.1/> "
            " PREFIX dc: <http://purl.org/dc/elements/1.1/> "
            " SELECT ?%4 FROM <%1> "
            " WHERE { "
            "   ?plugin a vamp:Plugin ; "
            "           vamp:identifier \"%2\" ; "
            "           %3 ?%4 . "
            " }")
        .arg(url)
        .arg(label);

    SimpleSPARQLQuery::Value v;

    v = SimpleSPARQLQuery::singleResultQuery
        (url, queryTemplate.arg("vamp:name").arg("name"), "name");
    
    if (v.type == SimpleSPARQLQuery::LiteralValue && v.value != "") {
        m_pluginName = v.value;
    }

    v = SimpleSPARQLQuery::singleResultQuery
        (url, queryTemplate.arg("dc:description").arg("description"), "description");
    
    if (v.type == SimpleSPARQLQuery::LiteralValue && v.value != "") {
        m_pluginDescription = v.value;
    }

    v = SimpleSPARQLQuery::singleResultQuery
        (url,
         QString(
            " PREFIX vamp: <http://purl.org/ontology/vamp/> "
            " PREFIX foaf: <http://xmlns.com/foaf/0.1/> "
            " SELECT ?name FROM <%1> "
            " WHERE { "
            "   ?plugin a vamp:Plugin ; "
            "           vamp:identifier \"%2\" ; "
            "           foaf:maker ?maker . "
            "   ?maker foaf:name ?name . "
            " }")
         .arg(url)
         .arg(label), "name");
    
    if (v.type == SimpleSPARQLQuery::LiteralValue && v.value != "") {
        m_pluginMaker = v.value;
    }

    // If we have a more-information URL for this plugin, then we take
    // that.  Otherwise, a more-information URL for the plugin
    // library would do nicely.  Failing that, we could perhaps use
    // any foaf:page URL at all that appears in the file -- but
    // perhaps that would be unwise

    v = SimpleSPARQLQuery::singleResultQuery
        (url,
         QString(
            " PREFIX vamp: <http://purl.org/ontology/vamp/> "
            " PREFIX foaf: <http://xmlns.com/foaf/0.1/> "
            " SELECT ?page from <%1> "
            " WHERE { "
            "   ?plugin a vamp:Plugin ; "
            "           vamp:identifier \"%2\" ; "
            "           foaf:page ?page . "
            " }")
         .arg(url)
         .arg(label), "page");

    if (v.type == SimpleSPARQLQuery::URIValue && v.value != "") {

        m_pluginInfoURL = v.value;

    } else {

        v = SimpleSPARQLQuery::singleResultQuery
            (url,
             QString(
                " PREFIX vamp: <http://purl.org/ontology/vamp/> "
                " PREFIX foaf: <http://xmlns.com/foaf/0.1/> "
                " SELECT ?page from <%1> "
                " WHERE { "
                "   ?library a vamp:PluginLibrary ; "
                "            vamp:available_plugin ?plugin ; "
                "            foaf:page ?page . "
                "   ?plugin a vamp:Plugin ; "
                "           vamp:identifier \"%2\" . "
                " }")
             .arg(url)
             .arg(label), "page");

        if (v.type == SimpleSPARQLQuery::URIValue && v.value != "") {

            m_pluginInfoURL = v.value;
        }
    }

    return true;
}

bool
PluginRDFDescription::indexOutputs(QString url, QString label)
{
    Profiler profiler("PluginRDFDescription::indexOutputs");

    SimpleSPARQLQuery query
        (url,
         QString
         (
             " PREFIX vamp: <http://purl.org/ontology/vamp/> "

             " SELECT ?output ?output_id ?output_type ?unit "
             " FROM <%1> "

             " WHERE { "

             "   ?plugin a vamp:Plugin ; "
             "           vamp:identifier \"%2\" ; "
             "           vamp:output ?output . "

             "   ?output vamp:identifier ?output_id ; "
             "           a ?output_type . "

             "   OPTIONAL { "
             "     ?output vamp:unit ?unit "
             "   } . "

             " } "
             )
         .arg(url)
         .arg(label));

    SimpleSPARQLQuery::ResultList results = query.execute();

    if (!query.isOK()) {
        cerr << "ERROR: PluginRDFDescription::indexURL: ERROR: Failed to query document at <"
             << url.toStdString() << ">: "
             << query.getErrorString().toStdString() << endl;
        return false;
    }

    if (results.empty()) {
        cerr << "ERROR: PluginRDFDescription::indexURL: NOTE: Document at <"
             << url.toStdString()
             << "> does not appear to describe any outputs for plugin with id \""
             << label.toStdString() << "\"" << endl;
        return false;
    }

    // Note that an output may appear more than once, if it inherits
    // more than one type (e.g. DenseOutput and QuantizedOutput).  So
    // these results must accumulate

    for (int i = 0; i < results.size(); ++i) {

        QString outputUri = results[i]["output"].value;
        QString outputId = results[i]["output_id"].value;
        QString outputType = results[i]["output_type"].value;

        if (outputType.contains("DenseOutput")) {
            m_outputDispositions[outputId] = OutputDense;
        } else if (outputType.contains("SparseOutput")) {
            m_outputDispositions[outputId] = OutputSparse;
        } else if (outputType.contains("TrackLevelOutput")) {
            m_outputDispositions[outputId] = OutputTrackLevel;
        } else {
            m_outputDispositions[outputId] = OutputDispositionUnknown;
        }
            
        if (results[i]["unit"].type == SimpleSPARQLQuery::LiteralValue) {

            QString unit = results[i]["unit"].value;
            
            if (unit != "") {
                m_outputUnitMap[outputId] = unit;
            }
        }

        SimpleSPARQLQuery::Value v;

        v = SimpleSPARQLQuery::singleResultQuery
            (url, 
             QString(" PREFIX vamp: <http://purl.org/ontology/vamp/> "
                     " PREFIX dc: <http://purl.org/dc/elements/1.1/> "
                     " SELECT ?title FROM <%1> "
                     " WHERE { <%2> dc:title ?title } ")
             .arg(url).arg(outputUri), "title");

        if (v.type == SimpleSPARQLQuery::LiteralValue && v.value != "") {
            m_outputNames[outputId] = v.value;
        }

        QString queryTemplate = 
            QString(" PREFIX vamp: <http://purl.org/ontology/vamp/> "
                    " SELECT ?%3 FROM <%1> "
                    " WHERE { <%2> vamp:computes_%3 ?%3 } ")
            .arg(url).arg(outputUri);

        v = SimpleSPARQLQuery::singleResultQuery
            (url, queryTemplate.arg("event_type"), "event_type");

        if (v.type == SimpleSPARQLQuery::URIValue && v.value != "") {
            m_outputEventTypeURIMap[outputId] = v.value;
        }

        v = SimpleSPARQLQuery::singleResultQuery
            (url, queryTemplate.arg("feature_attribute"), "feature_attribute");

        if (v.type == SimpleSPARQLQuery::URIValue && v.value != "") {
            m_outputFeatureAttributeURIMap[outputId] = v.value;
        }

        v = SimpleSPARQLQuery::singleResultQuery
            (url, queryTemplate.arg("signal_type"), "signal_type");

        if (v.type == SimpleSPARQLQuery::URIValue && v.value != "") {
            m_outputSignalTypeURIMap[outputId] = v.value;
        }
    }

    return true;
}

