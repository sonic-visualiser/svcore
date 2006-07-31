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

#include "FileReadThread.h"

#include "base/Profiler.h"

#include <iostream>
#include <unistd.h>

//#define DEBUG_FILE_READ_THREAD 1

FileReadThread::FileReadThread() :
    m_nextToken(0),
    m_exiting(false)
{
}

void
FileReadThread::run()
{
    m_mutex.lock();

    while (!m_exiting) {
        if (m_queue.empty()) {
            m_condition.wait(&m_mutex, 1000);
        } else {
            process();
        }
        notifyCancelled();
    }

    notifyCancelled();
    m_mutex.unlock();

#ifdef DEBUG_FILE_READ_THREAD
    std::cerr << "FileReadThread::run() exiting" << std::endl;
#endif
}

void
FileReadThread::finish()
{
#ifdef DEBUG_FILE_READ_THREAD
    std::cerr << "FileReadThread::finish()" << std::endl;
#endif

    m_mutex.lock();
    while (!m_queue.empty()) {
        m_cancelledRequests[m_queue.begin()->first] = m_queue.begin()->second;
        m_newlyCancelled.insert(m_queue.begin()->first);
        m_queue.erase(m_queue.begin());
    }

    m_exiting = true;
    m_mutex.unlock();

    m_condition.wakeAll();

#ifdef DEBUG_FILE_READ_THREAD
    std::cerr << "FileReadThread::finish() exiting" << std::endl;
#endif
}

int
FileReadThread::request(const Request &request)
{
    m_mutex.lock();
    
    int token = m_nextToken++;
    m_queue[token] = request;

    m_mutex.unlock();
    m_condition.wakeAll();

    return token;
}

void
FileReadThread::cancel(int token)
{
    m_mutex.lock();

    if (m_queue.find(token) != m_queue.end()) {
        m_cancelledRequests[token] = m_queue[token];
        m_queue.erase(token);
        m_newlyCancelled.insert(token);
    } else if (m_readyRequests.find(token) != m_readyRequests.end()) {
        m_cancelledRequests[token] = m_readyRequests[token];
        m_readyRequests.erase(token);
    } else {
        std::cerr << "WARNING: FileReadThread::cancel: token " << token << " not found" << std::endl;
    }

    m_mutex.unlock();

#ifdef DEBUG_FILE_READ_THREAD
    std::cerr << "FileReadThread::cancel(" << token << ") waking condition" << std::endl;
#endif

    m_condition.wakeAll();
}

bool
FileReadThread::isReady(int token)
{
    m_mutex.lock();

    bool ready = m_readyRequests.find(token) != m_readyRequests.end();

    m_mutex.unlock();
    return ready;
}

bool
FileReadThread::isCancelled(int token)
{
    m_mutex.lock();

    bool cancelled = 
        m_cancelledRequests.find(token) != m_cancelledRequests.end() &&
        m_newlyCancelled.find(token) == m_newlyCancelled.end();

    m_mutex.unlock();
    return cancelled;
}

bool
FileReadThread::getRequest(int token, Request &request)
{
    m_mutex.lock();

    bool found = false;

    if (m_queue.find(token) != m_queue.end()) {
        request = m_queue[token];
        found = true;
    } else if (m_cancelledRequests.find(token) != m_cancelledRequests.end()) {
        request = m_cancelledRequests[token];
        found = true;
    } else if (m_readyRequests.find(token) != m_readyRequests.end()) {
        request = m_readyRequests[token];
        found = true;
    }

    m_mutex.unlock();
    
    return found;
}

void
FileReadThread::done(int token)
{
    m_mutex.lock();

    bool found = false;

    if (m_cancelledRequests.find(token) != m_cancelledRequests.end()) {
        m_cancelledRequests.erase(token);
        m_newlyCancelled.erase(token);
        found = true;
    } else if (m_readyRequests.find(token) != m_readyRequests.end()) {
        m_readyRequests.erase(token);
        found = true;
    } else if (m_queue.find(token) != m_queue.end()) {
        std::cerr << "WARNING: FileReadThread::done(" << token << "): request is still in queue (wait or cancel it)" << std::endl;
    }

    m_mutex.unlock();

    if (!found) {
        std::cerr << "WARNING: FileReadThread::done(" << token << "): request not found" << std::endl;
    }
}

void
FileReadThread::process()
{
    // entered with m_mutex locked and m_queue non-empty

#ifdef DEBUG_FILE_READ_THREAD
    Profiler profiler("FileReadThread::process()", true);
#endif

    int token = m_queue.begin()->first;
    Request request = m_queue.begin()->second;

    m_mutex.unlock();

#ifdef DEBUG_FILE_READ_THREAD
    std::cerr << "FileReadThread::process: reading " << request.start << ", " << request.size << " on " << request.fd << std::endl;
#endif

    bool successful = false;
    bool seekFailed = false;
    ssize_t r = 0;

    if (request.mutex) request.mutex->lock();

    if (::lseek(request.fd, request.start, SEEK_SET) == (off_t)-1) {
        seekFailed = true;
    } else {
        
        // if request.size is large, we want to avoid making a single
        // system call to read it all as it may block too much

        static const size_t blockSize = 256 * 1024;
        
        size_t size = request.size;
        char *destination = request.data;

        while (size > 0) {
            size_t readSize = size;
            if (readSize > blockSize) readSize = blockSize;
            ssize_t br = ::read(request.fd, destination, readSize);
            if (br < 0) { 
                r = br;
                break;
            } else {
                r += br;
                if (br < ssize_t(readSize)) break;
            }
            destination += readSize;
            size -= readSize;
        }
    }

    if (request.mutex) request.mutex->unlock();

    if (seekFailed) {
        ::perror("Seek failed");
        std::cerr << "ERROR: FileReadThread::process: seek to "
                  << request.start << " failed" << std::endl;
        request.size = 0;
    } else {
        if (r < 0) {
            ::perror("ERROR: FileReadThread::process: Read failed");
            request.size = 0;
        } else if (r < ssize_t(request.size)) {
            std::cerr << "WARNING: FileReadThread::process: read "
                      << request.size << " returned only " << r << " bytes"
                      << std::endl;
            request.size = r;
            usleep(100000);
        } else {
            successful = true;
        }
    }
        
    // Check that the token hasn't been cancelled and the thread
    // hasn't been asked to finish
    
    m_mutex.lock();

    request.successful = successful;
        
    if (m_queue.find(token) != m_queue.end() && !m_exiting) {
        m_queue.erase(token);
        m_readyRequests[token] = request;
#ifdef DEBUG_FILE_READ_THREAD
        std::cerr << "FileReadThread::process: done, marking as ready" << std::endl;
#endif
    } else {
#ifdef DEBUG_FILE_READ_THREAD
        std::cerr << "FileReadThread::process: request disappeared or exiting" << std::endl;
#endif
    }
}

void
FileReadThread::notifyCancelled()
{
    // entered with m_mutex locked

    while (!m_newlyCancelled.empty()) {

        int token = *m_newlyCancelled.begin();

#ifdef DEBUG_FILE_READ_THREAD
        std::cerr << "FileReadThread::notifyCancelled: token " << token << std::endl;
#endif

        m_newlyCancelled.erase(token);
    }
}
        
    
