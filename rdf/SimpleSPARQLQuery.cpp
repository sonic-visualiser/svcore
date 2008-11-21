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

#include "SimpleSPARQLQuery.h"
#include "base/ProgressReporter.h"
#include "base/Profiler.h"

#include <QMutex>
#include <QMutexLocker>
#include <QRegExp>

#include <set>

#ifdef USE_NEW_RASQAL_API
#include <rasqal/rasqal.h>
#else
#include <rasqal.h>
#endif

#include <redland.h>

#define DEBUG_SIMPLE_SPARQL_QUERY 1

#include <iostream>

using std::cerr;
using std::endl;

#ifdef USE_NEW_RASQAL_API
class WrasqalWorldWrapper // wrong but wromantic, etc
{
public:
    WrasqalWorldWrapper() :
        m_world(0)
    {
        m_world = rasqal_new_world();
        if (!m_world) {
            cerr << "SimpleSPARQLQuery: ERROR: Failed to create RASQAL world!" << endl;
            return;
        }
/*!!! This appears to be new for 0.9.17?
        if (rasqal_world_open(m_world)) {
            cerr << "SimpleSPARQLQuery: ERROR: Failed to open RASQAL world!" << endl;
            return;
        }
*/
    }
    ~WrasqalWorldWrapper()
    {
        rasqal_free_world(m_world);
    }

    bool isOK() const { return (m_world != 0); }

    rasqal_world *getWorld() { return m_world; }
    const rasqal_world *getWorld() const { return m_world; }

private:
    rasqal_world *m_world;
};
#endif

class WredlandWorldWrapper
{
public:
    WredlandWorldWrapper() :
        m_world(0), m_storage(0), m_model(0)
    {
        m_world = librdf_new_world();
        if (!m_world) {
            cerr << "SimpleSPARQLQuery: ERROR: Failed to create LIBRDF world!" << endl;
            return;
        }
        librdf_world_open(m_world);
        m_storage = librdf_new_storage(m_world, "trees", NULL, NULL);
        if (!m_storage) {
            std::cerr << "SimpleSPARQLQuery: ERROR: Failed to initialise Redland trees datastore, falling back to memory store" << std::endl;
            m_storage = librdf_new_storage(m_world, NULL, NULL, NULL);
            if (!m_storage) {
                std::cerr << "SimpleSPARQLQuery: ERROR: Failed to initialise Redland memory datastore" << std::endl;
                return;
            }                
        }
        m_model = librdf_new_model(m_world, m_storage, NULL);
        if (!m_model) {
            std::cerr << "SimpleSPARQLQuery: ERROR: Failed to initialise Redland data model" << std::endl;
            return;
        }
    }

    ~WredlandWorldWrapper()
    {
        while (!m_parsedUris.empty()) {
            librdf_free_uri(m_parsedUris.begin()->second);
            m_parsedUris.erase(m_parsedUris.begin());
        }
        if (m_model) librdf_free_model(m_model);
        if (m_storage) librdf_free_storage(m_storage);
        if (m_world) librdf_free_world(m_world);
    }

    bool isOK() const { return (m_model != 0); }

    librdf_uri *getUri(QString uriString, QString &errorString)
    {
        if (m_parsedUris.find(uriString) != m_parsedUris.end()) {
            return m_parsedUris[uriString];
        }

        librdf_uri *uri = librdf_new_uri
            (m_world, (const unsigned char *)uriString.toUtf8().data());
        if (!uri) {
            errorString = "Failed to construct librdf_uri!";
            return 0;
        }

        librdf_parser *parser = librdf_new_parser(m_world, "guess", NULL, NULL);
        if (!parser) {
            errorString = "Failed to initialise Redland parser";
            return 0;
        }
        
        std::cerr << "About to parse \"" << uriString.toStdString() << "\"" << std::endl;

        Profiler p("SimpleSPARQLQuery: Parse URI into LIBRDF model");

        if (librdf_parser_parse_into_model(parser, uri, NULL, m_model)) {

            errorString = QString("Failed to parse RDF from URI \"%1\"")
                .arg(uriString);
            librdf_free_parser(parser);
//            librdf_free_uri(uri);
            return 0;

        } else {

            librdf_free_parser(parser);
            m_parsedUris[uriString] = uri;
            return uri;
        }
    }
        
    librdf_world *getWorld() { return m_world; }
    const librdf_world *getWorld() const { return m_world; }
        
    librdf_model *getModel() { return m_model; }
    const librdf_model *getModel() const { return m_model; }

private:
    librdf_world *m_world;
    librdf_storage *m_storage;
    librdf_model *m_model;

    QMutex m_mutex;
    std::map<QString, librdf_uri *> m_parsedUris;
};

class SimpleSPARQLQuery::Impl
{
public:
    Impl(SimpleSPARQLQuery::QueryType, QString query);
    ~Impl();

    static bool addSourceToModel(QString sourceUri);

    void setProgressReporter(ProgressReporter *reporter) { m_reporter = reporter; }
    bool wasCancelled() const { return m_cancelled; }

    ResultList execute();

    bool isOK() const;
    QString getErrorString() const;

protected:
    static void errorHandler(void *, raptor_locator *, const char *);

    static QMutex m_mutex;

#ifdef USE_NEW_RASQAL_API
    static WrasqalWorldWrapper *m_rasqal;
#else
    static bool m_rasqalInitialised;
#endif

    static WredlandWorldWrapper *m_redland;

    ResultList executeDirectParser();
    ResultList executeDatastore();

    QueryType m_type;
    QString m_query;
    QString m_errorString;
    ProgressReporter *m_reporter;
    bool m_cancelled;
};

#ifdef USE_NEW_RASQAL_API
WrasqalWorldWrapper *SimpleSPARQLQuery::Impl::m_rasqal = 0;
#else
bool SimpleSPARQLQuery::Impl::m_rasqalInitialised = false;
#endif

WredlandWorldWrapper *SimpleSPARQLQuery::Impl::m_redland = 0;

QMutex SimpleSPARQLQuery::Impl::m_mutex;

SimpleSPARQLQuery::SimpleSPARQLQuery(QueryType type, QString query) :
    m_impl(new Impl(type, query))
{
}

SimpleSPARQLQuery::~SimpleSPARQLQuery() 
{
    delete m_impl;
}

void
SimpleSPARQLQuery::setProgressReporter(ProgressReporter *reporter)
{
    m_impl->setProgressReporter(reporter);
}

bool
SimpleSPARQLQuery::wasCancelled() const
{
    return m_impl->wasCancelled();
}

SimpleSPARQLQuery::ResultList
SimpleSPARQLQuery::execute()
{
    return m_impl->execute();
}

bool
SimpleSPARQLQuery::isOK() const
{
    return m_impl->isOK();
}

QString
SimpleSPARQLQuery::getErrorString() const
{
    return m_impl->getErrorString();
}

bool
SimpleSPARQLQuery::addSourceToModel(QString sourceUri)
{
    return SimpleSPARQLQuery::Impl::addSourceToModel(sourceUri);
}

SimpleSPARQLQuery::Impl::Impl(QueryType type, QString query) :
    m_type(type),
    m_query(query),
    m_reporter(0),
    m_cancelled(false)
{
#ifdef DEBUG_SIMPLE_SPARQL_QUERY
    std::cerr << "SimpleSPARQLQuery::Impl: Query is: \"" << query.toStdString() << "\"" << std::endl;
#endif
}

SimpleSPARQLQuery::Impl::~Impl()
{
}

bool
SimpleSPARQLQuery::Impl::isOK() const
{
    return (m_errorString == "");
}

QString
SimpleSPARQLQuery::Impl::getErrorString() const
{
    return m_errorString;
}

void
SimpleSPARQLQuery::Impl::errorHandler(void *data, 
                                      raptor_locator *locator,
                                      const char *message) 
{
    SimpleSPARQLQuery::Impl *impl = (SimpleSPARQLQuery::Impl *)data;
    
    char buffer[256];
    raptor_format_locator(buffer, 255, locator);
    QString loc(buffer);
    if (loc != "") {
        impl->m_errorString = QString("%1 - %2").arg(loc).arg(message);
    } else {
        impl->m_errorString = message;
    }

    cerr << "SimpleSPARQLQuery: ERROR: " << impl->m_errorString.toStdString() << endl;
}

SimpleSPARQLQuery::ResultList
SimpleSPARQLQuery::Impl::execute()
{
    ResultList list;

    QMutexLocker locker(&m_mutex);

    if (m_type == QueryFromModel) {
        if (!m_redland) {
            // There can be no results, because no sources have been
            // added to the model yet (m_redland is only created when
            // addSourceToModel is called)
            cerr << "SimpleSPARQLQuery::execute: NOTE: No sources have been added to data model yet, so no results are possible" << endl;
            return list;
        }
    }

    if (m_type == QueryFromSingleSource) {
#ifdef USE_NEW_RASQAL_API
        if (!m_rasqal) {
            m_rasqal = new WrasqalWorldWrapper();
            if (!m_rasqal->isOK()) {
                cerr << "ERROR: SimpleSPARQLQuery::execute: Failed to initialise Rasqal query engine" << endl;
                delete m_rasqal;
                m_rasqal = 0;
                return list;
            }
        }
#else
        if (!m_rasqalInitialised) {
            rasqal_init();
            m_rasqalInitialised = true;
        }
#endif
    }

    if (m_type == QueryFromSingleSource) {
        return executeDirectParser();
    } else {
        return executeDatastore();
    }
}

SimpleSPARQLQuery::ResultList
SimpleSPARQLQuery::Impl::executeDirectParser()
{
    ResultList list;

    Profiler profiler("SimpleSPARQLQuery::executeDirectParser");

#ifdef USE_NEW_RASQAL_API
    rasqal_query *query = rasqal_new_query(m_rasqal->getWorld(), "sparql", NULL);
#else
    rasqal_query *query = rasqal_new_query("sparql", NULL);
#endif
    if (!query) {
        m_errorString = "Failed to construct query";
        cerr << "SimpleSPARQLQuery: ERROR: " << m_errorString.toStdString() << endl;
        return list;
    }

    rasqal_query_set_error_handler(query, this, errorHandler);
    rasqal_query_set_fatal_error_handler(query, this, errorHandler);

    {
        Profiler p("SimpleSPARQLQuery: Prepare RASQAL query");

        if (rasqal_query_prepare
            (query, (const unsigned char *)m_query.toUtf8().data(), NULL)) {
            cerr << "SimpleSPARQLQuery: Failed to prepare query" << endl;
            rasqal_free_query(query);
            return list;
        }
    }

    rasqal_query_results *results;
    
    {
        Profiler p("SimpleSPARQLQuery: Execute RASQAL query");
        results = rasqal_query_execute(query);
    }
    
//    cerr << "Query executed" << endl;

    if (!results) {
        cerr << "SimpleSPARQLQuery: RASQAL query failed" << endl;
        rasqal_free_query(query);
        return list;
    }

    if (!rasqal_query_results_is_bindings(results)) {
        cerr << "SimpleSPARQLQuery: RASQAL query has wrong result type (not bindings)" << endl;
        rasqal_free_query_results(results);
        rasqal_free_query(query);
        return list;
    }
    
    int resultCount = 0;
    int resultTotal = rasqal_query_results_get_count(results); // probably wrong
    m_cancelled = false;

    while (!rasqal_query_results_finished(results)) {

        int count = rasqal_query_results_get_bindings_count(results);

        KeyValueMap resultmap;

        for (int i = 0; i < count; ++i) {

            const unsigned char *name =
                rasqal_query_results_get_binding_name(results, i);

            if (!name) {
                std::cerr << "WARNING: Result " << i << " of query has no name" << std::endl;
                continue;
            }

            rasqal_literal *literal =
                rasqal_query_results_get_binding_value(results, i);

            QString key = (const char *)name;

            if (!literal) {
                resultmap[key] = Value();
                continue;
            }

            ValueType type = LiteralValue;
            if (literal->type == RASQAL_LITERAL_BLANK) type = BlankValue;
            else if (literal->type == RASQAL_LITERAL_URI) type = URIValue;

            QString text;
            const char *lit = (const char *)rasqal_literal_as_string(literal);
            if (!lit) {
                std::cerr << "WARNING: Result " << i << " of query has null value" << std::endl;
            } else {
                text = lit;
            }

#ifdef DEBUG_SIMPLE_SPARQL_QUERY
            std::cerr << i << ". " << key.toStdString() << " -> " << text.toStdString() << " (type " << type << ")" << std::endl;
#endif

            resultmap[key] = Value(type, text);
        }

        list.push_back(resultmap);

        rasqal_query_results_next(results);

        resultCount++;

        if (m_reporter) {
            if (resultCount >= resultTotal) {
                if (m_reporter->isDefinite()) m_reporter->setDefinite(false);
                m_reporter->setProgress(resultCount);
            } else {
                m_reporter->setProgress((resultCount * 100) / resultTotal);
            }

            if (m_reporter->wasCancelled()) {
                m_cancelled = true;
                break;
            }
        }
    }

    rasqal_free_query_results(results);
    rasqal_free_query(query);

    return list;
}

SimpleSPARQLQuery::ResultList
SimpleSPARQLQuery::Impl::executeDatastore()
{
    ResultList list;

    Profiler profiler("SimpleSPARQLQuery::executeDatastore");

/*!!!
    static std::map<QString, int> counter;
    if (counter.find(m_query) == counter.end()) counter[m_query] = 1;
    else ++counter[m_query];
    std::cerr << "Counter for this query: " << counter[m_query] << std::endl;
    std::cerr << "Base URI is: \"" << m_fromUri.toStdString() << "\"" << std::endl;
*/

    librdf_query *query;

    {
        Profiler p("SimpleSPARQLQuery: Prepare LIBRDF query");
        query = librdf_new_query
            (m_redland->getWorld(), "sparql", NULL,
             (const unsigned char *)m_query.toUtf8().data(), NULL);
    }
    
    if (!query) {
        m_errorString = "Failed to construct query";
        return list;
    }

    librdf_query_results *results;
    {
        Profiler p("SimpleSPARQLQuery: Execute LIBRDF query");
        results = librdf_query_execute(query, m_redland->getModel());
    }

    if (!results) {
        cerr << "SimpleSPARQLQuery: LIBRDF query failed" << endl;
        librdf_free_query(query);
        return list;
    }

    if (!librdf_query_results_is_bindings(results)) {
        cerr << "SimpleSPARQLQuery: LIBRDF query has wrong result type (not bindings)" << endl;
        librdf_free_query_results(results);
        librdf_free_query(query);
        return list;
    }
    
    int resultCount = 0;
    int resultTotal = librdf_query_results_get_count(results); // probably wrong
    m_cancelled = false;

    while (!librdf_query_results_finished(results)) {

        int count = librdf_query_results_get_bindings_count(results);

        KeyValueMap resultmap;

        for (int i = 0; i < count; ++i) {

            const char *name =
                librdf_query_results_get_binding_name(results, i);

            if (!name) {
                std::cerr << "WARNING: Result " << i << " of query has no name" << std::endl;
                continue;
            }

            librdf_node *node =
                librdf_query_results_get_binding_value(results, i);

            QString key = (const char *)name;

            if (!node) {
                resultmap[key] = Value();
                continue;
            }

            ValueType type = LiteralValue;
            QString text;

            if (librdf_node_is_resource(node)) {

                type = URIValue;
                librdf_uri *uri = librdf_node_get_uri(node);
                const char *us = (const char *)librdf_uri_as_string(uri);

                if (!us) {
                    std::cerr << "WARNING: Result " << i << " of query claims URI type, but has null URI" << std::endl;
                } else {
                    text = us;
                }

            } else if (librdf_node_is_literal(node)) {

                type = LiteralValue;

                const char *lit = (const char *)librdf_node_get_literal_value(node);
                if (!lit) {
                    std::cerr << "WARNING: Result " << i << " of query claims literal type, but has no literal" << std::endl;
                } else {
                    text = lit;
                }

            } else if (librdf_node_is_blank(node)) {

                type = BlankValue;

            } else {

                cerr << "SimpleSPARQLQuery: LIBRDF query returned unknown node type (not resource, literal, or blank)" << endl;
            }

#ifdef DEBUG_SIMPLE_SPARQL_QUERY
            cerr << i << ". " << key.toStdString() << " -> " << text.toStdString() << " (type " << type << ")" << endl;
#endif

            resultmap[key] = Value(type, text);

            librdf_free_node(node);
        }

        list.push_back(resultmap);

        librdf_query_results_next(results);

        resultCount++;

        if (m_reporter) {
            if (resultCount >= resultTotal) {
                if (m_reporter->isDefinite()) m_reporter->setDefinite(false);
                m_reporter->setProgress(resultCount);
            } else {
                m_reporter->setProgress((resultCount * 100) / resultTotal);
            }

            if (m_reporter->wasCancelled()) {
                m_cancelled = true;
                break;
            }
        }
    }

    librdf_free_query_results(results);
    librdf_free_query(query);

#ifdef DEBUG_SIMPLE_SPARQL_QUERY
    cerr << "All results retrieved (" << resultCount << " of them)" << endl;
#endif

    return list;
}

bool
SimpleSPARQLQuery::Impl::addSourceToModel(QString sourceUri)
{
    QString err;

    QMutexLocker locker(&m_mutex);

    if (!m_redland) {
        m_redland = new WredlandWorldWrapper();
        if (!m_redland->isOK()) {
            cerr << "ERROR: SimpleSPARQLQuery::addSourceToModel: Failed to initialise Redland datastore" << endl;
            delete m_redland;
            m_redland = 0;
            return false;
        }
    }

    librdf_uri *uri = m_redland->getUri(sourceUri, err);

    if (!uri) {
        std::cerr << "SimpleSPARQLQuery::addSourceToModel: Failed to add source URI \"" << sourceUri.toStdString() << ": " << err.toStdString() << std::endl;
        return false;
    }
    return true;
}

SimpleSPARQLQuery::Value
SimpleSPARQLQuery::singleResultQuery(QueryType type,
                                     QString query, QString binding)
{
    SimpleSPARQLQuery q(type, query);
    ResultList results = q.execute();
    if (!q.isOK()) {
        cerr << "SimpleSPARQLQuery::singleResultQuery: ERROR: "
             << q.getErrorString().toStdString() << endl;
        return Value();
    }
    if (results.empty()) {
        return Value();
    }
    for (int i = 0; i < results.size(); ++i) {
        if (results[i].find(binding) != results[i].end() &&
            results[i][binding].type != NoValue) {
            return results[i][binding];
        }
    }
    return Value();
}



