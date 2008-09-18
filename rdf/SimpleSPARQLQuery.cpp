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

#include <rasqal.h>

#include <iostream>

using std::cerr;
using std::endl;

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

    static bool m_initialised;
    
    QString m_query;
    QString m_errorString;
    ProgressReporter *m_reporter;
    bool m_cancelled;
};

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

bool
SimpleSPARQLQuery::Impl::m_initialised = false;

SimpleSPARQLQuery::Impl::Impl(QString query) :
    m_query(query),
    m_reporter(0),
    m_cancelled(false)
{
    //!!! fortunately this global stuff goes away in future rasqal versions
    if (!m_initialised) {
        rasqal_init();
    }
}

SimpleSPARQLQuery::Impl::~Impl()
{
//!!!    rasqal_finish();
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

    rasqal_query *query = rasqal_new_query("sparql", NULL);
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
    
