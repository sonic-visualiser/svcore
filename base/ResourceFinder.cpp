/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2005-2011 Chris Cannam and the Rosegarden
   development team.
*/

#include "ResourceFinder.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QProcess>
#include <QCoreApplication>

#include <cstdlib>
#include <iostream>

/**
   Resource files may be found in three places:

   * Bundled into the application as Qt4 resources.  These may be
     opened using Qt classes such as QFile, with "fake" file paths
     starting with a colon.  For example ":icons/fileopen.png".

   * Installed with the package, or in the user's equivalent home
     directory location.  For example,

     - on Linux, in /usr/share/<appname> or /usr/local/share/<appname>
     - on Linux, in $HOME/.local/share/<appname>

     - on OS/X, in /Library/Application Support/<appname>
     - on OS/X, in $HOME/Library/Application Support/<appname>

     - on Windows, in %ProgramFiles%/<company>/<appname>
     - on Windows, in (where?) something from http://msdn.microsoft.com/en-us/library/dd378457%28v=vs.85%29.aspx ?

   These locations are searched in reverse order (user-installed
   copies take priority over system-installed copies take priority
   over bundled copies).  Also, /usr/local takes priority over /usr.
*/

QStringList
ResourceFinder::getSystemResourcePrefixList()
{
    // returned in order of priority

    QStringList list;

#ifdef Q_OS_WIN32
    char *programFiles = getenv("ProgramFiles");
    if (programFiles && programFiles[0]) {
        list << QString("%1/%2/%3")
            .arg(programFiles)
            .arg(qApp->organizationName())
            .arg(qApp->applicationName());
    } else {
        list << QString("C:/Program Files/%1/%2")
            .arg(qApp->organizationName())
            .arg(qApp->applicationName());
    }
#else
#ifdef Q_OS_MAC
    list << QString("/Library/Application Support/%1/%2")
        .arg(qApp->organizationName())
        .arg(qApp->applicationName());
#else
    list << QString("/usr/local/share/%1")
        .arg(qApp->applicationName());
    list << QString("/usr/share/%1")
        .arg(qApp->applicationName());
#endif
#endif    

    return list;
}

QString
ResourceFinder::getUserResourcePrefix()
{
#ifdef Q_OS_WIN32
    char *homedrive = getenv("HOMEDRIVE");
    char *homepath = getenv("HOMEPATH");
    QString home;
    if (homedrive && homepath) {
        home = QString("%1%2").arg(homedrive).arg(homepath);
    } else {
        home = QDir::home().absolutePath();
    }
    if (home == "") return "";
    return QString("%1/.%2").arg(home).arg(qApp->applicationName()); //!!! wrong
#else
    char *home = getenv("HOME");
    if (!home || !home[0]) return "";
#ifdef Q_OS_MAC
    return QString("%1/Library/Application Support/%2/%3")
        .arg(home)
        .arg(qApp->organizationName())
        .arg(qApp->applicationName());
#else
    return QString("%1/.local/share/%2")
        .arg(home)
        .arg(qApp->applicationName());
#endif
#endif    
}

QStringList
ResourceFinder::getResourcePrefixList()
{
    // returned in order of priority

    QStringList list;

    QString user = getUserResourcePrefix();
    if (user != "") list << user;

    list << getSystemResourcePrefixList();

    list << ":"; // bundled resource location

    return list;
}

QString
ResourceFinder::getResourcePath(QString resourceCat, QString fileName)
{
    // We don't simply call getResourceDir here, because that returns
    // only the "installed file" location.  We also want to search the
    // bundled resources and user-saved files.

    QStringList prefixes = getResourcePrefixList();
    
    if (resourceCat != "") resourceCat = "/" + resourceCat;

    for (QStringList::const_iterator i = prefixes.begin();
         i != prefixes.end(); ++i) {
        
        QString prefix = *i;

        SVDEBUG << "ResourceFinder::getResourcePath: Looking up file \"" << fileName << "\" for category \"" << resourceCat << "\" in prefix \"" << prefix << "\"" << endl;

        QString path =
            QString("%1%2/%3").arg(prefix).arg(resourceCat).arg(fileName);
        if (QFileInfo(path).exists() && QFileInfo(path).isReadable()) {
            std::cerr << "Found it!" << std::endl;
            return path;
        }
    }

    return "";
}

QString
ResourceFinder::getResourceDir(QString resourceCat)
{
    // Returns only the "installed file" location

    QStringList prefixes = getSystemResourcePrefixList();
    
    if (resourceCat != "") resourceCat = "/" + resourceCat;

    for (QStringList::const_iterator i = prefixes.begin();
         i != prefixes.end(); ++i) {
        
        QString prefix = *i;
        QString path = QString("%1%2").arg(prefix).arg(resourceCat);
        if (QFileInfo(path).exists() &&
            QFileInfo(path).isDir() &&
            QFileInfo(path).isReadable()) {
            return path;
        }
    }

    return "";
}

QString
ResourceFinder::getResourceSavePath(QString resourceCat, QString fileName)
{
    QString dir = getResourceSaveDir(resourceCat);
    if (dir == "") return "";

    return dir + "/" + fileName;
}

QString
ResourceFinder::getResourceSaveDir(QString resourceCat)
{
    // Returns the "user" location

    QString user = getUserResourcePrefix();
    if (user == "") return "";

    if (resourceCat != "") resourceCat = "/" + resourceCat;

    QDir userDir(user);
    if (!userDir.exists()) {
        if (!userDir.mkpath(user)) {
            std::cerr << "ResourceFinder::getResourceSaveDir: ERROR: Failed to create user resource path \"" << user << "\"" << std::endl;
            return "";
        }
    }

    if (resourceCat != "") {
        QString save = QString("%1%2").arg(user).arg(resourceCat);
        QDir saveDir(save);
        if (!saveDir.exists()) {
            if (!userDir.mkpath(save)) {
                std::cerr << "ResourceFinder::getResourceSaveDir: ERROR: Failed to create user resource path \"" << save << "\"" << std::endl;
                return "";
            }
        }
        return save;
    } else {
        return user;
    }
}

QStringList
ResourceFinder::getResourceFiles(QString resourceCat, QString fileExt)
{
    QStringList results;
    QStringList prefixes = getResourcePrefixList();

    QStringList filters;
    filters << QString("*.%1").arg(fileExt);

    for (QStringList::const_iterator i = prefixes.begin();
         i != prefixes.end(); ++i) {
        
        QString prefix = *i;
        QString path;

        if (resourceCat != "") {
            path = QString("%1/%2").arg(prefix).arg(resourceCat);
        } else {
            path = prefix;
        }
        
        QDir dir(path);
        if (!dir.exists()) continue;

        dir.setNameFilters(filters);
        QStringList entries = dir.entryList
            (QDir::Files | QDir::Readable, QDir::Name);
        
        for (QStringList::const_iterator j = entries.begin();
             j != entries.end(); ++j) {
            results << QString("%1/%2").arg(path).arg(*j);
        }
    }

    return results;
}

bool
ResourceFinder::unbundleResource(QString resourceCat, QString fileName)
{
    QString path = getResourcePath(resourceCat, fileName);
    
    if (!path.startsWith(':')) return true;

    // This is the lowest-priority alternative path for this
    // resource, so we know that there must be no installed copy.
    // Install one to the user location.
    SVDEBUG << "ResourceFinder::unbundleResource: File " << fileName << " is bundled, un-bundling it" << endl;
    QString target = getResourceSavePath(resourceCat, fileName);
    QFile file(path);
    if (!file.copy(target)) {
        std::cerr << "ResourceFinder::unbundleResource: ERROR: Failed to un-bundle resource file \"" << fileName << "\" to user location \"" << target << "\"" << std::endl;
        return false;
    }

    QFile chmod(target);
    chmod.setPermissions(QFile::ReadOwner |
                         QFile::ReadUser  | /* for potential platform-independence */
                         QFile::ReadGroup |
                         QFile::ReadOther |
                         QFile::WriteOwner|
                         QFile::WriteUser); /* for potential platform-independence */

    return true;
}

