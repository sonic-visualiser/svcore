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
#include <QHttpResponseHeader>

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
    m_lastStatus(0),
    m_done(false),
    m_progressDialog(0),
    m_progressShowTimer(this)
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

        m_ok = true;
        m_http = new QHttp(url.host(), url.port(80));
        connect(m_http, SIGNAL(done(bool)), this, SLOT(done(bool)));
        connect(m_http, SIGNAL(dataReadProgress(int, int)),
                this, SLOT(dataReadProgress(int, int)));
        connect(m_http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
                this, SLOT(httpResponseHeaderReceived(const QHttpResponseHeader &)));
        QString path = url.path();
        std::cerr << "RemoteFile: path is \"" << path.toStdString() << "\"" << std::endl;
        m_http->get(path, m_localFile);

    } else if (scheme == "ftp") {

        m_ok = true;
        m_ftp = new QFtp;
        connect(m_ftp, SIGNAL(done(bool)), this, SLOT(done(bool)));
        connect(m_ftp, SIGNAL(commandFinished(int, bool)),
                this, SLOT(ftpCommandFinished(int, bool)));
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

        m_ftp->login(username, password);

        QString dirpath = url.path().section('/', 0, -2);
        QString filename = url.path().section('/', -1);

        if (dirpath == "") dirpath = "/";
        m_ftp->cd(dirpath);
        m_ftp->get(filename, m_localFile);
    }

    if (m_ok) {
        m_progressDialog = new QProgressDialog(tr("Downloading %1...").arg(url.toString()), tr("Cancel"), 0, 100);
        m_progressDialog->hide();
        connect(&m_progressShowTimer, SIGNAL(timeout()),
                this, SLOT(showProgressDialog()));
        connect(m_progressDialog, SIGNAL(canceled()), this, SLOT(cancelled()));
        m_progressShowTimer.setSingleShot(true);
        m_progressShowTimer.start(2000);
    }
}

RemoteFile::~RemoteFile()
{
    cleanup();
}

void
RemoteFile::cleanup()
{
//    std::cerr << "RemoteFile::cleanup" << std::endl;
    m_done = true;
    if (m_http) {
        delete m_http;
        m_http = 0;
    }
    if (m_ftp) {
        m_ftp->abort();
        m_ftp->deleteLater();
        m_ftp = 0;
    }
    delete m_progressDialog;
    m_progressDialog = 0;
    delete m_localFile;
    m_localFile = 0;
}

bool
RemoteFile::canHandleScheme(QUrl url)
{
    QString scheme = url.scheme().toLower();
    return (scheme == "http" || scheme == "ftp");
}

bool
RemoteFile::isAvailable()
{
    while (m_ok && (!m_done && m_lastStatus == 0)) {
        QApplication::processEvents();
    }
    bool available = true;
    if (!m_ok) available = false;
    else available = (m_lastStatus / 100 == 2);
    std::cerr << "RemoteFile::isAvailable: " << (available ? "yes" : "no")
              << std::endl;
    return available;
}

void
RemoteFile::wait()
{
    while (m_ok && !m_done) {
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
RemoteFile::httpResponseHeaderReceived(const QHttpResponseHeader &resp)
{
    m_lastStatus = resp.statusCode();
    if (m_lastStatus / 100 >= 4) {
        m_errorString = QString("%1 %2")
            .arg(resp.statusCode()).arg(resp.reasonPhrase());
        std::cerr << "RemoteFile::responseHeaderReceived: "
                  << m_errorString.toStdString() << std::endl;
    } else {
        std::cerr << "RemoteFile::responseHeaderReceived: "
                  << m_lastStatus << std::endl;
    }        
}

void
RemoteFile::ftpCommandFinished(int id, bool error)
{
    std::cerr << "RemoteFile::ftpCommandFinished(" << id << ", " << error << ")" << std::endl;

    if (!m_ftp) return;

    QFtp::Command command = m_ftp->currentCommand();

    if (!error) {
        std::cerr << "RemoteFile::ftpCommandFinished: success for command "
                  << command << std::endl;
        return;
    }

    if (command == QFtp::ConnectToHost) {
        m_errorString = tr("Failed to connect to FTP server");
    } else if (command == QFtp::Login) {
        m_errorString = tr("Login failed");
    } else if (command == QFtp::Cd) {
        m_errorString = tr("Failed to change to correct directory");
    } else if (command == QFtp::Get) {
        m_errorString = tr("FTP download aborted");
    }

    m_lastStatus = 400; // for done()
}

void
RemoteFile::dataTransferProgress(qint64 done, qint64 total)
{
    if (!m_progressDialog) return;

    int percent = int((double(done) / double(total)) * 100.0 - 0.1);
    emit progress(percent);

    if (percent > 0) {
        m_progressDialog->setValue(percent);
        m_progressDialog->show();
    }
}

void
RemoteFile::cancelled()
{
    deleteLocalFile();
    m_done = true;
    m_ok = false;
    m_errorString = tr("Download cancelled");
}

void
RemoteFile::done(bool error)
{
    std::cerr << "RemoteFile::done(" << error << ")" << std::endl;

    if (m_done) return;

    emit progress(100);

    if (error) {
        if (m_http) {
            m_errorString = m_http->errorString();
        } else if (m_ftp) {
            m_errorString = m_ftp->errorString();
        }
    }

    if (m_lastStatus / 100 >= 4) {
        error = true;
    }

    cleanup();

    if (!error) {
        QFileInfo fi(m_localFilename);
        if (!fi.exists()) {
            m_errorString = tr("Failed to create local file %1").arg(m_localFilename);
            error = true;
        } else if (fi.size() == 0) {
            m_errorString = tr("File contains no data!");
            error = true;
        }
    }

    if (error) {
        deleteLocalFile();
    }

    m_ok = !error;
    m_done = true;
}

void
RemoteFile::deleteLocalFile()
{
//    std::cerr << "RemoteFile::deleteLocalFile" << std::endl;

    cleanup();

    if (m_localFilename == "") return;

    m_fileCreationMutex.lock();

    if (!QFile(m_localFilename).remove()) {
        std::cerr << "RemoteFile::deleteLocalFile: ERROR: Failed to delete file \"" << m_localFilename.toStdString() << "\"" << std::endl;
    } else {
        m_localFilename = "";
    }

    m_fileCreationMutex.unlock();

    m_done = true;
}

void
RemoteFile::showProgressDialog()
{
    if (m_progressDialog) m_progressDialog->show();
}

QString
RemoteFile::createLocalFile(QUrl url)
{
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
