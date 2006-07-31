/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "RecentFiles.h"
#include "ConfigFile.h"

#include "base/Preferences.h"

#include <QFileInfo>

RecentFiles *
RecentFiles::m_instance = 0;

RecentFiles *
RecentFiles::getInstance(int maxFileCount)
{
    if (!m_instance) {
        m_instance = new RecentFiles(maxFileCount);
    }
    return m_instance;
}

RecentFiles::RecentFiles(int maxFileCount) :
    m_maxFileCount(maxFileCount)
{
    readFiles();
}

RecentFiles::~RecentFiles()
{
    // nothing
}

void
RecentFiles::readFiles()
{
    m_files.clear();
    ConfigFile *cf = Preferences::getInstance()->getConfigFile();
    for (unsigned int i = 0; i < 100; ++i) {
        QString key = QString("recent-file-%1").arg(i);
        QString filename = cf->get(key);
        if (filename == "") break;
        if (i < m_maxFileCount) m_files.push_back(filename);
        else cf->set(key, "");
    }
    cf->commit();
}

void
RecentFiles::writeFiles()
{
    ConfigFile *cf = Preferences::getInstance()->getConfigFile();
    for (unsigned int i = 0; i < m_maxFileCount; ++i) {
        QString key = QString("recent-file-%1").arg(i);
        QString filename = "";
        if (i < m_files.size()) filename = m_files[i];
        cf->set(key, filename);
    }
    cf->commit();
}

void
RecentFiles::truncateAndWrite()
{
    while (m_files.size() > m_maxFileCount) {
        m_files.pop_back();
    }
    writeFiles();
}

std::vector<QString>
RecentFiles::getRecentFiles() const
{
    std::vector<QString> files;
    for (unsigned int i = 0; i < m_maxFileCount; ++i) {
        if (i < m_files.size()) {
            files.push_back(m_files[i]);
        }
    }
    return files;
}

void
RecentFiles::addFile(QString filename)
{
    filename = QFileInfo(filename).absoluteFilePath();

    bool have = false;
    for (unsigned int i = 0; i < m_files.size(); ++i) {
        if (m_files[i] == filename) {
            have = true;
            break;
        }
    }
    
    if (!have) {
        m_files.push_front(filename);
    } else {
        std::deque<QString> newfiles;
        newfiles.push_back(filename);
        for (unsigned int i = 0; i < m_files.size(); ++i) {
            if (m_files[i] == filename) continue;
            newfiles.push_back(m_files[i]);
        }
    }

    truncateAndWrite();
    emit recentFilesChanged();
}


