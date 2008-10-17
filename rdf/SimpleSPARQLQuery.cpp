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

#ifdef USE_NEW_RASQAL_API
#include <rasqal/rasqal.h>
#else
#include <rasqal.h>
#endif

//#define DEBUG_SIMPLE_SPARQL_QUERY 1

#include <iostream>

using std::cerr;
using std::endl;

#ifdef USE_NEW_RASQAL_API
class WrasqalWorldWrapper // wrong but wromantic, etc
{
public:
    WrasqalWorldWrapper() : m_world(rasqal_new_world()) { }
    ~WrasqalWorldWrapper() { rasqal_free_world(m_world); }

    rasqal_world *getWorld() const { return m_world; }

private:
    rasqal_world *m_world;
};
#endif

class SimpleSPARQLQuery::Impl
{
public:
    Impl(QString query);
    ~Impl();

    void setProgressReporter(ProgressReporter *reporter) { m_reporter = reporter; }
    bool wasCancelled() const { return m_cancelled; }

    ResultList execute();

    bool isOK() const;
    QString getErrorString() const;

protected:
    static void errorHandler(void *, raptor_locator *, const char *);

#ifdef USE_NEW_RASQAL_API
    static WrasqalWorldWrapper m_www;
#else
    static bool m_initialised;
#endif
    
    QString m_query;
    QString m_errorString;
    ProgressReporter *m_reporter;
    bool m_cancelled;
};

#ifdef USE_NEW_RASQAL_API
WrasqalWorldWrapper
SimpleSPARQLQuery::Impl::m_www;
#else
bool
SimpleSPARQLQuery::Impl::m_initialised = false;
#endif

SimpleSPARQLQuery::SimpleSPARQLQuery(QString query) :
    m_impl(new Impl(query)) { }

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

SimpleSPARQLQuery::Impl::Impl(QString query) :
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
    
//    char buffer[256];
//    raptor_format_locator(buffer, 255, locator);
//    impl->m_errorString = QString("%1 - %2").arg(buffer).arg(message);

    impl->m_errorString = message;

    cerr << "SimpleSPARQLQuery: ERROR: " << impl->m_errorString.toStdString() << endl;
}

SimpleSPARQLQuery::ResultList
SimpleSPARQLQuery::Impl::execute()
{
    ResultList list;

#ifdef USE_NEW_RASQAL_API
    rasqal_query *query = rasqal_new_query(m_www.getWorld(), "sparql", NULL);
#else
    if (!m_initialised) {
        m_initialised = true;
        rasqal_init();
    }
    rasqal_query *query = rasqal_new_query("sparql", NULL);
#endif
    if (!query) {
        m_errorString = "Failed to construct query";
        cerr << "SimpleSPARQLQuery: ERROR: " << m_errorString.toStdString() << endl;
        return list;
    }

    rasqal_query_set_error_handler(query, this, errorHandler);
    rasqal_query_set_fatal_error_handler(query, this, errorHandler);

    if (rasqal_query_prepare
        (query, (const unsigned char *)m_query.toUtf8().data(), NULL)) {
        cerr << "SimpleSPARQLQuery: Failed to prepare query" << endl;
        rasqal_free_query(query);
        return list;
    }

    rasqal_query_results *results = rasqal_query_execute(query);
    
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

            rasqal_literal *literal =
                rasqal_query_results_get_binding_value(results, i);

            QString key = (const char *)name;

            if (!literal) {
                resultmap[key] = Value();
                continue;
            }

            ValueType type = LiteralValue;
            if (literal->type == RASQAL_LITERAL_URI) type = URIValue;
            else if (literal->type == RASQAL_LITERAL_BLANK) type = BlankValue;

            QString text = (const char *)rasqal_literal_as_string(literal);

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

SimpleSPARQLQuery::Value
SimpleSPARQLQuery::singleResultQuery(QString query, QString binding)
{
    SimpleSPARQLQuery q(query);
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



