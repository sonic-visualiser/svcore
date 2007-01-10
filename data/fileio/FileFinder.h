/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2007 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _FILE_FINDER_H_
#define _FILE_FINDER_H_

#include <QString>

class FileFinder
{
public:
    /**
     * Find a file.
     *
     * "location" is what we know about where the file is supposed to
     * be: it may be a relative path, an absolute path, a URL, or just
     * a filename.
     *
     * "lastKnownLocation", if provided, is a path or URL of something
     * that can be used as a reference point to locate it -- for
     * example, the location of the session file that is referring to
     * the file we're looking for.
     */
    FileFinder(QString location, QString lastKnownLocation = "");
    virtual ~FileFinder();

    QString getLocation();

protected:
    QString m_location;
    QString m_lastKnownLocation;
    QString m_lastLocatedLocation;
};

#endif

