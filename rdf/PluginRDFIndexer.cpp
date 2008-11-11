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

#include "PluginRDFIndexer.h"

#include "SimpleSPARQLQuery.h"

#include "data/fileio/CachedFile.h"
#include "data/fileio/FileSource.h"
#include "data/fileio/PlaylistFileReader.h"
#include "plugin/PluginIdentifier.h"

#include "base/Profiler.h"

#include <vamp-hostsdk/PluginHostAdapter.h>

#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QDateTime>
#include <QSettings>
#include <QFile>

#include <iostream>
using std::cerr;
using std::endl;
using std::vector;
using std::string;
using Vamp::PluginHostAdapter;

PluginRDFIndexer *
PluginRDFIndexer::m_instance = 0;

PluginRDFIndexer *
PluginRDFIndexer::getInstance() 
{
    if (!m_instance) m_instance = new PluginRDFIndexer();
    return m_instance;
}

PluginRDFIndexer::PluginRDFIndexer()
{
    indexInstalledURLs();
}

PluginRDFIndexer::~PluginRDFIndexer()
{
    QMutexLocker locker(&m_mutex);
}

void
PluginRDFIndexer::indexInstalledURLs()
{
    vector<string> paths = PluginHostAdapter::getPluginPath();

    QStringList filters;
    filters << "*.n3";
    filters << "*.N3";
    filters << "*.rdf";
    filters << "*.RDF";

    // Search each Vamp plugin path for a .rdf file that either has
    // name "soname", "soname:label" or "soname/label" plus RDF
    // extension.  Use that order of preference, and prefer n3 over
    // rdf extension.

    for (vector<string>::const_iterator i = paths.begin(); i != paths.end(); ++i) {
        
        QDir dir(i->c_str());
        if (!dir.exists()) continue;

        QStringList entries = dir.entryList
            (filters, QDir::Files | QDir::Readable);

        for (QStringList::const_iterator j = entries.begin();
             j != entries.end(); ++j) {
            QFileInfo fi(dir.filePath(*j));
            indexFile(fi.absoluteFilePath());
        }

        QStringList subdirs = dir.entryList
            (QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Readable);

        for (QStringList::const_iterator j = subdirs.begin();
             j != subdirs.end(); ++j) {
            QDir subdir(dir.filePath(*j));
            if (subdir.exists()) {
                entries = subdir.entryList
                    (filters, QDir::Files | QDir::Readable);
                for (QStringList::const_iterator k = entries.begin();
                     k != entries.end(); ++k) {
                    QFileInfo fi(subdir.filePath(*k));
                    indexFile(fi.absoluteFilePath());
                }
            }
        }
    }
}

bool
PluginRDFIndexer::indexConfiguredURLs()
{
    std::cerr << "PluginRDFIndexer::indexConfiguredURLs" << std::endl;

    QSettings settings;
    settings.beginGroup("RDF");
    
    QString indexKey("rdf-indices");
    QStringList indices = settings.value(indexKey).toStringList();
    
    for (int i = 0; i < indices.size(); ++i) {

        QString index = indices[i];

        std::cerr << "PluginRDFIndexer::indexConfiguredURLs: index url is "
                  << index.toStdString() << std::endl;

        CachedFile cf(index);
        if (!cf.isOK()) continue;

        FileSource indexSource(cf.getLocalFilename());

        PlaylistFileReader reader(indexSource);
        if (!reader.isOK()) continue;

        PlaylistFileReader::Playlist list = reader.load();
        for (PlaylistFileReader::Playlist::const_iterator j = list.begin();
             j != list.end(); ++j) {
            std::cerr << "PluginRDFIndexer::indexConfiguredURLs: url is "
                      << j->toStdString() << std::endl;
            indexURL(*j);
        }
    }

    QString urlListKey("rdf-urls");
    QStringList urls = settings.value(urlListKey).toStringList();

    for (int i = 0; i < urls.size(); ++i) {
        indexURL(urls[i]);
    }
    
    settings.endGroup();
    return true;
}

QString
PluginRDFIndexer::getURIForPluginId(QString pluginId)
{
    QMutexLocker locker(&m_mutex);

    if (m_idToUriMap.find(pluginId) == m_idToUriMap.end()) return "";
    return m_idToUriMap[pluginId];
}

QString
PluginRDFIndexer::getIdForPluginURI(QString uri)
{
    m_mutex.lock();

    if (m_uriToIdMap.find(uri) == m_uriToIdMap.end()) {

        m_mutex.unlock();

        // Haven't found this uri referenced in any document on the
        // local filesystem; try resolving the pre-fragment part of
        // the uri as a document URL and reading that if possible.

        // Because we may want to refer to this document again, we
        // cache it locally if it turns out to exist.

        cerr << "PluginRDFIndexer::getIdForPluginURI: NOTE: Failed to find a local RDF document describing plugin <" << uri.toStdString() << ">: attempting to retrieve one remotely by guesswork" << endl;

        QString baseUrl = QUrl(uri).toString(QUrl::RemoveFragment);

        indexURL(baseUrl);

        m_mutex.lock();

        if (m_uriToIdMap.find(uri) == m_uriToIdMap.end()) {
            m_uriToIdMap[uri] = "";
        }
    }

    QString id = m_uriToIdMap[uri];
    m_mutex.unlock();
    return id;
}

QString
PluginRDFIndexer::getDescriptionURLForPluginId(QString pluginId)
{
    QMutexLocker locker(&m_mutex);

    if (m_idToDescriptionMap.find(pluginId) == m_idToDescriptionMap.end()) return "";
    return m_idToDescriptionMap[pluginId];
}

QString
PluginRDFIndexer::getDescriptionURLForPluginURI(QString uri)
{
    QMutexLocker locker(&m_mutex);

    QString id = getIdForPluginURI(uri);
    if (id == "") return "";
    return getDescriptionURLForPluginId(id);
}

QStringList
PluginRDFIndexer::getIndexedPluginIds() 
{
    QMutexLocker locker(&m_mutex);

    QStringList ids;
    for (StringMap::const_iterator i = m_idToDescriptionMap.begin();
         i != m_idToDescriptionMap.end(); ++i) {
        ids.push_back(i->first);
    }
    return ids;
}

bool
PluginRDFIndexer::indexFile(QString filepath)
{
    QUrl url = QUrl::fromLocalFile(filepath);
    QString urlString = url.toString();
    return indexURL(urlString);
}

bool
PluginRDFIndexer::indexURL(QString urlString)
{
    Profiler profiler("PluginRDFIndexer::indexURL");

    std::cerr << "PluginRDFIndexer::indexURL(" << urlString.toStdString() << ")" << std::endl;

    QMutexLocker locker(&m_mutex);

    QString localString = urlString;

    if (FileSource::isRemote(urlString) &&
        FileSource::canHandleScheme(urlString)) {

        CachedFile cf(urlString);
        if (!cf.isOK()) {
            return false;
        }

        localString = cf.getLocalFilename();
    }

//    cerr << "PluginRDFIndexer::indexURL: url = <" << urlString.toStdString() << ">" << endl;

    SimpleSPARQLQuery query
        (QString
         (
             " PREFIX vamp: <http://purl.org/ontology/vamp/> "

             " SELECT ?plugin ?library_id ?plugin_id "
             " FROM <%1> "

             " WHERE { "
             "   ?plugin a vamp:Plugin . "

             // Make the identifier and library parts optional, so
             // that we can check and report helpfully if one or both
             // is absent instead of just getting no results

             //!!! No -- because of rasqal's inability to correctly
             // handle more than one OPTIONAL graph in a query, let's
             // make identifier compulsory after all
             //"   OPTIONAL { ?plugin vamp:identifier ?plugin_id } . "

             "   ?plugin vamp:identifier ?plugin_id . "

             "   OPTIONAL { "
             "     ?library a vamp:PluginLibrary ; "
             "              vamp:available_plugin ?plugin ; "
             "              vamp:identifier ?library_id "
             "   } "
             " } "
             )
         .arg(localString));

    SimpleSPARQLQuery::ResultList results = query.execute();

    if (!query.isOK()) {
        cerr << "ERROR: PluginRDFIndexer::indexURL: ERROR: Failed to index document at <"
             << urlString.toStdString() << ">: "
             << query.getErrorString().toStdString() << endl;
        return false;
    }

    if (results.empty()) {
        cerr << "PluginRDFIndexer::indexURL: NOTE: Document at <"
             << urlString.toStdString()
             << "> does not describe any vamp:Plugin resources" << endl;
        return false;
    }

    bool foundSomething = false;
    bool addedSomething = false;

    for (SimpleSPARQLQuery::ResultList::iterator i = results.begin();
         i != results.end(); ++i) {

        QString pluginUri = (*i)["plugin"].value;
        QString soname = (*i)["library_id"].value;
        QString identifier = (*i)["plugin_id"].value;

        if (identifier == "") {
            cerr << "PluginRDFIndexer::indexURL: NOTE: Document at <"
                 << urlString.toStdString()
                 << "> fails to define any vamp:identifier for plugin <"
                 << pluginUri.toStdString() << ">"
                 << endl;
            continue;
        }
        if (soname == "") {
            cerr << "PluginRDFIndexer::indexURL: NOTE: Document at <"
                 << urlString.toStdString() << "> does not associate plugin <"
                 << pluginUri.toStdString() << "> with any implementation library"
                 << endl;
            continue;
        }
/*
        cerr << "PluginRDFIndexer::indexURL: Document for plugin \""
             << soname.toStdString() << ":" << identifier.toStdString()
             << "\" (uri <" << pluginUri.toStdString() << ">) is at url <"
             << urlString.toStdString() << ">" << endl;
*/
        QString pluginId = PluginIdentifier::createIdentifier
            ("vamp", soname, identifier);

        foundSomething = true;

        if (m_idToDescriptionMap.find(pluginId) != m_idToDescriptionMap.end()) {
            cerr << "PluginRDFIndexer::indexURL: NOTE: Plugin id \""
                 << pluginId.toStdString() << "\", described in document at <"
                 << urlString.toStdString()
                 << ">, has already been described in document <"
                 << m_idToDescriptionMap[pluginId].toStdString()
                 << ">: ignoring this new description" << endl;
            continue;
        }

        m_idToDescriptionMap[pluginId] = urlString;
        m_idToUriMap[pluginId] = pluginUri;

        addedSomething = true;

        if (pluginUri != "") {
            if (m_uriToIdMap.find(pluginUri) != m_uriToIdMap.end()) {
                cerr << "PluginRDFIndexer::indexURL: WARNING: Found multiple plugins with the same URI:" << endl;
                cerr << "  1. Plugin id \"" << m_uriToIdMap[pluginUri].toStdString() << "\"" << endl;
                cerr << "     described in <" << m_idToDescriptionMap[m_uriToIdMap[pluginUri]].toStdString() << ">" << endl;
                cerr << "  2. Plugin id \"" << pluginId.toStdString() << "\"" << endl;
                cerr << "     described in <" << urlString.toStdString() << ">" << endl;
                cerr << "both claim URI <" << pluginUri.toStdString() << ">" << endl;
            } else {
                m_uriToIdMap[pluginUri] = pluginId;
            }
        }
    }

    if (!foundSomething) {
        cerr << "PluginRDFIndexer::indexURL: NOTE: Document at <"
             << urlString.toStdString()
             << "> does not sufficiently describe any plugins" << endl;
    }
    
    return addedSomething;
}



