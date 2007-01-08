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

#include "RemoteFile.h"
#include "base/TempDirectory.h"
#include "base/Exceptions.h"

#include <QHttp>
#include <QFtp>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QProgressDialog>

#include <iostream>

int
RemoteFile::m_count = 0;

QMutex
RemoteFile::m_fileCreationMutex;

RemoteFile::RemoteFile(QUrl url) :
    m_ftp(0),
    m_http(0),
    m_localFile(0),
    m_ok(false),
    m_done(false),
    m_progressDialog(0)
{
    if (!canHandleScheme(url)) {
        std::cerr << "RemoteFile::RemoteFile: ERROR: Unsupported scheme in URL \"" << url.toString().toStdString() << "\"" << std::endl;
        return;
    }

    m_localFilename = createLocalFile(url);
    if (m_localFilename == "") return;
    m_localFile = new QFile(m_localFilename);
    m_localFile->open(QFile::WriteOnly);

    QString scheme = url.scheme().toLower();

    if (scheme == "http") {

        m_http = new QHttp(url.host(), url.port(80));
        connect(m_http, SIGNAL(done(bool)), this, SLOT(done(bool)));
        connect(m_http, SIGNAL(dataReadProgress(int, int)),
                this, SLOT(dataReadProgress(int, int)));
        m_http->get(url.path(), m_localFile);
        m_ok = true;

    } else if (scheme == "ftp") {

        m_ftp = new QFtp;
        connect(m_ftp, SIGNAL(done(bool)), this, SLOT(done(bool)));
        connect(m_ftp, SIGNAL(dataTransferProgress(qint64, qint64)),
                this, SLOT(dataTransferProgress(qint64, qint64)));
        m_ftp->connectToHost(url.host(), url.port(21));

        QString username = url.userName();
        if (username == "") {
            username = "anonymous";
        }

        QString password = url.password();
        if (password == "") {
            password = QString("%1@%2").arg(getenv("USER")).arg(getenv("HOST"));
        }

        QStringList path = url.path().split('/');
        for (QStringList::iterator i = path.begin(); i != path.end(); ) {
            QString bit = *i;
            ++i;
            if (i != path.end()) {
                m_ftp->cd(*i);
            } else {
                m_ftp->get(*i, m_localFile);
            }
        }

        m_ok = true;
    }

    if (m_ok) {
        m_progressDialog = new QProgressDialog(tr("Downloading %1...").arg(url.toString()), tr("Cancel"), 0, 100);
        m_progressDialog->show();
    }
}

RemoteFile::~RemoteFile()
{
    delete m_ftp;
    delete m_http;
    delete m_localFile;
    delete m_progressDialog;
}

bool
RemoteFile::canHandleScheme(QUrl url)
{
    QString scheme = url.scheme().toLower();
    return (scheme == "http" || scheme == "ftp");
}

void
RemoteFile::wait()
{
    while (!m_done) {
        QApplication::processEvents();
    }
}

bool
RemoteFile::isOK() const
{
    return m_ok;
}

bool
RemoteFile::isDone() const
{
    return m_done;
}

QString
RemoteFile::getLocalFilename() const
{
    return m_localFilename;
}

QString
RemoteFile::getErrorString() const
{
    return m_errorString;
}

void
RemoteFile::dataReadProgress(int done, int total)
{
    dataTransferProgress(done, total);
}

void
RemoteFile::dataTransferProgress(qint64 done, qint64 total)
{
    int percent = int((double(done) / double(total)) * 100.0 - 0.1);
    emit progress(percent);

    m_progressDialog->setValue(percent);
}

void
RemoteFile::done(bool error)
{
    //!!! need to identify 404s etc in the return headers

    emit progress(100);
    m_ok = !error;
    if (error) {
        if (m_http) {
            m_errorString = m_http->errorString();
        } else if (m_ftp) {
            m_errorString = m_ftp->errorString();
        }
    }

    delete m_localFile;
    m_localFile = 0;

    delete m_progressDialog;
    m_progressDialog = 0;

    if (m_ok) {
        QFileInfo fi(m_localFilename);
        if (!fi.exists()) {
            m_errorString = tr("Failed to create local file %1").arg(m_localFilename);
            m_ok = false;
        } else if (fi.size() == 0) {
            m_errorString = tr("File contains no data!");
            m_ok = false;
        }
    }
    m_done = true;
}

QString
RemoteFile::createLocalFile(QUrl url)
{
    //!!! should we actually put up dialogs for these errors? or propagate an exception?
    
    QDir dir;
    try {
        dir = TempDirectory::getInstance()->getSubDirectoryPath("download");
    } catch (DirectoryCreationFailed f) {
        std::cerr << "RemoteFile::createLocalFile: ERROR: Failed to create temporary directory: " << f.what() << std::endl;
        return "";
    }

    QString filepart = url.path().section('/', -1, -1,
                                          QString::SectionSkipEmpty);

    QString extension = filepart.section('.', -1);
    QString base = filepart;
    if (extension != "") {
        base = base.left(base.length() - extension.length() - 1);
    }
    if (base == "") base = "remote";

    QString filename;

    if (extension == "") {
        filename = base;
    } else {
        filename = QString("%1.%2").arg(base).arg(extension);
    }

    QString filepath(dir.filePath(filename));

    std::cerr << "RemoteFile::createLocalFile: URL is \"" << url.toString().toStdString() << "\", dir is \"" << dir.path().toStdString() << "\", base \"" << base.toStdString() << "\", extension \"" << extension.toStdString() << "\", filebase \"" << filename.toStdString() << "\", filename \"" << filepath.toStdString() << "\"" << std::endl;

    m_fileCreationMutex.lock();
    ++m_count;

    if (QFileInfo(filepath).exists() ||
        !QFile(filepath).open(QFile::WriteOnly)) {

        std::cerr << "RemoteFile::createLocalFile: Failed to create local file \""
                  << filepath.toStdString() << "\" for URL \""
                  << url.toString().toStdString() << "\" (or file already exists): appending suffix instead" << std::endl;


        if (extension == "") {
            filename = QString("%1_%2").arg(base).arg(m_count);
        } else {
            filename = QString("%1_%2.%3").arg(base).arg(m_count).arg(extension);
        }
        filepath = dir.filePath(filename);

        if (QFileInfo(filepath).exists() ||
            !QFile(filepath).open(QFile::WriteOnly)) {

            std::cerr << "RemoteFile::createLocalFile: ERROR: Failed to create local file \""
                      << filepath.toStdString() << "\" for URL \""
                      << url.toString().toStdString() << "\" (or file already exists)" << std::endl;

            m_fileCreationMutex.unlock();
            return "";
        }
    }

    m_fileCreationMutex.unlock();

    std::cerr << "RemoteFile::createLocalFile: url "
              << url.toString().toStdString() << " -> local filename "
              << filepath.toStdString() << std::endl;

    return filepath;
}
