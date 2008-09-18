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

#include <redland.h>
#include <rasqal.h>

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

    SimpleSPARQLQuery query
        (QString
         (
             " PREFIX vamp: <http://purl.org/ontology/vamp/> "

             " SELECT ?transform ?plugin ?output ?program "
             "        ?step_size ?block_size ?window_type "
             "        ?sample_rate ?start ?duration "

             " FROM <%1> "

             " WHERE { "
             "   ?transform a vamp:Transform ; "
             "              vamp:plugin ?plugin . "
             "   OPTIONAL { ?transform vamp:output ?output } . "
             "   OPTIONAL { ?transform vamp:program ?program } . "
             "   OPTIONAL { ?transform vamp:step_size ?step_size } . "
             "   OPTIONAL { ?transform vamp:block_size ?block_size } . "
             "   OPTIONAL { ?transform vamp:window_type ?window_type } . "
             "   OPTIONAL { ?transform vamp:sample_rate ?sample_rate } . "
             "   OPTIONAL { ?transform vamp:start ?start } . "
             "   OPTIONAL { ?transform vamp:duration ?duration } "
             " } "
             )
         .arg(m_urlString));

    SimpleSPARQLQuery::ResultList results = query.execute();

    if (!query.isOK()) {
        m_errorString = query.getErrorString();
        return transforms;
    }

    if (query.wasCancelled()) {
        m_errorString = "Query cancelled";
        return transforms;
    }

    PluginRDFIndexer *indexer = PluginRDFIndexer::getInstance();

    for (int i = 0; i < results.size(); ++i) {

        SimpleSPARQLQuery::KeyValueMap &result = results[i];

        QString transformUri = result["transform"].value;
        QString pluginUri = result["plugin"].value;

        QString pluginId = indexer->getIdForPluginURI(pluginUri);

        if (pluginId == "") {
            cerr << "RDFTransformFactory: WARNING: Unknown plugin <"
                 << pluginUri.toStdString() << "> for transform <"
                 << transformUri.toStdString() << ">" << endl;
            continue;
        }

        Transform transform;
        transform.setPluginIdentifier(pluginId);
        
        if (result["output"].type == SimpleSPARQLQuery::LiteralValue) {
            transform.setOutput(result["output"].value);
        }

        if (result["program"].type == SimpleSPARQLQuery::LiteralValue) {
            transform.setProgram(result["program"].value);
        }
        
        if (result["step_size"].type == SimpleSPARQLQuery::LiteralValue) {
            transform.setStepSize(result["step_size"].value.toUInt());
        }
        
        if (result["block_size"].type == SimpleSPARQLQuery::LiteralValue) {
            transform.setBlockSize(result["block_size"].value.toUInt());
        }
        
        if (result["window_type"].type == SimpleSPARQLQuery::LiteralValue) {
            cerr << "NOTE: can't handle window type yet (value is \""
                 << result["window_type"].value.toStdString() << "\")" << endl;
        }
        
        if (result["sample_rate"].type == SimpleSPARQLQuery::LiteralValue) {
            transform.setStepSize(result["sample_rate"].value.toFloat());
        }

        if (result["start"].type == SimpleSPARQLQuery::LiteralValue) {
            transform.setStartTime(RealTime::fromXsdDuration
                                   (result["start"].value.toStdString()));
        }

        if (result["duration"].type == SimpleSPARQLQuery::LiteralValue) {
            transform.setDuration(RealTime::fromXsdDuration
                                  (result["duration"].value.toStdString()));
        }

        SimpleSPARQLQuery paramQuery
            (QString
             (
                 " PREFIX vamp: <http://purl.org/ontology/vamp/> "

                 " SELECT ?param_id ?param_value "

                 " FROM <%1> "

                 " WHERE { "
                 "   <%2> vamp:parameter ?param . "
                 "   ?param vamp:identifier ?param_id ; "
                 "          vamp:value ?param_value "
                 " } "
                 )
             .arg(m_urlString)
             .arg(transformUri));
        
        SimpleSPARQLQuery::ResultList paramResults = paramQuery.execute();

        if (!paramQuery.isOK()) {
            m_errorString = paramQuery.getErrorString();
            return transforms;
        }

        if (paramQuery.wasCancelled()) {
            m_errorString = "Query cancelled";
            return transforms;
        }

        for (int j = 0; j < paramResults.size(); ++j) {

            QString paramId = paramResults[j]["param_id"].value;
            QString paramValue = paramResults[j]["param_value"].value;

            if (paramId == "" || paramValue == "") continue;

            transform.setParameter(paramId, paramValue.toFloat());
        }

        cerr << "RDFTransformFactory: NOTE: Transform is: " << endl;
        cerr << transform.toXmlString().toStdString() << endl;

        transforms.push_back(transform);
    }
        
    return transforms;
}

