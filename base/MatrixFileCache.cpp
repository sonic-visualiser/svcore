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

#include "MatrixFileCache.h"
#include "base/TempDirectory.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>

#include <cstdio>

#include <QFileInfo>
#include <QDir>

MatrixFileCache::MatrixFileCache(QString fileBase, Mode mode) :
    m_fd(-1),
    m_off(-1),
    m_mode(mode),
    m_width(0),
    m_height(0),
    m_rx(0),
    m_rw(0),
    m_range(0),
    m_headerSize(2 * sizeof(size_t))
{
    QDir tempDir(TempDirectory::instance()->getPath());
    QString fileName(tempDir.filePath(QString("%1.mfc").arg(fileBase)));
    bool newFile = !QFileInfo(fileName).exists();

    if (newFile && mode == ReadOnly) {
        std::cerr << "ERROR: MatrixFileCache::MatrixFileCache: Read-only mode "
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

    if ((m_fd = ::open(fileName.toLocal8Bit(), flags, mode)) < 0) {
        ::perror("Open failed");
        std::cerr << "ERROR: MatrixFileCache::MatrixFileCache: "
                  << "Failed to open cache file \""
                  << fileName.toStdString() << "\"";
        if (mode == ReadWrite) std::cerr << " for writing";
        std::cerr << std::endl;
    }

    if (newFile) {
        resize(0, 0); // write header
    } else {
        size_t header[2];
        if (::read(m_fd, header, 2 * sizeof(size_t))) {
            perror("Read failed");
            std::cerr << "ERROR: MatrixFileCache::MatrixFileCache: "
                      << "Failed to read header" << std::endl;
            return;
        }
        m_width = header[0];
        m_height = header[1];
        seekTo(0, 0);
    }

    std::cerr << "MatrixFileCache::MatrixFileCache: Done, size is " << m_width << "x" << m_height << std::endl;

}

MatrixFileCache::~MatrixFileCache()
{
    if (m_fd >= 0) {
        if (::close(m_fd) < 0) {
            ::perror("MatrixFileCache::~MatrixFileCache: close failed");
        }
    }
}

size_t 
MatrixFileCache::getWidth() const
{
    return m_width;
}

size_t
MatrixFileCache::getHeight() const
{
    return m_height;
}

void
MatrixFileCache::resize(size_t w, size_t h)
{
    if (m_mode != ReadWrite) {
        std::cerr << "ERROR: MatrixFileCache::resize called on read-only cache"
                  << std::endl;
        return;
    }

    off_t off = m_headerSize + (w * h * sizeof(float));

    if (w * h > m_width * m_height) {

        if (::lseek(m_fd, off - sizeof(float), SEEK_SET) == (off_t)-1) {
            ::perror("Seek failed");
            std::cerr << "ERROR: MatrixFileCache::resize(" << w << ", "
                      << h << "): seek failed, cannot resize" << std::endl;
            return;
        }

        // guess this requires efficient support for sparse files
        
        float f(0);
        if (::write(m_fd, &f, sizeof(float)) != sizeof(float)) {
            ::perror("WARNING: MatrixFileCache::resize: write failed");
        }

    } else {
        
        if (::ftruncate(m_fd, off) < 0) {
            ::perror("MatrixFileCache::resize: ftruncate failed");
        }
    }

    m_width = 0;
    m_height = 0;
    m_off = 0;

    if (::lseek(m_fd, 0, SEEK_SET) == (off_t)-1) {
        ::perror("ERROR: MatrixFileCache::resize: Seek to write header failed");
        return;
    }

    size_t header[2];
    header[0] = w;
    header[1] = h;
    if (::write(m_fd, header, 2 * sizeof(size_t)) != 2 * sizeof(size_t)) {
        ::perror("ERROR: MatrixFileCache::resize: Failed to write header");
        return;
    }

    m_width = w;
    m_height = h;

    seekTo(0, 0);
}

void
MatrixFileCache::reset()
{
    if (m_mode != ReadWrite) {
        std::cerr << "ERROR: MatrixFileCache::reset called on read-only cache"
                  << std::endl;
        return;
    }
    
    //...
}

void
MatrixFileCache::setRangeOfInterest(size_t x, size_t width)
{
}

float
MatrixFileCache::getValueAt(size_t x, size_t y) const
{
    if (m_rw > 0 && x >= m_rx && x < m_rx + m_rw) {
        return m_range[x - m_rx][y];
    }

    if (!seekTo(x, y)) return 0.f;
    float value;
    if (::read(m_fd, &value, sizeof(float)) != sizeof(float)) {
        ::perror("MatrixFileCache::getValueAt: read failed");
        return 0.f;
    }
    return value;
}

void
MatrixFileCache::getColumnAt(size_t x, float *values) const
{
    if (m_rw > 0 && x >= m_rx && x < m_rx + m_rw) {
        for (size_t y = 0; y < m_height; ++y) {
            values[y] = m_range[x - m_rx][y];
        }
    }

    if (!seekTo(x, 0)) return;
    if (::read(m_fd, values, m_height * sizeof(float)) != m_height * sizeof(float)) {
        ::perror("MatrixFileCache::getColumnAt: read failed");
    }
    return;
}

void
MatrixFileCache::setValueAt(size_t x, size_t y, float value)
{
    if (m_mode != ReadWrite) {
        std::cerr << "ERROR: MatrixFileCache::setValueAt called on read-only cache"
                  << std::endl;
        return;
    }

    if (!seekTo(x, y)) return;
    if (::write(m_fd, &value, sizeof(float)) != sizeof(float)) {
        ::perror("WARNING: MatrixFileCache::setValueAt: write failed");
    }

    //... update range as appropriate
}

void
MatrixFileCache::setColumnAt(size_t x, float *values)
{
    if (m_mode != ReadWrite) {
        std::cerr << "ERROR: MatrixFileCache::setColumnAt called on read-only cache"
                  << std::endl;
        return;
    }

    if (!seekTo(x, 0)) return;
    if (::write(m_fd, values, m_height * sizeof(float)) != m_height * sizeof(float)) {
        ::perror("WARNING: MatrixFileCache::setColumnAt: write failed");
    }

    //... update range as appropriate
}

bool
MatrixFileCache::seekTo(size_t x, size_t y) const
{
    off_t off = m_headerSize + (x * m_height + y) * sizeof(float);
    if (off == m_off) return true;

    if (::lseek(m_fd, off, SEEK_SET) == (off_t)-1) {
        ::perror("Seek failed");
        std::cerr << "ERROR: MatrixFileCache::seekTo(" << x << ", " << y
                  << ") failed" << std::endl;
        return false;
    }

    m_off = off;
    return true;
}

