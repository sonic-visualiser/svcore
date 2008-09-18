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
    QString type, soname, label;
    PluginIdentifier::parseIdentifier(m_pluginId, type, soname, label);

    SimpleSPARQLQuery query
        (QString
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
        }
            
        if (results[i]["unit"].type == SimpleSPARQLQuery::LiteralValue) {

            QString unit = results[i]["unit"].value;
            
            if (unit != "") {
                m_outputUnitMap[outputId] = unit;
            }
        }

        QString queryTemplate = 
            QString(" PREFIX vamp: <http://purl.org/ontology/vamp/> "
                    " SELECT ?%3 FROM <%1> "
                    " WHERE { <%2> vamp:computes_%3 ?%3 } ")
            .arg(url).arg(outputUri);

        SimpleSPARQLQuery::Value v;

        v = SimpleSPARQLQuery::singleResultQuery
            (queryTemplate.arg("event_type"), "event_type");

        if (v.type == SimpleSPARQLQuery::URIValue && v.value != "") {
            m_outputEventTypeURIMap[outputId] = v.value;
        }

        v = SimpleSPARQLQuery::singleResultQuery
            (queryTemplate.arg("feature_attribute"), "feature_attribute");

        if (v.type == SimpleSPARQLQuery::URIValue && v.value != "") {
            m_outputFeatureAttributeURIMap[outputId] = v.value;
        }

        v = SimpleSPARQLQuery::singleResultQuery
            (queryTemplate.arg("signal_type"), "signal_type");

        if (v.type == SimpleSPARQLQuery::URIValue && v.value != "") {
            m_outputSignalTypeURIMap[outputId] = v.value;
        }
    }

    return true;
}

