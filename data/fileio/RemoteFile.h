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

#ifndef _REMOTE_FILE_H_
#define _REMOTE_FILE_H_

#include <QUrl>
#include <QMutex>
#include <QString>
#include <QTimer>

#include <map>

class QFtp;
class QHttp;
class QFile;
class QProgressDialog;
class QHttpResponseHeader;

class RemoteFile : public QObject
{
    Q_OBJECT

public:
    RemoteFile(QString fileOrUrl, bool showProgress = true);
    RemoteFile(QUrl url, bool showProgress = true);
    RemoteFile(const RemoteFile &);

    virtual ~RemoteFile();

    bool isAvailable();

    void waitForStatus();
    void waitForData();

    void setLeaveLocalFile(bool leave);

    bool isOK() const;
    bool isDone() const;
    bool isRemote() const;

    QString getLocation() const;
    QString getLocalFilename() const;
    QString getContentType() const;
    QString getExtension() const;

    QString getErrorString() const;

    static bool isRemote(QString fileOrUrl);
    static bool canHandleScheme(QUrl url);

signals:
    void progress(int percent);
    void ready();

protected slots:
    void dataReadProgress(int done, int total);
    void httpResponseHeaderReceived(const QHttpResponseHeader &resp);
    void ftpCommandFinished(int, bool);
    void dataTransferProgress(qint64 done, qint64 total);
    void done(bool error);
    void showProgressDialog();
    void cancelled();

protected:
    RemoteFile &operator=(const RemoteFile &); // not provided

    QUrl m_url;
    QFtp *m_ftp;
    QHttp *m_http;
    QFile *m_localFile;
    QString m_localFilename;
    QString m_errorString;
    QString m_contentType;
    bool m_ok;
    int m_lastStatus;
    bool m_remote;
    bool m_done;
    bool m_leaveLocalFile;
    QProgressDialog *m_progressDialog;
    QTimer m_progressShowTimer;

    typedef std::map<QUrl, int> RemoteRefCountMap;
    typedef std::map<QUrl, QString> RemoteLocalMap;
    static RemoteRefCountMap m_refCountMap;
    static RemoteLocalMap m_remoteLocalMap;
    static QMutex m_mapMutex;
    bool m_refCounted;

    void init(bool showProgress);
    void initHttp();
    void initFtp();

    void cleanup();

    // Create a local file for m_url.  If it already existed, return true.
    // The local filename is stored in m_localFilename.
    bool createCacheFile();
    void deleteCacheFile();

    static QMutex m_fileCreationMutex;
    static int m_count;
};

#endif
