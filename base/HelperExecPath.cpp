/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2016 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "HelperExecPath.h"

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QFileInfo>

static QStringList
getSuffixes()
{
    if (sizeof(void *) == 8) {
        return { "-64", "", "-32" };
    } else {
        return { "", "-32" };
    }
}

static bool
isGood(QString path)
{
    return QFile(path).exists() && QFileInfo(path).isExecutable();
}

QStringList
HelperExecPath::getHelperExecutables(QString basename)
{
    QStringList dummy;
    return search(basename, dummy);
}

QString
HelperExecPath::getHelperExecutable(QString basename)
{
    QStringList execs = getHelperExecutables(basename);
    if (execs.empty()) return "";
    else return execs[0];
}

QStringList
HelperExecPath::getHelperDirPaths()
{
    QStringList dirs;
    QString myDir = QCoreApplication::applicationDirPath();
    dirs.push_back(myDir + "/helpers");
    dirs.push_back(myDir);
    return dirs;
}

QStringList
HelperExecPath::getHelperCandidatePaths(QString basename)
{
    QStringList candidates;
    (void)search(basename, candidates);
    return candidates;
}

QStringList
HelperExecPath::search(QString basename, QStringList &candidates)
{
    // Helpers are expected to exist either in the same directory as
    // this executable was found, or in a subdirectory called helpers.

    QString extension = "";
#ifdef _WIN32
    extension = ".exe";
#endif

    QStringList executables;
    QStringList dirs = getHelperDirPaths();
    
    for (QString s: getSuffixes()) {
        for (QString d: dirs) {
            QString path = d + QDir::separator() + basename + s + extension;
            candidates.push_back(path);
            if (isGood(path)) {
                executables.push_back(path);
                break;
            }
        }
    }

    return executables;
}

