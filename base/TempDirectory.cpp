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

#include "TempDirectory.h"

#include <QDir>
#include <QFile>
#include <QMutexLocker>

#include <iostream>
#include <cassert>

TempDirectory *
TempDirectory::m_instance = new TempDirectory;

TempDirectory *
TempDirectory::instance()
{
    return m_instance;
}

TempDirectory::DirectoryCreationFailed::DirectoryCreationFailed(QString directory) throw() :
    m_directory(directory)
{
    std::cerr << "ERROR: Directory creation failed for directory: "
              << directory.toLocal8Bit().data() << std::endl;
}

const char *
TempDirectory::DirectoryCreationFailed::what() const throw()
{
    return QString("Directory creation failed for \"%1\"")
        .arg(m_directory).toLocal8Bit().data();
}

TempDirectory::TempDirectory() :
    m_tmpdir("")
{
}

TempDirectory::~TempDirectory()
{
    std::cerr << "TempDirectory::~TempDirectory" << std::endl;

    cleanup();
}

void
TempDirectory::cleanup()
{
    cleanupDirectory("");
}

QString
TempDirectory::getPath()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_tmpdir != "") return m_tmpdir;

    QDir systemTempDir = QDir::temp();

    // Generate a temporary directory.  Qt4.1 doesn't seem to be able
    // to do this for us, and mkdtemp is not standard.  This method is
    // based on the way glibc does mkdtemp.

    static QString chars =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    QString suffix;
    int padlen = 6, attempts = 100;
    unsigned int r = time(0) ^ getpid();

    for (int i = 0; i < padlen; ++i) {
        suffix += "X";
    }
    
    for (int j = 0; j < attempts; ++j) {

        unsigned int v = r;
        
        for (int i = 0; i < padlen; ++i) {
            suffix[i] = chars[v % 62];
            v /= 62;
        }

        QString candidate = QString("sv_%1").arg(suffix);

        if (QDir::temp().mkpath(candidate)) {
            m_tmpdir = systemTempDir.filePath(candidate);
            break;
        }

        r = r + 7777;
    }

    if (m_tmpdir == "") {
        throw DirectoryCreationFailed(QString("temporary subdirectory in %1")
                                      .arg(systemTempDir.canonicalPath()));
    }

    return m_tmpdir;
}

QString
TempDirectory::getSubDirectoryPath(QString subdir)
{
    QString tmpdirpath = getPath();
    
    QMutexLocker locker(&m_mutex);

    QDir tmpdir(tmpdirpath);
    QFileInfo fi(tmpdir.filePath(subdir));

    if (!fi.exists()) {
        if (!tmpdir.mkdir(subdir)) {
            throw DirectoryCreationFailed(fi.filePath());
        } else {
            return fi.filePath();
        }
    } else if (fi.isDir()) {
        return fi.filePath();
    } else {
        throw DirectoryCreationFailed(fi.filePath());
    }
}

void
TempDirectory::cleanupDirectory(QString tmpdir)
{
    bool isRoot = false;

    if (tmpdir == "") {

        m_mutex.lock();

        isRoot = true;
        tmpdir = m_tmpdir;

        if (tmpdir == "") {
            m_mutex.unlock();
            return;
        }
    }

    QDir dir(tmpdir);
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    for (unsigned int i = 0; i < dir.count(); ++i) {

        QFileInfo fi(dir.filePath(dir[i]));

        if (fi.isDir()) {
            cleanupDirectory(fi.absoluteFilePath());
        } else {
            if (!QFile(fi.absoluteFilePath()).remove()) {
                std::cerr << "WARNING: TempDirectory::cleanup: "
                          << "Failed to unlink file \""
                          << fi.absoluteFilePath().toStdString() << "\""
                          << std::endl;
            }
        }
    }

    QString dirname = dir.dirName();
    if (dirname != "") {
        if (!dir.cdUp()) {
            std::cerr << "WARNING: TempDirectory::cleanup: "
                      << "Failed to cd to parent directory of "
                      << tmpdir.toStdString() << std::endl;
            return;
        }
        if (!dir.rmdir(dirname)) {
            std::cerr << "WARNING: TempDirectory::cleanup: "
                      << "Failed to remove directory "
                      << dirname.toStdString() << std::endl;
        } 
    }

    if (isRoot) {
        m_tmpdir = "";
        m_mutex.unlock();
    }
}
