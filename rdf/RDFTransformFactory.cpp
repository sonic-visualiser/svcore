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

#include "RDFTransformFactory.h"

#include <map>
#include <vector>

#include <iostream>
#include <cmath>

#include "SimpleSPARQLQuery.h"
#include "PluginRDFIndexer.h"
#include "base/ProgressReporter.h"

#include "transform/TransformFactory.h"

using std::cerr;
using std::endl;

typedef const unsigned char *STR; // redland's expected string type


class RDFTransformFactoryImpl
{
public:
    RDFTransformFactoryImpl(QString url);
    virtual ~RDFTransformFactoryImpl();
    
    bool isOK();
    QString getErrorString() const;

    std::vector<Transform> getTransforms(ProgressReporter *);

protected:
    QString m_urlString;
    QString m_errorString;
    bool setOutput(Transform &, QString, QString);
    bool setParameters(Transform &, QString, QString);
};


QString
RDFTransformFactory::getKnownExtensions()
{
    return "*.rdf *.n3 *.ttl";
}

RDFTransformFactory::RDFTransformFactory(QString url) :
    m_d(new RDFTransformFactoryImpl(url)) 
{
}

RDFTransformFactory::~RDFTransformFactory()
{
    delete m_d;
}

bool
RDFTransformFactory::isOK()
{
    return m_d->isOK();
}

QString
RDFTransformFactory::getErrorString() const
{
    return m_d->getErrorString();
}

std::vector<Transform>
RDFTransformFactory::getTransforms(ProgressReporter *r)
{
    return m_d->getTransforms(r);
}

RDFTransformFactoryImpl::RDFTransformFactoryImpl(QString url) :
    m_urlString(url)
{
}

RDFTransformFactoryImpl::~RDFTransformFactoryImpl()
{
}

bool
RDFTransformFactoryImpl::isOK()
{
    return (m_errorString == "");
}

QString
RDFTransformFactoryImpl::getErrorString() const
{
    return m_errorString;
}

std::vector<Transform>
RDFTransformFactoryImpl::getTransforms(ProgressReporter *reporter)
{
    std::vector<Transform> transforms;

    // We have to do this a very long way round, to work around
    // rasqal's current inability to handle correctly more than one
    // OPTIONAL graph in a query

    const char *optionals[] = {
        "output",
        "program",
        "step_size",
        "block_size",
        "window_type",
        "sample_rate",
        "start", 
        "duration"
    };

    std::map<QString, Transform> uriTransformMap;

    QString queryTemplate = 
        " PREFIX vamp: <http://purl.org/ontology/vamp/> "

        " SELECT ?transform ?plugin %1 "
        
        " FROM <%2> "

        " WHERE { "
        "   ?transform a vamp:Transform ; "
        "              vamp:plugin ?plugin . "
        "   %3 "
        " } ";

    SimpleSPARQLQuery transformsQuery
        (queryTemplate.arg("").arg(m_urlString).arg(""));

    SimpleSPARQLQuery::ResultList transformResults = transformsQuery.execute();

    if (!transformsQuery.isOK()) {
        m_errorString = transformsQuery.getErrorString();
        return transforms;
    }

    if (transformResults.empty()) {
        cerr << "RDFTransformFactory: NOTE: No RDF/TTL transform descriptions found in document at <" << m_urlString.toStdString() << ">" << endl;
        return transforms;
    }

    PluginRDFIndexer *indexer = PluginRDFIndexer::getInstance();

    for (int i = 0; i < transformResults.size(); ++i) {

        SimpleSPARQLQuery::KeyValueMap &result = transformResults[i];

        QString transformUri = result["transform"].value;
        QString pluginUri = result["plugin"].value;

        QString pluginId = indexer->getIdForPluginURI(pluginUri);
        if (pluginId == "") {
            cerr << "RDFTransformFactory: WARNING: Unknown plugin <"
                 << pluginUri.toStdString() << "> for transform <"
                 << transformUri.toStdString() << ">, skipping this transform"
                 << endl;
            continue;
        }

        QString pluginDescriptionURL =
            indexer->getDescriptionURLForPluginId(pluginId);
        if (pluginDescriptionURL == "") {
            cerr << "RDFTransformFactory: WARNING: No RDF description available for plugin <"
                 << pluginUri.toStdString() << ">, skipping transform <"
                 << transformUri.toStdString() << ">" << endl;
            continue;
        }

        Transform transform;
        transform.setPluginIdentifier(pluginId);

        if (!setOutput(transform, transformUri, pluginDescriptionURL)) {
            return transforms;
        }

        if (!setParameters(transform, transformUri, pluginDescriptionURL)) {
            return transforms;
        }

        uriTransformMap[transformUri] = transform;
    }

    for (int i = 0; i < sizeof(optionals)/sizeof(optionals[0]); ++i) {

        QString optional = optionals[i];

        SimpleSPARQLQuery query
            (queryTemplate
             .arg(QString("?%1").arg(optional))
             .arg(m_urlString)
             .arg(QString("?transform vamp:%1 ?%2")
                  .arg(optionals[i]).arg(optional)));
        
        SimpleSPARQLQuery::ResultList results = query.execute();

        if (!query.isOK()) {
            m_errorString = query.getErrorString();
            return transforms;
        }

        if (results.empty()) continue;

        for (int j = 0; j < results.size(); ++j) {

            QString transformUri = results[j]["transform"].value;
            
            if (uriTransformMap.find(transformUri) == uriTransformMap.end()) {
                cerr << "RDFTransformFactory: ERROR: Transform URI <"
                     << transformUri.toStdString() << "> not found in internal map!" << endl;
                continue;
            }

            Transform &transform = uriTransformMap[transformUri];
            const SimpleSPARQLQuery::Value &v = results[j][optional];

            if (v.type == SimpleSPARQLQuery::LiteralValue) {
                
                if (optional == "program") {
                    transform.setProgram(v.value);
                } else if (optional == "step_size") {
                    transform.setStepSize(v.value.toUInt());
                } else if (optional == "block_size") {
                    transform.setBlockSize(v.value.toUInt());
                } else if (optional == "window_type") {
                    cerr << "NOTE: can't handle window type yet (value is \""
                         << v.value.toStdString() << "\")" << endl;
                } else if (optional == "sample_rate") {
                    transform.setSampleRate(v.value.toFloat());
                } else if (optional == "start") {
                    transform.setStartTime
                        (RealTime::fromXsdDuration(v.value.toStdString()));
                } else if (optional == "duration") {
                    transform.setDuration
                        (RealTime::fromXsdDuration(v.value.toStdString()));
                } else {
                    cerr << "RDFTransformFactory: ERROR: Inconsistent optionals lists (unexpected optional \"" << optional.toStdString() << "\"" << endl;
                }
            }
        }
    }

    for (std::map<QString, Transform>::iterator i = uriTransformMap.begin();
         i != uriTransformMap.end(); ++i) {

        Transform &transform = i->second;

        cerr << "RDFTransformFactory: NOTE: Transform is: " << endl;
        cerr << transform.toXmlString().toStdString() << endl;

        transforms.push_back(transform);
    }
        
    return transforms;
}

bool
RDFTransformFactoryImpl::setOutput(Transform &transform,
                                   QString transformUri,
                                   QString pluginDescriptionURL)
{
    SimpleSPARQLQuery outputQuery
        (QString
         (
             " PREFIX vamp: <http://purl.org/ontology/vamp/> "
             
             " SELECT ?output_id "
             
             " FROM <%1> "
             " FROM <%2> "
             
             " WHERE { "
             "   <%3> vamp:output ?output . "
             "   ?output vamp:identifier ?output_id "
             " } "
             )
         .arg(m_urlString)
         .arg(pluginDescriptionURL)
         .arg(transformUri));
    
    SimpleSPARQLQuery::ResultList outputResults = outputQuery.execute();
    
    if (!outputQuery.isOK()) {
        m_errorString = outputQuery.getErrorString();
        return false;
    }
    
    if (outputQuery.wasCancelled()) {
        m_errorString = "Query cancelled";
        return false;
    }
    
    for (int j = 0; j < outputResults.size(); ++j) {
        QString outputId = outputResults[j]["output_id"].value;
        transform.setOutput(outputId);
    }

    return true;
}
        

bool
RDFTransformFactoryImpl::setParameters(Transform &transform,
                                       QString transformUri,
                                       QString pluginDescriptionURL)
{
    SimpleSPARQLQuery paramQuery
        (QString
         (
             " PREFIX vamp: <http://purl.org/ontology/vamp/> "
             
             " SELECT ?param_id ?param_value "
             
             " FROM <%1> "
             " FROM <%2> "
             
             " WHERE { "
             "   <%3> vamp:parameter_binding ?binding . "
             "   ?binding vamp:parameter ?param ; "
             "            vamp:value ?param_value . "
             "   ?param vamp:identifier ?param_id "
             " } "
             )
         .arg(m_urlString)
         .arg(pluginDescriptionURL)
         .arg(transformUri));
    
    SimpleSPARQLQuery::ResultList paramResults = paramQuery.execute();
    
    if (!paramQuery.isOK()) {
        m_errorString = paramQuery.getErrorString();
        return false;
    }
    
    if (paramQuery.wasCancelled()) {
        m_errorString = "Query cancelled";
        return false;
    }
    
    for (int j = 0; j < paramResults.size(); ++j) {
        
        QString paramId = paramResults[j]["param_id"].value;
        QString paramValue = paramResults[j]["param_value"].value;
        
        if (paramId == "" || paramValue == "") continue;
        
        transform.setParameter(paramId, paramValue.toFloat());
    }

    return true;
}

