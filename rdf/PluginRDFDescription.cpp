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

PluginRDFDescription::OutputType
PluginRDFDescription::getOutputType(QString outputId) const
{
    if (m_outputTypes.find(outputId) == m_outputTypes.end()) {
        return OutputTypeUnknown;
    }
    return m_outputTypes.find(outputId)->second;
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
PluginRDFDescription::getOutputFeatureTypeURI(QString outputId) const
{
    if (m_outputFeatureTypeURIMap.find(outputId) ==
        m_outputFeatureTypeURIMap.end()) {
        return "";
    }
    return m_outputFeatureTypeURIMap.find(outputId)->second;
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

             " SELECT ?output_id ?output_type ?feature_type ?event_type ?unit "
             " FROM <%1> "

             " WHERE { "

             "   ?plugin a vamp:Plugin ; "
             "           vamp:identifier \"%2\" ; "
             "           vamp:output_descriptor ?output . "

             "   ?output vamp:identifier ?output_id ; "
             "           a ?output_type . "

             "   OPTIONAL { "
             "     ?output vamp:computes_feature_type ?feature_type "
             "   } . "

             "   OPTIONAL { "
             "     ?output vamp:computes_event_type ?event_type "
             "   } . "

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
             << "> does not appear to describe any plugin outputs" << endl;
        return false;
    }

    // Note that an output may appear more than once, if it inherits
    // more than one type (e.g. DenseOutput and QuantizedOutput).  So
    // these results must accumulate

    for (int i = 0; i < results.size(); ++i) {

        QString outputId = results[i]["output_id"].value;

        if (m_outputTypes.find(outputId) == m_outputTypes.end()) {
            m_outputTypes[outputId] = OutputTypeUnknown;
        }

        QString outputType = results[i]["output_type"].value;

        if (outputType.contains("DenseOutput")) {
            m_outputDispositions[outputId] = OutputDense;
        } else if (outputType.contains("SparseOutput")) {
            m_outputDispositions[outputId] = OutputSparse;
        } else if (outputType.contains("TrackLevelOutput")) {
            m_outputDispositions[outputId] = OutputTrackLevel;
        }

        if (results[i]["feature_type"].type == SimpleSPARQLQuery::URIValue) {

            QString featureType = results[i]["feature_type"].value;

            if (featureType != "") {
                if (m_outputTypes[outputId] == OutputEvents) {
                    m_outputTypes[outputId] = OutputFeaturesAndEvents;
                } else {
                    m_outputTypes[outputId] = OutputFeatures;
                }
                m_outputFeatureTypeURIMap[outputId] = featureType;
            }
        }

        if (results[i]["event_type"].type == SimpleSPARQLQuery::URIValue) {

            QString eventType = results[i]["event_type"].value;

            if (eventType != "") {
                if (m_outputTypes[outputId] == OutputFeatures) {
                    m_outputTypes[outputId] = OutputFeaturesAndEvents;
                } else {
                    m_outputTypes[outputId] = OutputEvents;
                }
                m_outputEventTypeURIMap[outputId] = eventType;
            }
        }
            
        if (results[i]["unit"].type == SimpleSPARQLQuery::LiteralValue) {

            QString unit = results[i]["unit"].value;
            
            if (unit != "") {
                m_outputUnitMap[outputId] = unit;
            }
        }
    }

    return true;
}

