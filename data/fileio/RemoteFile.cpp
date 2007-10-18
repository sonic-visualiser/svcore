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

RemoteFile::RemoteRefCountMap
RemoteFile::m_refCountMap;

RemoteFile::RemoteLocalMap
RemoteFile::m_remoteLocalMap;

QMutex
RemoteFile::m_mapMutex;

RemoteFile::RemoteFile(QString fileOrUrl, bool showProgress) :
    m_url(fileOrUrl),
    m_ftp(0),
    m_http(0),
    m_localFile(0),
    m_ok(false),
    m_lastStatus(0),
    m_remote(isRemote(fileOrUrl)),
    m_done(false),
    m_leaveLocalFile(false),
    m_progressDialog(0),
    m_progressShowTimer(this),
    m_refCounted(false)
{
    std::cerr << "RemoteFile::RemoteFile(" << fileOrUrl.toStdString() << ")" << std::endl;

    if (!canHandleScheme(m_url)) {
        std::cerr << "RemoteFile::RemoteFile: ERROR: Unsupported scheme in URL \"" << m_url.toString().toStdString() << "\"" << std::endl;
        m_errorString = tr("Unsupported scheme in URL");
        return;
    }

    init(showProgress);

    if (isRemote() &&
        (fileOrUrl.contains('%') ||
         fileOrUrl.contains("--"))) { // for IDNA

        waitForStatus();

        if (!isAvailable()) {
            // The URL was created on the assumption that the string
            // was human-readable.  Let's try again, this time
            // assuming it was already encoded.
            std::cerr << "RemoteFile::RemoteFile: Failed to retrieve URL \""
                      << fileOrUrl.toStdString() 
                      << "\" as human-readable URL; "
                      << "trying again treating it as encoded URL"
                      << std::endl;
            m_url.setEncodedUrl(fileOrUrl.toAscii());
            init(showProgress);
        }
    }
}

RemoteFile::RemoteFile(QUrl url, bool showProgress) :
    m_url(url),
    m_ftp(0),
    m_http(0),
    m_localFile(0),
    m_ok(false),
    m_lastStatus(0),
    m_remote(isRemote(url.toString())),
    m_done(false),
    m_leaveLocalFile(false),
    m_progressDialog(0),
    m_progressShowTimer(this),
    m_refCounted(false)
{
    std::cerr << "RemoteFile::RemoteFile(" << url.toString().toStdString() << ") [as url]" << std::endl;

    if (!canHandleScheme(m_url)) {
        std::cerr << "RemoteFile::RemoteFile: ERROR: Unsupported scheme in URL \"" << m_url.toString().toStdString() << "\"" << std::endl;
        m_errorString = tr("Unsupported scheme in URL");
        return;
    }

    init(showProgress);
}

RemoteFile::RemoteFile(const RemoteFile &rf) :
    QObject(),
    m_url(rf.m_url),
    m_ftp(0),
    m_http(0),
    m_localFile(0),
    m_ok(rf.m_ok),
    m_lastStatus(rf.m_lastStatus),
    m_remote(rf.m_remote),
    m_done(false),
    m_leaveLocalFile(false),
    m_progressDialog(0),
    m_progressShowTimer(0),
    m_refCounted(false)
{
    std::cerr << "RemoteFile::RemoteFile(" << m_url.toString().toStdString() << ") [copy ctor]" << std::endl;

    if (!canHandleScheme(m_url)) {
        std::cerr << "RemoteFile::RemoteFile: ERROR: Unsupported scheme in URL \"" << m_url.toString().toStdString() << "\"" << std::endl;
        m_errorString = tr("Unsupported scheme in URL");
        return;
    }

    if (!isRemote()) {
        m_localFilename = rf.m_localFilename;
    } else {
        QMutexLocker locker(&m_mapMutex);
        std::cerr << "RemoteFile::RemoteFile(copy ctor): ref count is "
                  << m_refCountMap[m_url] << std::endl;
        if (m_refCountMap[m_url] > 0) {
            m_refCountMap[m_url]++;
            std::cerr << "raised it to " << m_refCountMap[m_url] << std::endl;
            m_localFilename = m_remoteLocalMap[m_url];
            m_refCounted = true;
        } else {
            m_ok = false;
            m_lastStatus = 404;
        }
    }

    m_done = true;
}

RemoteFile::~RemoteFile()
{
    std::cerr << "RemoteFile(" << m_url.toString().toStdString() << ")::~RemoteFile" << std::endl;

    cleanup();

    if (isRemote() && !m_leaveLocalFile) deleteCacheFile();
}

void
RemoteFile::init(bool showProgress)
{
    if (!isRemote()) {
        m_localFilename = m_url.toLocalFile();
        m_ok = true;
        if (!QFileInfo(m_localFilename).exists()) {
            m_lastStatus = 404;
        } else {
            m_lastStatus = 200;
        }
        m_done = true;
        return;
    }

    if (createCacheFile()) {
        std::cerr << "RemoteFile::init: Already have this one" << std::endl;
        m_ok = true;
        if (!QFileInfo(m_localFilename).exists()) {
            m_lastStatus = 404;
        } else {
            m_lastStatus = 200;
        }
        m_done = true;
        return;
    }

    if (m_localFilename == "") return;
    m_localFile = new QFile(m_localFilename);
    m_localFile->open(QFile::WriteOnly);

    QString scheme = m_url.scheme().toLower();

    std::cerr << "RemoteFile::init: Don't have local copy of \""
              << m_url.toString().toStdString() << "\", retrieving" << std::endl;

    if (scheme == "http") {
        initHttp();
    } else if (scheme == "ftp") {
        initFtp();
    } else {
        m_remote = false;
        m_ok = false;
    }

    if (m_ok) {
        
        QMutexLocker locker(&m_mapMutex);

        if (m_refCountMap[m_url] > 0) {
            // someone else has been doing the same thing at the same time,
            // but has got there first
            cleanup();
            m_refCountMap[m_url]++;
            std::cerr << "RemoteFile::init: Another RemoteFile has got there first, abandoning our download and using theirs" << std::endl;
            m_localFilename = m_remoteLocalMap[m_url];
            m_refCounted = true;
            m_ok = true;
            if (!QFileInfo(m_localFilename).exists()) {
                m_lastStatus = 404;
            }
            m_done = true;
            return;
        }

        m_remoteLocalMap[m_url] = m_localFilename;
        m_refCountMap[m_url]++;
        m_refCounted = true;

        if (showProgress) {
            m_progressDialog = new QProgressDialog(tr("Downloading %1...").arg(m_url.toString()), tr("Cancel"), 0, 100);
            m_progressDialog->hide();
            connect(&m_progressShowTimer, SIGNAL(timeout()),
                    this, SLOT(showProgressDialog()));
            connect(m_progressDialog, SIGNAL(canceled()), this, SLOT(cancelled()));
            m_progressShowTimer.setSingleShot(true);
            m_progressShowTimer.start(2000);
        }
    }
}

void
RemoteFile::initHttp()
{
    m_ok = true;
    m_http = new QHttp(m_url.host(), m_url.port(80));
    connect(m_http, SIGNAL(done(bool)), this, SLOT(done(bool)));
    connect(m_http, SIGNAL(dataReadProgress(int, int)),
            this, SLOT(dataReadProgress(int, int)));
    connect(m_http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
            this, SLOT(httpResponseHeaderReceived(const QHttpResponseHeader &)));

    // I don't quite understand this.  url.path() returns a path
    // without percent encoding; for example, spaces appear as
    // literal spaces.  This generally won't work if sent to the
    // server directly.  You can retrieve a correctly encoded URL
    // from QUrl using url.toEncoded(), but that gives you the
    // whole URL; there doesn't seem to be any way to retrieve
    // only an encoded path.  Furthermore there doesn't seem to be
    // any way to convert a retrieved path into an encoded path
    // without explicitly specifying that you don't want the path
    // separators ("/") to be encoded.  (Besides being painful to
    // manage, I don't see how this can work correctly in any case
    // where a percent-encoded "/" is supposed to appear within a
    // path element?)  There also seems to be no way to retrieve
    // the path plus query string, i.e. everything that I need to
    // send to the HTTP server.  And no way for QHttp to take a
    // QUrl argument.  I'm obviously missing something.

    // So, two ways to do this: query the bits from the URL,
    // encode them individually, and glue them back together
    // again...
/*
    QString path = QUrl::toPercentEncoding(m_url.path(), "/");
    QList<QPair<QString, QString> > query = m_url.queryItems();
    if (!query.empty()) {
        QStringList q2;
        for (QList<QPair<QString, QString> >::iterator i = query.begin();
             i != query.end(); ++i) {
            q2.push_back(QString("%1=%3")
                         .arg(QString(QUrl::toPercentEncoding(i->first)))
                         .arg(QString(QUrl::toPercentEncoding(i->second))));
        }
        path = QString("%1%2%3")
            .arg(path).arg("?")
            .arg(q2.join("&"));
    }
*/

    // ...or, much simpler but relying on knowledge about the
    // scheme://host/path/path/query etc format of the URL, we can
    // get the whole URL ready-encoded and then split it on "/" as
    // appropriate...
        
    QString path = "/" + QString(m_url.toEncoded()).section('/', 3);

    std::cerr << "RemoteFile: path is \""
              << path.toStdString() << "\"" << std::endl;
        
    m_http->get(path, m_localFile);
}

void
RemoteFile::initFtp()
{
    m_ok = true;
    m_ftp = new QFtp;
    connect(m_ftp, SIGNAL(done(bool)), this, SLOT(done(bool)));
    connect(m_ftp, SIGNAL(commandFinished(int, bool)),
            this, SLOT(ftpCommandFinished(int, bool)));
    connect(m_ftp, SIGNAL(dataTransferProgress(qint64, qint64)),
            this, SLOT(dataTransferProgress(qint64, qint64)));
    m_ftp->connectToHost(m_url.host(), m_url.port(21));
    
    QString username = m_url.userName();
    if (username == "") {
        username = "anonymous";
    }
    
    QString password = m_url.password();
    if (password == "") {
        password = QString("%1@%2").arg(getenv("USER")).arg(getenv("HOST"));
    }
    
    m_ftp->login(username, password);
    
    QString dirpath = m_url.path().section('/', 0, -2);
    QString filename = m_url.path().section('/', -1);
    
    if (dirpath == "") dirpath = "/";
    m_ftp->cd(dirpath);
    m_ftp->get(filename, m_localFile);
}

void
RemoteFile::cleanup()
{
    m_done = true;
    if (m_http) {
        QHttp *h = m_http;
        m_http = 0;
        h->abort();
        h->deleteLater();
    }
    if (m_ftp) {
        QFtp *f = m_ftp;
        m_ftp = 0;
        f->abort();
        f->deleteLater();
    }
    delete m_progressDialog;
    m_progressDialog = 0;
    delete m_localFile; // does not actually delete the file
    m_localFile = 0;
}

bool
RemoteFile::isRemote(QString fileOrUrl)
{
    QString scheme = QUrl(fileOrUrl).scheme().toLower();
    return (scheme == "http" || scheme == "ftp");
}

bool
RemoteFile::canHandleScheme(QUrl url)
{
    QString scheme = url.scheme().toLower();
    return (scheme == "http" || scheme == "ftp" ||
            scheme == "file" || scheme == "");
}

bool
RemoteFile::isAvailable()
{
    waitForStatus();
    bool available = true;
    if (!m_ok) available = false;
    else available = (m_lastStatus / 100 == 2);
    std::cerr << "RemoteFile::isAvailable: " << (available ? "yes" : "no")
              << std::endl;
    return available;
}

void
RemoteFile::waitForStatus()
{
    while (m_ok && (!m_done && m_lastStatus == 0)) {
//        std::cerr << "waitForStatus: processing (last status " << m_lastStatus << ")" << std::endl;
        QApplication::processEvents();
    }
}

void
RemoteFile::waitForData()
{
    while (m_ok && !m_done) {
        QApplication::processEvents();
    }
}

void
RemoteFile::setLeaveLocalFile(bool leave)
{
    m_leaveLocalFile = leave;
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

bool
RemoteFile::isRemote() const
{
    return m_remote;
}

QString
RemoteFile::getLocation() const
{
    return m_url.toString();
}

QString
RemoteFile::getLocalFilename() const
{
    return m_localFilename;
}

QString
RemoteFile::getContentType() const
{
    return m_contentType;
}

QString
RemoteFile::getExtension() const
{
    if (m_localFilename != "") {
        return QFileInfo(m_localFilename).suffix().toLower();
    } else {
        return QFileInfo(m_url.toLocalFile()).suffix().toLower();
    }
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
        if (resp.hasContentType()) m_contentType = resp.contentType();
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
    m_done = true;
    cleanup();

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
        std::cerr << "RemoteFile::done: error is " << error << ", deleting cache file" << std::endl;
        deleteCacheFile();
    }

    m_ok = !error;
    m_done = true;
    emit ready();
}

void
RemoteFile::deleteCacheFile()
{
    std::cerr << "RemoteFile::deleteCacheFile(\"" << m_localFilename.toStdString() << "\")" << std::endl;

    cleanup();

    if (m_localFilename == "") {
        return;
    }

    if (!isRemote()) {
        std::cerr << "not a cache file" << std::endl;
        return;
    }

    if (m_refCounted) {

        QMutexLocker locker(&m_mapMutex);
        m_refCounted = false;

        if (m_refCountMap[m_url] > 0) {
            m_refCountMap[m_url]--;
            std::cerr << "reduced ref count to " << m_refCountMap[m_url] << std::endl;
            if (m_refCountMap[m_url] > 0) {
                m_done = true;
                return;
            }
        }
    }

    m_fileCreationMutex.lock();

    if (!QFile(m_localFilename).remove()) {
        std::cerr << "RemoteFile::deleteCacheFile: ERROR: Failed to delete file \"" << m_localFilename.toStdString() << "\"" << std::endl;
    } else {
        std::cerr << "RemoteFile::deleteCacheFile: Deleted cache file \"" << m_localFilename.toStdString() << "\"" << std::endl;
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

bool
RemoteFile::createCacheFile()
{
    {
        QMutexLocker locker(&m_mapMutex);

        std::cerr << "RemoteFile::createCacheFile: refcount is " << m_refCountMap[m_url] << std::endl;

        if (m_refCountMap[m_url] > 0) {
            m_refCountMap[m_url]++;
            m_localFilename = m_remoteLocalMap[m_url];
            std::cerr << "raised it to " << m_refCountMap[m_url] << std::endl;
            m_refCounted = true;
            return true;
        }
    }

    QDir dir;
    try {
        dir = TempDirectory::getInstance()->getSubDirectoryPath("download");
    } catch (DirectoryCreationFailed f) {
        std::cerr << "RemoteFile::createCacheFile: ERROR: Failed to create temporary directory: " << f.what() << std::endl;
        return "";
    }

    QString filepart = m_url.path().section('/', -1, -1,
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

    std::cerr << "RemoteFile::createCacheFile: URL is \"" << m_url.toString().toStdString() << "\", dir is \"" << dir.path().toStdString() << "\", base \"" << base.toStdString() << "\", extension \"" << extension.toStdString() << "\", filebase \"" << filename.toStdString() << "\", filename \"" << filepath.toStdString() << "\"" << std::endl;

    QMutexLocker fcLocker(&m_fileCreationMutex);

    ++m_count;

    if (QFileInfo(filepath).exists() ||
        !QFile(filepath).open(QFile::WriteOnly)) {

        std::cerr << "RemoteFile::createCacheFile: Failed to create local file \""
                  << filepath.toStdString() << "\" for URL \""
                  << m_url.toString().toStdString() << "\" (or file already exists): appending suffix instead" << std::endl;


        if (extension == "") {
            filename = QString("%1_%2").arg(base).arg(m_count);
        } else {
            filename = QString("%1_%2.%3").arg(base).arg(m_count).arg(extension);
        }
        filepath = dir.filePath(filename);

        if (QFileInfo(filepath).exists() ||
            !QFile(filepath).open(QFile::WriteOnly)) {

            std::cerr << "RemoteFile::createCacheFile: ERROR: Failed to create local file \""
                      << filepath.toStdString() << "\" for URL \""
                      << m_url.toString().toStdString() << "\" (or file already exists)" << std::endl;

            return "";
        }
    }

    std::cerr << "RemoteFile::createCacheFile: url "
              << m_url.toString().toStdString() << " -> local filename "
              << filepath.toStdString() << std::endl;
    
    m_localFilename = filepath;

    return false;
}
