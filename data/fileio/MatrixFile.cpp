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
#include "system/System.h"
#include "base/Profiler.h"
#include "base/Exceptions.h"
#include "base/Thread.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>

#include <cstdio>
#include <cassert>

#include <cstdlib>

#include <QFileInfo>
#include <QDir>

//#define DEBUG_MATRIX_FILE 1
//#define DEBUG_MATRIX_FILE_READ_SET 1

#ifdef DEBUG_MATRIX_FILE_READ_SET
#ifndef DEBUG_MATRIX_FILE
#define DEBUG_MATRIX_FILE 1
#endif
#endif

std::map<QString, int> MatrixFile::m_refcount;
QMutex MatrixFile::m_refcountMutex;

MatrixFile::ResizeableBitsetMap MatrixFile::m_columnBitsets;
QMutex MatrixFile::m_columnBitsetWriteMutex;

FileReadThread *MatrixFile::m_readThread = 0;

static size_t totalStorage = 0;
static size_t totalMemory = 0;
static size_t totalCount = 0;

MatrixFile::MatrixFile(QString fileBase, Mode mode,
                       size_t cellSize, bool eagerCache) :
    m_fd(-1),
    m_mode(mode),
    m_flags(0),
    m_fmode(0),
    m_cellSize(cellSize),
    m_width(0),
    m_height(0),
    m_headerSize(2 * sizeof(size_t)),
    m_defaultCacheWidth(1024),
    m_prevX(0),
    m_eagerCache(eagerCache),
    m_requestToken(-1),
    m_spareData(0),
    m_columnBitset(0)
{
    Profiler profiler("MatrixFile::MatrixFile", true);

    if (!m_readThread) {
        m_readThread = new FileReadThread;
        m_readThread->start();
    }

    m_cache.data = 0;

    QDir tempDir(TempDirectory::getInstance()->getPath());
    QString fileName(tempDir.filePath(QString("%1.mfc").arg(fileBase)));
    bool newFile = !QFileInfo(fileName).exists();

    if (newFile && m_mode == ReadOnly) {
        std::cerr << "ERROR: MatrixFile::MatrixFile: Read-only mode "
                  << "specified, but cache file does not exist" << std::endl;
        throw FileNotFound(fileName);
    }

    if (!newFile && m_mode == ReadWrite) {
        std::cerr << "Note: MatrixFile::MatrixFile: Read/write mode "
                  << "specified, but file already exists; falling back to "
                  << "read-only mode" << std::endl;
        m_mode = ReadOnly;
    }

    if (!eagerCache && m_mode == ReadOnly) {
        std::cerr << "WARNING: MatrixFile::MatrixFile: Eager cacheing not "
                  << "specified, but file is open in read-only mode -- cache "
                  << "will not be used" << std::endl;
    }

    m_flags = 0;
    m_fmode = S_IRUSR | S_IWUSR;

    if (m_mode == ReadWrite) {
        m_flags = O_RDWR | O_CREAT;
    } else {
        m_flags = O_RDONLY;
    }

#ifdef _WIN32
    m_flags |= O_BINARY;
#endif

#ifdef DEBUG_MATRIX_FILE
    std::cerr << "MatrixFile::MatrixFile: opening " << fileName.toStdString() << "..." << std::endl;
#endif

    if ((m_fd = ::open(fileName.toLocal8Bit(), m_flags, m_fmode)) < 0) {
        ::perror("Open failed");
        std::cerr << "ERROR: MatrixFile::MatrixFile: "
                  << "Failed to open cache file \""
                  << fileName.toStdString() << "\"";
        if (m_mode == ReadWrite) std::cerr << " for writing";
        std::cerr << std::endl;
        throw FailedToOpenFile(fileName);
    }

    if (newFile) {
        resize(0, 0); // write header
    } else {
        size_t header[2];
        if (::read(m_fd, header, 2 * sizeof(size_t)) < 0) {
            ::perror("MatrixFile::MatrixFile: read failed");
            std::cerr << "ERROR: MatrixFile::MatrixFile: "
                      << "Failed to read header (fd " << m_fd << ", file \""
                      << fileName.toStdString() << "\")" << std::endl;
            throw FileReadFailed(fileName);
        }
        m_width = header[0];
        m_height = header[1];
        seekTo(0, 0);
    }

    m_fileName = fileName;

    {
        MutexLocker locker
            (&m_columnBitsetWriteMutex,
             "MatrixFile::MatrixFile::m_columnBitsetWriteMutex");

        if (m_columnBitsets.find(m_fileName) == m_columnBitsets.end()) {
            m_columnBitsets[m_fileName] = new ResizeableBitset;
        }
        m_columnBitset = m_columnBitsets[m_fileName];
    }

    MutexLocker locker(&m_refcountMutex,
                       "MatrixFile::MatrixFile::m_refcountMutex");
    ++m_refcount[fileName];

//    std::cerr << "MatrixFile(" << this << "): fd " << m_fd << ", file " << fileName.toStdString() << ", ref " << m_refcount[fileName] << std::endl;

//    std::cerr << "MatrixFile::MatrixFile: Done, size is " << "(" << m_width << ", " << m_height << ")" << std::endl;

    ++totalCount;

}

MatrixFile::~MatrixFile()
{
    char *requestData = 0;

    if (m_requestToken >= 0) {
        FileReadThread::Request request;
        if (m_readThread->getRequest(m_requestToken, request)) {
            requestData = request.data;
        }
        m_readThread->cancel(m_requestToken);
    }

    if (requestData) free(requestData);
    if (m_cache.data) free(m_cache.data);
    if (m_spareData) free(m_spareData);

    if (m_fd >= 0) {
        if (::close(m_fd) < 0) {
            ::perror("MatrixFile::~MatrixFile: close failed");
        }
    }

    if (m_fileName != "") {

        MutexLocker locker(&m_refcountMutex,
                           "MatrixFile::~MatrixFile::m_refcountMutex");

        if (--m_refcount[m_fileName] == 0) {

            if (::unlink(m_fileName.toLocal8Bit())) {
//                ::perror("Unlink failed");
//                std::cerr << "WARNING: MatrixFile::~MatrixFile: reference count reached 0, but failed to unlink file \"" << m_fileName.toStdString() << "\"" << std::endl;
            } else {
//                std::cerr << "deleted " << m_fileName.toStdString() << std::endl;
            }

            MutexLocker locker2
                (&m_columnBitsetWriteMutex,
                 "MatrixFile::~MatrixFile::m_columnBitsetWriteMutex");
            m_columnBitsets.erase(m_fileName);
            delete m_columnBitset;
        }
    }
    
    totalStorage -= (m_headerSize + (m_width * m_height * m_cellSize));
    totalMemory -= (2 * m_defaultCacheWidth * m_height * m_cellSize);
    totalCount --;

//    std::cerr << "MatrixFile::~MatrixFile: " << std::endl;
//    std::cerr << "Total storage now " << totalStorage/1024 << "K, theoretical max memory "
//              << totalMemory/1024 << "K in " << totalCount << " instances" << std::endl;

}

void
MatrixFile::resize(size_t w, size_t h)
{
    Profiler profiler("MatrixFile::resize", true);

    assert(m_mode == ReadWrite);

    MutexLocker locker(&m_fdMutex, "MatrixFile::resize::m_fdMutex");
    
    totalStorage -= (m_headerSize + (m_width * m_height * m_cellSize));
    totalMemory -= (2 * m_defaultCacheWidth * m_height * m_cellSize);

    off_t off = m_headerSize + (w * h * m_cellSize);

#ifdef DEBUG_MATRIX_FILE
    std::cerr << "MatrixFile::resize(" << w << ", " << h << "): resizing file" << std::endl;
#endif

    if (w * h < m_width * m_height) {
        if (::ftruncate(m_fd, off) < 0) {
            ::perror("WARNING: MatrixFile::resize: ftruncate failed");
            throw FileOperationFailed(m_fileName, "ftruncate");
        }
    }

    m_width = 0;
    m_height = 0;

    if (::lseek(m_fd, 0, SEEK_SET) == (off_t)-1) {
        ::perror("ERROR: MatrixFile::resize: Seek to write header failed");
        throw FileOperationFailed(m_fileName, "lseek");
    }

    size_t header[2];
    header[0] = w;
    header[1] = h;
    if (::write(m_fd, header, 2 * sizeof(size_t)) != 2 * sizeof(size_t)) {
        ::perror("ERROR: MatrixFile::resize: Failed to write header");
        throw FileOperationFailed(m_fileName, "write");
    }

    if (w > 0 && m_defaultCacheWidth > w) {
        m_defaultCacheWidth = w;
    }

//!!!    static size_t maxCacheMB = 16;
    static size_t maxCacheMB = 4;
    if (2 * m_defaultCacheWidth * h * m_cellSize > maxCacheMB * 1024 * 1024) { //!!!
        m_defaultCacheWidth = (maxCacheMB * 1024 * 1024) / (2 * h * m_cellSize);
        if (m_defaultCacheWidth < 16) m_defaultCacheWidth = 16;
    }

    if (m_columnBitset) {
        MutexLocker locker(&m_columnBitsetWriteMutex,
                           "MatrixFile::resize::m_columnBitsetWriteMutex");
        m_columnBitset->resize(w);
    }

    if (m_cache.data) {
        free(m_cache.data);
        m_cache.data = 0;
    }

    if (m_spareData) {
        free(m_spareData);
        m_spareData = 0;
    }
    
    m_width = w;
    m_height = h;

    totalStorage += (m_headerSize + (m_width * m_height * m_cellSize));
    totalMemory += (2 * m_defaultCacheWidth * m_height * m_cellSize);

#ifdef DEBUG_MATRIX_FILE
    std::cerr << "MatrixFile::resize(" << w << ", " << h << "): cache width "
              << m_defaultCacheWidth << ", storage "
              << (m_headerSize + w * h * m_cellSize) << ", mem "
              << (2 * h * m_defaultCacheWidth * m_cellSize) << std::endl;

    std::cerr << "Total storage " << totalStorage/1024 << "K, theoretical max memory "
              << totalMemory/1024 << "K in " << totalCount << " instances" << std::endl;
#endif

    seekTo(0, 0);
}

void
MatrixFile::reset()
{
    Profiler profiler("MatrixFile::reset", true);

    assert (m_mode == ReadWrite);
    
    if (m_eagerCache) {
        void *emptyCol = calloc(m_height, m_cellSize);
        for (size_t x = 0; x < m_width; ++x) setColumnAt(x, emptyCol);
        free(emptyCol);
    }
    
    if (m_columnBitset) {
        MutexLocker locker(&m_columnBitsetWriteMutex,
                           "MatrixFile::reset::m_columnBitsetWriteMutex");
        m_columnBitset->resize(m_width);
    }
}

void
MatrixFile::getColumnAt(size_t x, void *data)
{
    Profiler profiler("MatrixFile::getColumnAt");

//    assert(haveSetColumnAt(x));

    if (getFromCache(x, 0, m_height, data)) return;

    Profiler profiler2("MatrixFile::getColumnAt (uncached)");

    ssize_t r = 0;

#ifdef DEBUG_MATRIX_FILE
    std::cerr << "MatrixFile::getColumnAt(" << x << ")"
              << ": reading the slow way";

    if (m_requestToken >= 0 &&
        x >= m_requestingX &&
        x <  m_requestingX + m_requestingWidth) {
        
        std::cerr << " (awaiting " << m_requestingX << ", " << m_requestingWidth << " from disk)";
    }

    std::cerr << std::endl;
#endif

    {
        MutexLocker locker(&m_fdMutex, "MatrixFile::getColumnAt::m_fdMutex");

        if (seekTo(x, 0)) {
            r = ::read(m_fd, data, m_height * m_cellSize);
        }
    }
    
    if (r < 0) {
        ::perror("MatrixFile::getColumnAt: read failed");
        std::cerr << "ERROR: MatrixFile::getColumnAt: "
                  << "Failed to read column " << x << " (height " << m_height << ", cell size " << m_cellSize << ", fd " << m_fd << ", file \""
                  << m_fileName.toStdString() << "\")" << std::endl;
        throw FileReadFailed(m_fileName);
    }

    return;
}

bool
MatrixFile::getFromCache(size_t x, size_t ystart, size_t ycount, void *data)
{
    bool fail = false;
    bool primeLeft = false;

    {
        MutexLocker locker(&m_cacheMutex,
                           "MatrixFile::getFromCache::m_cacheMutex");

        if (!m_cache.data || x < m_cache.x || x >= m_cache.x + m_cache.width) {
            fail = true;
            primeLeft = (m_cache.data && x < m_cache.x);
        } else {
            memcpy(data,
                   m_cache.data + m_cellSize * ((x - m_cache.x) * m_height + ystart),
                   ycount * m_cellSize);
        }            
    }

    if (fail) {
        primeCache(x, primeLeft); // this doesn't take effect until a later callback
        m_prevX = x;
        return false;
    }

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
MatrixFile::setColumnAt(size_t x, const void *data)
{
    assert(m_mode == ReadWrite);

#ifdef DEBUG_MATRIX_FILE_READ_SET
//    std::cerr << "MatrixFile::setColumnAt(" << x << ")" << std::endl;
    std::cerr << ".";
#endif

    ssize_t w = 0;
    bool seekFailed = false;
    
    {
        MutexLocker locker(&m_fdMutex, "MatrixFile::setColumnAt::m_fdMutex");

        if (seekTo(x, 0)) {
            w = ::write(m_fd, data, m_height * m_cellSize);
        } else {
            seekFailed = true;
        }
    }

    if (!seekFailed && w != ssize_t(m_height * m_cellSize)) {
        ::perror("WARNING: MatrixFile::setColumnAt: write failed");
        throw FileOperationFailed(m_fileName, "write");
    } else if (seekFailed) {
        throw FileOperationFailed(m_fileName, "seek");
    } else {
        MutexLocker locker
            (&m_columnBitsetWriteMutex,
             "MatrixFile::setColumnAt::m_columnBitsetWriteMutex");
        m_columnBitset->set(x);
    }
}

void
MatrixFile::suspend()
{
    MutexLocker locker(&m_fdMutex, "MatrixFile::suspend::m_fdMutex");
    MutexLocker locker2(&m_cacheMutex, "MatrixFile::suspend::m_cacheMutex");

    if (m_fd < 0) return; // already suspended

#ifdef DEBUG_MATRIX_FILE
    std::cerr << "MatrixFile(" << this << ":" << m_fileName.toStdString() << ")::suspend(): fd was " << m_fd << std::endl;
#endif

    if (m_requestToken >= 0) {
        void *data = 0;
        FileReadThread::Request request;
        if (m_readThread->getRequest(m_requestToken, request)) {
            data = request.data;
        }
        m_readThread->cancel(m_requestToken);
        if (data) free(data);
        m_requestToken = -1;
    }

    if (m_cache.data) {
        free(m_cache.data);
        m_cache.data = 0;
    }

    if (m_spareData) {
        free(m_spareData);
        m_spareData = 0;
    }
    
    if (::close(m_fd) < 0) {
        ::perror("WARNING: MatrixFile::suspend: close failed");
        throw FileOperationFailed(m_fileName, "close");
    }

    m_fd = -1;
}

void
MatrixFile::resume()
{
    if (m_fd >= 0) return;

#ifdef DEBUG_MATRIX_FILE    
    std::cerr << "MatrixFile(" << this << ")::resume()" << std::endl;
#endif

    if ((m_fd = ::open(m_fileName.toLocal8Bit(), m_flags, m_fmode)) < 0) {
        ::perror("Open failed");
        std::cerr << "ERROR: MatrixFile::resume: "
                  << "Failed to open cache file \""
                  << m_fileName.toStdString() << "\"";
        if (m_mode == ReadWrite) std::cerr << " for writing";
        std::cerr << std::endl;
        throw FailedToOpenFile(m_fileName);
    }

    std::cerr << "MatrixFile(" << this << ":" << m_fileName.toStdString() << ")::resume(): fd is " << m_fd << std::endl;
}

static int alloc = 0;

void
MatrixFile::primeCache(size_t x, bool goingLeft)
{
//    Profiler profiler("MatrixFile::primeCache");

#ifdef DEBUG_MATRIX_FILE_READ_SET
    std::cerr << "MatrixFile::primeCache(" << x << ", " << goingLeft << ")" << std::endl;
#endif

    size_t rx = x;
    size_t rw = m_defaultCacheWidth;

    size_t left = rw / 3;
    if (goingLeft) left = (rw * 2) / 3;

    if (rx > left) rx -= left;
    else rx = 0;

    if (rx + rw > m_width) rw = m_width - rx;

    if (!m_eagerCache) {

        size_t ti = 0;

        for (ti = 0; ti < rw; ++ti) {
            if (!m_columnBitset->get(rx + ti)) break;
        }
        
#ifdef DEBUG_MATRIX_FILE
        if (ti < rw) {
            std::cerr << "eagerCache is false and there's a hole at "
                      << rx + ti << ", reducing rw from " << rw << " to "
                      << ti << std::endl;
        }
#endif

        rw = std::min(rw, ti);
        if (rw < 10 || rx + rw <= x) return;
    }

    MutexLocker locker(&m_cacheMutex, "MatrixFile::primeCache::m_cacheMutex");

    FileReadThread::Request request;

    if (m_requestToken >= 0 &&
        m_readThread->getRequest(m_requestToken, request)) {

        if (x >= m_requestingX &&
            x <  m_requestingX + m_requestingWidth) {

            if (m_readThread->isReady(m_requestToken)) {

                if (!request.successful) {
                    std::cerr << "ERROR: MatrixFile::primeCache: Last request was unsuccessful" << std::endl;
                    throw FileReadFailed(m_fileName);
                }
                
#ifdef DEBUG_MATRIX_FILE_READ_SET
                std::cerr << "last request is ready! (" << m_requestingX << ", "<< m_requestingWidth << ")"  << std::endl;
#endif

                m_cache.x = (request.start - m_headerSize) / (m_height * m_cellSize);
                m_cache.width = request.size / (m_height * m_cellSize);
      
#ifdef DEBUG_MATRIX_FILE_READ_SET
                std::cerr << "received last request: actual size is: " << m_cache.x << ", " << m_cache.width << std::endl;
#endif

                if (m_cache.data) {
                    if (m_spareData) {
                        std::cerr << this << ": Freeing spare data" << std::endl;
                        free(m_spareData);
                    }
                    std::cerr << this << ": Moving old cache data to spare" << std::endl;
                    m_spareData = m_cache.data;
                }
                std::cerr << this << ": Moving request data to cache" << std::endl;
                m_cache.data = request.data;

                m_readThread->done(m_requestToken);
                m_requestToken = -1;
            }

            // already requested something covering this area; wait for it
            return;
        }

        // the current request is no longer of any use
        m_readThread->cancel(m_requestToken);

        // crude way to avoid leaking the data
        while (!m_readThread->isCancelled(m_requestToken)) {
            usleep(10000);
        }

#ifdef DEBUG_MATRIX_FILE_READ_SET
        std::cerr << "cancelled " << m_requestToken << std::endl;
#endif

        if (m_spareData) {
            std::cerr << this << ": Freeing spare data" << std::endl;
            free(m_spareData);
        }
        std::cerr << this << ": Moving request data to spare" << std::endl;
        m_spareData = request.data;
        m_readThread->done(m_requestToken);

        m_requestToken = -1;
    }

    if (m_fd < 0) {
        MutexLocker locker(&m_fdMutex, "MatrixFile::primeCache::m_fdMutex");
        if (m_fd < 0) resume();
    }

    request.fd = m_fd;
    request.mutex = &m_fdMutex;
    request.start = m_headerSize + rx * m_height * m_cellSize;
    request.size = rw * m_height * m_cellSize;

    std::cerr << this << ": Moving spare data to request, and resizing to " << rw * m_height * m_cellSize << std::endl;

    request.data = (char *)realloc(m_spareData, rw * m_height * m_cellSize);
    MUNLOCK(request.data, rw * m_height * m_cellSize);
    m_spareData = 0;

    m_requestingX = rx;
    m_requestingWidth = rw;

    int token = m_readThread->request(request);
#ifdef DEBUG_MATRIX_FILE_READ_SET
    std::cerr << "MatrixFile::primeCache: request token is "
              << token << " (x = [" << rx << "], w = [" << rw << "], left = [" << goingLeft << "])" << std::endl;
#endif
    m_requestToken = token;
}

bool
MatrixFile::seekTo(size_t x, size_t y)
{
    if (m_fd < 0) resume();

    off_t off = m_headerSize + (x * m_height + y) * m_cellSize;

    if (::lseek(m_fd, off, SEEK_SET) == (off_t)-1) {
        ::perror("Seek failed");
        std::cerr << "ERROR: MatrixFile::seekTo(" << x << ", " << y
                  << ") failed" << std::endl;
        return false;
    }

    return true;
}

