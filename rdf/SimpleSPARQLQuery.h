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

#ifndef _SIMPLE_SPARQL_QUERY_H_
#define _SIMPLE_SPARQL_QUERY_H_

#include <QString>
#include <map>
#include <vector>

class ProgressReporter;

class SimpleSPARQLQuery
{
public:
    enum ValueType { NoValue, URIValue, LiteralValue, BlankValue };

    struct Value {
        Value() : type(NoValue), value() { }
        Value(ValueType t, QString v) : type(t), value(v) { }
        ValueType type;
        QString value;
    };

    typedef std::map<QString, Value> KeyValueMap;
    typedef std::vector<KeyValueMap> ResultList;

    SimpleSPARQLQuery(QString fromUri, QString query);
    ~SimpleSPARQLQuery();

    void setProgressReporter(ProgressReporter *reporter);
    bool wasCancelled() const;
    
    ResultList execute();

    bool isOK() const;
    QString getErrorString() const;

    // Do a query and return the value for the given binding, from the
    // first result that has a value for it
    static Value singleResultQuery(QString fromUri,
                                   QString query,
                                   QString binding);

    enum ImplementationPreference {
        UseDirectParser, // rasqal (default because it's simpler if seldom used)
        UseDatastore     // redland
    };
    static void setImplementationPreference(ImplementationPreference);

protected:
    class Impl;
    Impl *m_impl;
};

#endif
