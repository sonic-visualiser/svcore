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

#ifndef _FILE_READ_THREAD_H_
#define _FILE_READ_THREAD_H_

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <map>
#include <set>

#include <stdint.h>

class FileReadThread : public QThread
{
    Q_OBJECT

public:
    FileReadThread();

    virtual void run();
    virtual void finish();

    struct Request {
        int fd;
        QMutex *mutex; // used to synchronise access to fd; may be null
        off_t start;
        size_t size;
        char *data; // caller is responsible for allocating and deallocating
    };
    
    virtual int request(const Request &request);
    virtual void cancel(int token);

    virtual bool isReady(int token);
    virtual bool isCancelled(int token); // and safe to delete
    virtual bool getRequest(int token, Request &request);
    virtual void done(int token);

signals:
    void ready(int token, bool successful); 
    void cancelled(int token);
    
protected:
    int m_nextToken;
    bool m_exiting;
    
    typedef std::map<int, Request> RequestQueue;
    RequestQueue m_queue;
    RequestQueue m_cancelledRequests;
    RequestQueue m_readyRequests;
    std::set<int> m_newlyCancelled;

    QMutex m_mutex;
    QWaitCondition m_condition;

    void process();
    void notifyCancelled();
};

#endif
