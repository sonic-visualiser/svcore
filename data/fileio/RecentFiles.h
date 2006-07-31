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

#ifndef _RECENT_FILES_H_
#define _RECENT_FILES_H_

#include <QObject>
#include <QString>
#include <vector>
#include <deque>

class RecentFiles : public QObject
{
    Q_OBJECT

public:
    // The maxFileCount argument will only be used the first time this is called
    static RecentFiles *getInstance(int maxFileCount = 10);

    virtual ~RecentFiles();

    int getMaxFileCount() const { return m_maxFileCount; }

    std::vector<QString> getRecentFiles() const;
    
    void addFile(QString filename);

signals:
    void recentFilesChanged();

protected:
    RecentFiles(int maxFileCount);

    int m_maxFileCount;
    std::deque<QString> m_files;

    void readFiles();
    void writeFiles();
    void truncateAndWrite();

    static RecentFiles *m_instance;
};

#endif
