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

#include "MatrixFile.h"
#include "base/TempDirectory.h"
#include "base/System.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>

#include <cstdio>

#include <QFileInfo>
#include <QDir>

std::map<QString, int> MatrixFile::m_refcount;
QMutex MatrixFile::m_refcountMutex;

MatrixFile::MatrixFile(QString fileBase, Mode mode) :
    m_fd(-1),
    m_mode(mode),
    m_width(0),
    m_height(0),
    m_headerSize(2 * sizeof(size_t)),
    m_defaultCacheWidth(512),
    m_prevX(0),
    m_requestToken(-1),
    m_spareData(0)
{
    m_cache.data = 0;

    QDir tempDir(TempDirectory::instance()->getPath());
    QString fileName(tempDir.filePath(QString("%1.mfc").arg(fileBase)));
    bool newFile = !QFileInfo(fileName).exists();

    if (newFile && mode == ReadOnly) {
        std::cerr << "ERROR: MatrixFile::MatrixFile: Read-only mode "
                  << "specified, but cache file does not exist" << std::endl;
        return;
    }

    int flags = 0;
    mode_t fmode = S_IRUSR | S_IWUSR;

    if (mode == ReadWrite) {
        flags = O_RDWR | O_CREAT;
    } else {
        flags = O_RDONLY;
    }

    if ((m_fd = ::open(fileName.toLocal8Bit(), flags, fmode)) < 0) {
        ::perror("Open failed");
        std::cerr << "ERROR: MatrixFile::MatrixFile: "
                  << "Failed to open cache file \""
                  << fileName.toStdString() << "\"";
        if (mode == ReadWrite) std::cerr << " for writing";
        std::cerr << std::endl;
        return;
    }

    if (newFile) {
        resize(0, 0); // write header
    } else {
        size_t header[2];
        if (::read(m_fd, header, 2 * sizeof(size_t)) < 0) {
            perror("Read failed");
            std::cerr << "ERROR: MatrixFile::MatrixFile: "
                      << "Failed to read header (fd " << m_fd << ", file \""
                      << fileName.toStdString() << "\")" << std::endl;
            return;
        }
        m_width = header[0];
        m_height = header[1];
        seekTo(0, 0);
    }

    m_fileName = fileName;

    m_readThread.start();

    QMutexLocker locker(&m_refcountMutex);
    ++m_refcount[fileName];

    std::cerr << "MatrixFile::MatrixFile: Done, size is " << "(" << m_width << ", " << m_height << ")" << std::endl;

}

MatrixFile::~MatrixFile()
{
    float *requestData = 0;

    if (m_requestToken >= 0) {
        FileReadThread::Request request;
        if (m_readThread.getRequest(m_requestToken, request)) {
            requestData = (float *)request.data;
        }
    }

    m_readThread.finish();
    m_readThread.wait();

    if (requestData) free(requestData);
    if (m_cache.data) free(m_cache.data);
    if (m_spareData) free(m_spareData);

    if (m_fd >= 0) {
        if (::close(m_fd) < 0) {
            ::perror("MatrixFile::~MatrixFile: close failed");
        }
    }

    if (m_fileName != "") {
        QMutexLocker locker(&m_refcountMutex);
        if (--m_refcount[m_fileName] == 0) {
            if (::unlink(m_fileName.toLocal8Bit())) {
                ::perror("Unlink failed");
                std::cerr << "WARNING: MatrixFile::~MatrixFile: reference count reached 0, but failed to unlink file \"" << m_fileName.toStdString() << "\"" << std::endl;
            } else {
                std::cerr << "deleted " << m_fileName.toStdString() << std::endl;
            }
        }
    }
}

size_t 
MatrixFile::getWidth() const
{
    return m_width;
}

size_t
MatrixFile::getHeight() const
{
    return m_height;
}

void
MatrixFile::resize(size_t w, size_t h)
{
    if (m_mode != ReadWrite) {
        std::cerr << "ERROR: MatrixFile::resize called on read-only cache"
                  << std::endl;
        return;
    }

    QMutexLocker locker(&m_fdMutex);
    
    off_t off = m_headerSize + (w * h * sizeof(float));

    if (w * h > m_width * m_height) {

        if (::lseek(m_fd, off - sizeof(float), SEEK_SET) == (off_t)-1) {
            ::perror("Seek failed");
            std::cerr << "ERROR: MatrixFile::resize(" << w << ", "
                      << h << "): seek failed, cannot resize" << std::endl;
            return;
        }
            
        // guess this requires efficient support for sparse files
        
        float f(0);
        if (::write(m_fd, &f, sizeof(float)) != sizeof(float)) {
            ::perror("WARNING: MatrixFile::resize: write failed");
        }

    } else {
        
        if (::ftruncate(m_fd, off) < 0) {
            ::perror("WARNING: MatrixFile::resize: ftruncate failed");
        }
    }

    m_width = 0;
    m_height = 0;

    if (::lseek(m_fd, 0, SEEK_SET) == (off_t)-1) {
        ::perror("ERROR: MatrixFile::resize: Seek to write header failed");
        return;
    }

    size_t header[2];
    header[0] = w;
    header[1] = h;
    if (::write(m_fd, header, 2 * sizeof(size_t)) != 2 * sizeof(size_t)) {
        ::perror("ERROR: MatrixFile::resize: Failed to write header");
        return;
    }

    m_width = w;
    m_height = h;

    seekTo(0, 0);
}

void
MatrixFile::reset()
{
    if (m_mode != ReadWrite) {
        std::cerr << "ERROR: MatrixFile::reset called on read-only cache"
                  << std::endl;
        return;
    }
    
    QMutexLocker locker(&m_fdMutex);

    float *emptyCol = new float[m_height];
    for (size_t y = 0; y < m_height; ++y) emptyCol[y] = 0.f;

    seekTo(0, 0);
    for (size_t x = 0; x < m_width; ++x) setColumnAt(x, emptyCol);
    
    delete[] emptyCol;
}

float
MatrixFile::getValueAt(size_t x, size_t y)
{
    float value = 0.f;
    if (getValuesFromCache(x, y, 1, &value)) return value;

    ssize_t r = 0;

//    std::cout << "MatrixFile::getValueAt(" << x << ", " << y << ")"
//              << ": reading the slow way" << std::endl;

    m_fdMutex.lock();

    if (seekTo(x, y)) {
        r = ::read(m_fd, &value, sizeof(float));
    }

    m_fdMutex.unlock();

    if (r < 0) {
        ::perror("MatrixFile::getValueAt: Read failed");
    }
    if (r != sizeof(float)) {
        value = 0.f;
    }

    return value;
}

void
MatrixFile::getColumnAt(size_t x, float *values)
{
    if (getValuesFromCache(x, 0, m_height, values)) return;

    ssize_t r = 0;

    std::cout << "MatrixFile::getColumnAt(" << x << ")"
              << ": reading the slow way" << std::endl;

    m_fdMutex.lock();

    if (seekTo(x, 0)) {
        r = ::read(m_fd, values, m_height * sizeof(float));
    }

    m_fdMutex.unlock();
    
    if (r < 0) {
        ::perror("MatrixFile::getColumnAt: read failed");
    }
}

bool
MatrixFile::getValuesFromCache(size_t x, size_t ystart, size_t ycount,
                                    float *values)
{
    m_cacheMutex.lock();

    if (!m_cache.data || x < m_cache.x || x >= m_cache.x + m_cache.width) {
        bool left = (m_cache.data && x < m_cache.x);
        m_cacheMutex.unlock();
        primeCache(x, left); // this doesn't take effect until a later callback
        m_prevX = x;
        return false;
    }

    for (size_t y = ystart; y < ystart + ycount; ++y) {
        values[y - ystart] = m_cache.data[(x - m_cache.x) * m_height + y];
    }
    m_cacheMutex.unlock();

    if (m_cache.x > 0 && x < m_prevX && x < m_cache.x + m_cache.width/4) {
        primeCache(x, true);
    }

    if (m_cache.x + m_cache.width < m_width &&
        x > m_prevX &&
        x > m_cache.x + (m_cache.width * 3) / 4) {
        primeCache(x, false);
    }

    m_prevX = x;
    return true;
}

void
MatrixFile::setValueAt(size_t x, size_t y, float value)
{
    if (m_mode != ReadWrite) {
        std::cerr << "ERROR: MatrixFile::setValueAt called on read-only cache"
                  << std::endl;
        return;
    }

    ssize_t w = 0;
    bool seekFailed = false;

    m_fdMutex.lock();

    if (seekTo(x, y)) {
        w = ::write(m_fd, &value, sizeof(float));
    } else {
        seekFailed = true;
    }

    m_fdMutex.unlock();

    if (!seekFailed && w != sizeof(float)) {
        ::perror("WARNING: MatrixFile::setValueAt: write failed");
    }

    //... update cache as appropriate
}

void
MatrixFile::setColumnAt(size_t x, float *values)
{
    if (m_mode != ReadWrite) {
        std::cerr << "ERROR: MatrixFile::setColumnAt called on read-only cache"
                  << std::endl;
        return;
    }

    ssize_t w = 0;
    bool seekFailed = false;

    m_fdMutex.lock();

    if (seekTo(x, 0)) {
        w = ::write(m_fd, values, m_height * sizeof(float));
    } else {
        seekFailed = true;
    }

    m_fdMutex.unlock();

    if (!seekFailed && w != ssize_t(m_height * sizeof(float))) {
        ::perror("WARNING: MatrixFile::setColumnAt: write failed");
    }

    //... update cache as appropriate
}

void
MatrixFile::primeCache(size_t x, bool goingLeft)
{
//    std::cerr << "MatrixFile::primeCache(" << x << ", " << goingLeft << ")" << std::endl;

    size_t rx = x;
    size_t rw = m_defaultCacheWidth;

    size_t left = rw / 3;
    if (goingLeft) left = (rw * 2) / 3;

    if (rx > left) rx -= left;
    else rx = 0;

    if (rx + rw > m_width) rw = m_width - rx;

    QMutexLocker locker(&m_cacheMutex);

    FileReadThread::Request request;

    if (m_requestToken >= 0 &&
        m_readThread.getRequest(m_requestToken, request)) {

        if (x >= m_requestingX &&
            x <  m_requestingX + m_requestingWidth) {

            if (m_readThread.isReady(m_requestToken)) {

                std::cerr << "last request is ready! (" << m_requestingX << ", "<< m_requestingWidth << ")"  << std::endl;

                m_cache.x = (request.start - m_headerSize) / (m_height * sizeof(float));
                m_cache.width = request.size / (m_height * sizeof(float));
                
                std::cerr << "actual: " << m_cache.x << ", " << m_cache.width << std::endl;

                if (m_cache.data) {
                    if (m_spareData) free(m_spareData);
                    m_spareData = (char *)m_cache.data;
                }
                m_cache.data = (float *)request.data;

                m_readThread.done(m_requestToken);
                m_requestToken = -1;
            }

            // already requested something covering this area; wait for it
            return;
        }

        // the current request is no longer of any use
        m_readThread.cancel(m_requestToken);

        // crude way to avoid leaking the data
        while (!m_readThread.isCancelled(m_requestToken)) {
            usleep(10000);
        }

        if (m_spareData) free(m_spareData);
        m_spareData = request.data;
        m_readThread.done(m_requestToken);

        m_requestToken = -1;
    }

    request.fd = m_fd;
    request.mutex = &m_fdMutex;
    request.start = m_headerSize + rx * m_height * sizeof(float);
    request.size = rw * m_height * sizeof(float);
    request.data = (char *)realloc(m_spareData, rw * m_height * sizeof(float));
    MUNLOCK(request.data, rw * m_height * sizeof(float));
    m_spareData = 0;

    m_requestingX = rx;
    m_requestingWidth = rw;

    int token = m_readThread.request(request);
    std::cerr << "MatrixFile::primeCache: request token is "
              << token << " (x = " << rx << ", w = " << rw << ", left = " << goingLeft << ")" << std::endl;

    m_requestToken = token;
}

bool
MatrixFile::seekTo(size_t x, size_t y)
{
    off_t off = m_headerSize + (x * m_height + y) * sizeof(float);

    if (::lseek(m_fd, off, SEEK_SET) == (off_t)-1) {
        ::perror("Seek failed");
        std::cerr << "ERROR: MatrixFile::seekTo(" << x << ", " << y
                  << ") failed" << std::endl;
        return false;
    }

    return true;
}

