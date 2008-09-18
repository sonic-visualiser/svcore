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

#ifndef _PLUGIN_RDF_INDEXER_H_
#define _PLUGIN_RDF_INDEXER_H_

#include <QString>
#include <map>
#include <set>

class FileSource;

class PluginRDFIndexer
{
public:
    static PluginRDFIndexer *getInstance();

    QString getURIForPluginId(QString pluginId);
    QString getIdForPluginURI(QString uri);
    QString getDescriptionURLForPluginId(QString pluginId);
    QString getDescriptionURLForPluginURI(QString uri);

    ~PluginRDFIndexer();

protected:
    PluginRDFIndexer();
    typedef std::map<QString, QString> StringMap;
    StringMap m_uriToIdMap;
    StringMap m_idToUriMap;
    StringMap m_idToDescriptionMap;
    bool indexFile(QString path);
    bool indexURL(QString url);
    std::set<FileSource *> m_cache;
    static PluginRDFIndexer *m_instance;
};

#endif

