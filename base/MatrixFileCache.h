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

#ifndef _MATRIX_FILE_CACHE_H_
#define _MATRIX_FILE_CACHE_H_

#include <sys/types.h>
#include <QString>

// This class is _not_ thread safe.  Each instance must only be used
// within a single thread.  You may however have as many instances as
// you like referring to the same file in separate threads.

class MatrixFileCache
{
public:
    enum Mode { ReadOnly, ReadWrite };

    MatrixFileCache(QString fileBase, Mode mode);
    virtual ~MatrixFileCache();

    size_t getWidth() const;
    size_t getHeight() const;
    
    void resize(size_t width, size_t height);
    void reset();

    void setRegionOfInterest(size_t x, size_t width);
    void clearRegionOfInterest();

    float getValueAt(size_t x, size_t y) const;
    void getColumnAt(size_t x, float *values) const;

    void setValueAt(size_t x, size_t y, float value);
    void setColumnAt(size_t x, float *values);
    
protected:
    int     m_fd;
    Mode    m_mode;
    size_t  m_width;
    size_t  m_height;
    size_t  m_headerSize;
    size_t  m_autoRegionWidth;

    mutable off_t   m_off;
    mutable size_t  m_rx;
    mutable size_t  m_rw;
    mutable bool    m_userRegion;
    mutable float  *m_region;
    mutable bool    m_mmapped;
    mutable size_t  m_mmapSize;
    mutable size_t  m_mmapOff;
    mutable bool    m_preferMmap;
    float *getRegionPtr(size_t x, size_t y) const;

    bool autoSetRegion(size_t x) const;
    bool setRegion(size_t x, size_t width, bool user) const;

    bool seekTo(size_t x, size_t y) const;
};

#endif

