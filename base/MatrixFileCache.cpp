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
#include "base/System.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>

#include <cstdio>

#include <QFileInfo>
#include <QDir>

#define HAVE_MMAP 1

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

//!!! This class is a work in progress -- it does only as much as we
// need for the current SpectrogramLayer.  Slated for substantial
// refactoring and extension.

MatrixFileCache::MatrixFileCache(QString fileBase, Mode mode) :
    m_fd(-1),
    m_mode(mode),
    m_width(0),
    m_height(0),
    m_headerSize(2 * sizeof(size_t)),
    m_autoRegionWidth(2048),
    m_off(-1),
    m_rx(0),
    m_rw(0),
    m_userRegion(false),
    m_region(0),
    m_mmapped(false),
    m_mmapSize(0),
    m_mmapOff(0),
    m_preferMmap(true)
{
    // Ensure header size is a multiple of the size of our data (for
    // alignment purposes)
    size_t hs = ((m_headerSize / sizeof(float)) * sizeof(float));
    if (hs != m_headerSize) m_headerSize = hs + sizeof(float);

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

    if ((m_fd = ::open(fileName.toLocal8Bit(), flags, fmode)) < 0) {
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
        if (::read(m_fd, header, 2 * sizeof(size_t)) < 0) {
            perror("Read failed");
            std::cerr << "ERROR: MatrixFileCache::MatrixFileCache: "
                      << "Failed to read header (fd " << m_fd << ", file \""
                      << fileName.toStdString() << "\")" << std::endl;
            return;
        }
        m_width = header[0];
        m_height = header[1];
        seekTo(0, 0);
    }

    std::cerr << "MatrixFileCache::MatrixFileCache: Done, size is " << "(" << m_width << ", " << m_height << ")" << std::endl;

}

MatrixFileCache::~MatrixFileCache()
{
    if (m_rw > 0) {
        if (m_mmapped) {
#ifdef HAVE_MMAP
            ::munmap(m_region, m_mmapSize);
#endif
        } else {
            delete[] m_region;
        }
    }

    if (m_fd >= 0) {
        if (::close(m_fd) < 0) {
            ::perror("MatrixFileCache::~MatrixFileCache: close failed");
        }
    }

    //!!! refcount and unlink
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

#ifdef HAVE_MMAP
        // If we're going to mmap the file, we need to ensure it's long
        // enough beforehand
        
        if (m_preferMmap) {
        
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
        }
#endif

    } else {
        
        if (::ftruncate(m_fd, off) < 0) {
            ::perror("WARNING: MatrixFileCache::resize: ftruncate failed");
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
    
    float *emptyCol = new float[m_height];
    for (size_t y = 0; y < m_height; ++y) emptyCol[y] = 0.f;

    seekTo(0, 0);
    for (size_t x = 0; x < m_width; ++x) setColumnAt(x, emptyCol);
    
    delete[] emptyCol;
}

void
MatrixFileCache::setRegionOfInterest(size_t x, size_t width)
{
    setRegion(x, width, true);
}

void
MatrixFileCache::clearRegionOfInterest()
{
    m_userRegion = false;
}

float
MatrixFileCache::getValueAt(size_t x, size_t y) const
{
    if (m_rw > 0 && x >= m_rx && x < m_rx + m_rw) {
        float *rp = getRegionPtr(x, y);
        if (rp) return *rp;
    } else if (!m_userRegion) {
        if (autoSetRegion(x)) {
            float *rp = getRegionPtr(x, y);
            if (rp) return *rp;
        }
    }

    if (!seekTo(x, y)) return 0.f;
    float value;
    ssize_t r = ::read(m_fd, &value, sizeof(float));
    if (r != sizeof(float)) {
        ::perror("MatrixFileCache::getValueAt: read failed");
        value = 0.f;
    }
    if (r > 0) m_off += r;
    return value;
}

void
MatrixFileCache::getColumnAt(size_t x, float *values) const
{
    if (m_rw > 0 && x >= m_rx && x < m_rx + m_rw) {
        float *rp = getRegionPtr(x, 0);
        if (rp) {
            for (size_t y = 0; y < m_height; ++y) {
                values[y] = rp[y];
            }
            return;
        }
    } else if (!m_userRegion) {
        if (autoSetRegion(x)) {
            float *rp = getRegionPtr(x, 0);
            if (rp) {
                for (size_t y = 0; y < m_height; ++y) {
                    values[y] = rp[y];
                }
                return;
            }
        }
    }

    if (!seekTo(x, 0)) return;
    ssize_t r = ::read(m_fd, values, m_height * sizeof(float));
    if (r != m_height * sizeof(float)) {
        ::perror("MatrixFileCache::getColumnAt: read failed");
    }
    if (r > 0) m_off += r;
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
    ssize_t w = ::write(m_fd, &value, sizeof(float));
    if (w != sizeof(float)) {
        ::perror("WARNING: MatrixFileCache::setValueAt: write failed");
    }
    if (w > 0) m_off += w;

    //... update region as appropriate
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
    ssize_t w = ::write(m_fd, values, m_height * sizeof(float));
    if (w != m_height * sizeof(float)) {
        ::perror("WARNING: MatrixFileCache::setColumnAt: write failed");
    }
    if (w > 0) m_off += w;

    //... update region as appropriate
}

float *
MatrixFileCache::getRegionPtr(size_t x, size_t y) const
{
    if (m_rw == 0) return 0;

    float *region = m_region;

    if (m_mmapOff > 0) {
        char *cr = (char *)m_region;
        cr += m_mmapOff;
        region = (float *)cr;
    }

    float *ptr = &(region[(x - m_rx) * m_height + y]);
    
//    std::cerr << "getRegionPtr(" << x << "," << y << "): region is " << m_region << ", returning " << ptr << std::endl;
    return ptr;
}

bool
MatrixFileCache::autoSetRegion(size_t x) const
{
    size_t rx = x;
    size_t rw = m_autoRegionWidth;
    size_t left = rw / 4;
    if (x < m_rx) left = (rw * 3) / 4;
    if (rx > left) rx -= left;
    else rx = 0;
    if (rx + rw > m_width) rw = m_width - rx;
    return setRegion(rx, rw, false);
}

bool
MatrixFileCache::setRegion(size_t x, size_t width, bool user) const
{
    if (!user && m_userRegion) return false;
    if (m_rw > 0 && x >= m_rx && x + width <= m_rx + m_rw) return true;

    if (m_rw > 0) {
        if (m_mmapped) {
#ifdef HAVE_MMAP
            ::munmap(m_region, m_mmapSize);
            std::cerr << "unmapped " << m_mmapSize << " at " << m_region << std::endl;
#endif
        } else {
            delete[] m_region;
        }
        m_region = 0;
        m_mmapped = false;
        m_mmapSize = 0;
        m_mmapOff = 0;
        m_rw = 0;
    }

    if (width == 0) {
        return true;
    }

#ifdef HAVE_MMAP

    if (m_preferMmap) {

        size_t mmapSize = m_height * width * sizeof(float);
        off_t offset = m_headerSize + (x * m_height) * sizeof(float);
        int pagesize = getpagesize();
        off_t aligned = (offset / pagesize) * pagesize;
        size_t mmapOff = offset - aligned;
        mmapSize += mmapOff;
        
        m_region = (float *)
            ::mmap(0, mmapSize, PROT_READ, MAP_PRIVATE, m_fd, aligned);
        
        if (m_region == MAP_FAILED) {
            
            ::perror("Mmap failed");
            std::cerr << "ERROR: MatrixFileCache::setRegion(" << x << ", "
                      << width << "): Mmap(0, " << mmapSize
                      << ", " << PROT_READ << ", " << MAP_SHARED << ", " << m_fd 
                      << ", " << aligned << ") failed, falling back to "
                      << "non-mmapping code for this cache" << std::endl;
            m_preferMmap = false;
            
        } else {

            std::cerr << "mmap succeeded (offset " << aligned << ", size " << mmapSize << ", m_mmapOff " << mmapOff << ") = " << m_region << std::endl;
            
            m_mmapped = true;
            m_mmapSize = mmapSize;
            m_mmapOff = mmapOff;
            m_rx = x;
            m_rw = width;
            if (user) m_userRegion = true;
//            MUNLOCK(m_region, m_mmapSize);
            return true;
        }
    }
#endif

    if (!seekTo(x, 0)) return false;

    m_region = new float[width * m_height];

    ssize_t r = ::read(m_fd, m_region, width * m_height * sizeof(float));
    if (r < 0) {
        ::perror("Read failed");
        std::cerr << "ERROR: MatrixFileCache::setRegion(" << x << ", " << width
                  << ") failed" << std::endl;
        delete[] m_region;
        m_region = 0;
        return false;
    }
    
    m_off += r;

    if (r < width * m_height * sizeof(float)) {
        // didn't manage to read the whole thing, but did get something
        std::cerr << "WARNING: MatrixFileCache::setRegion(" << x << ", " << width
                  << "): ";
        width = r / (m_height * sizeof(float));
        std::cerr << "Only got " << width << " columns" << std::endl;
    }

    m_rx = x;
    m_rw = width;
    if (m_rw == 0) {
        delete[] m_region;
        m_region = 0;
    }

    std::cerr << "MatrixFileCache::setRegion: set region to " << x << ", " << width << std::endl;

    if (user) m_userRegion = true;
    if (m_rw > 0) MUNLOCK(m_region, m_rw * m_height);
    return true;
}

bool
MatrixFileCache::seekTo(size_t x, size_t y) const
{
    off_t off = m_headerSize + (x * m_height + y) * sizeof(float);
    if (off == m_off) return true;

    if (m_mode == ReadWrite) {
        std::cerr << "writer: ";
        std::cerr << "seek required (from " << m_off << " to " << off << ")" << std::endl;
    }

    if (::lseek(m_fd, off, SEEK_SET) == (off_t)-1) {
        ::perror("Seek failed");
        std::cerr << "ERROR: MatrixFileCache::seekTo(" << x << ", " << y
                  << ") failed" << std::endl;
        return false;
    }

    m_off = off;
    return true;
}

