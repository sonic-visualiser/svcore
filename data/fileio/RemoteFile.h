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

class QFtp;
class QHttp;
class QFile;
class QProgressDialog;

class RemoteFile : public QObject
{
    Q_OBJECT

public:
    RemoteFile(QUrl url);
    virtual ~RemoteFile();

    void wait();

    bool isOK() const;
    bool isDone() const;
    QString getLocalFilename() const;
    QString getErrorString() const;

    static bool canHandleScheme(QUrl url);

signals:
    void progress(int percent);
    void ready();

protected slots:
    void dataReadProgress(int done, int total);
    void dataTransferProgress(qint64 done, qint64 total);
    void done(bool error);

protected:
    QFtp *m_ftp;
    QHttp *m_http;
    QFile *m_localFile;
    QString m_localFilename;
    QString m_errorString;
    bool m_ok;
    bool m_done;
    QProgressDialog *m_progressDialog;

    QString createLocalFile(QUrl url);

    static QMutex m_fileCreationMutex;
    static int m_count;
};

#endif
