/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _FFT_FILE_CACHE_H_
#define _FFT_FILE_CACHE_H_

#include "FFTCache.h"
#include "fileio/MatrixFile.h"

#include <QMutex>

class FFTFileCache : public FFTCache
{
public:
    enum StorageType {
        Compact, // 16 bits normalized polar
        Rectangular, // floating point real+imag
        Polar, // floating point mag+phase
    };

    FFTFileCache(QString fileBase, MatrixFile::Mode mode,
                 StorageType storageType);
    virtual ~FFTFileCache();

    MatrixFile::Mode getMode() const { return m_mfc->getMode(); }

    virtual size_t getWidth() const;
    virtual size_t getHeight() const;
	
    virtual void resize(size_t width, size_t height);
    virtual void reset(); // zero-fill or 1-fill as appropriate without changing size
	
    virtual float getMagnitudeAt(size_t x, size_t y) const;
    virtual float getNormalizedMagnitudeAt(size_t x, size_t y) const;
    virtual float getMaximumMagnitudeAt(size_t x) const;
    virtual float getPhaseAt(size_t x, size_t y) const;

    virtual void getValuesAt(size_t x, size_t y, float &real, float &imag) const;

    virtual bool haveSetColumnAt(size_t x) const;

    virtual void setColumnAt(size_t x, float *mags, float *phases, float factor);
    virtual void setColumnAt(size_t x, float *reals, float *imags);

    virtual void suspend() { m_mfc->suspend(); }

    static size_t getCacheSize(size_t width, size_t height, StorageType type);

    virtual Type getType() { return FileCache; }

protected:
    char *m_writebuf;
    mutable char *m_readbuf;
    mutable size_t m_readbufCol;
    mutable size_t m_readbufWidth;

    float getFromReadBufStandard(size_t x, size_t y) const {
        if (m_readbuf &&
            (m_readbufCol == x || (m_readbufWidth > 1 && m_readbufCol+1 == x))) {
            return ((float *)m_readbuf)[(x - m_readbufCol) * m_mfc->getHeight() + y];
        } else {
            populateReadBuf(x);
            return getFromReadBufStandard(x, y);
        }
    }

    float getFromReadBufCompactUnsigned(size_t x, size_t y) const {
        if (m_readbuf &&
            (m_readbufCol == x || (m_readbufWidth > 1 && m_readbufCol+1 == x))) {
            return ((uint16_t *)m_readbuf)[(x - m_readbufCol) * m_mfc->getHeight() + y];
        } else {
            populateReadBuf(x);
            return getFromReadBufCompactUnsigned(x, y);
        }
    }

    float getFromReadBufCompactSigned(size_t x, size_t y) const {
        if (m_readbuf &&
            (m_readbufCol == x || (m_readbufWidth > 1 && m_readbufCol+1 == x))) {
            return ((int16_t *)m_readbuf)[(x - m_readbufCol) * m_mfc->getHeight() + y];
        } else {
            populateReadBuf(x);
            return getFromReadBufCompactSigned(x, y);
        }
    }

    void populateReadBuf(size_t x) const;

    float getNormalizationFactor(size_t col) const {
        size_t h = m_mfc->getHeight();
        if (h < m_factorSize) return 0;
        if (m_storageType != Compact) {
            return getFromReadBufStandard(col, h - 1);
        } else {
            union {
                float f;
                uint16_t u[2];
            } factor;
            if (!m_readbuf ||
                !(m_readbufCol == col ||
                  (m_readbufWidth > 1 && m_readbufCol+1 == col))) {
                populateReadBuf(col);
            }
            size_t ix = (col - m_readbufCol) * m_mfc->getHeight() + h;
            factor.u[0] = ((uint16_t *)m_readbuf)[ix - 2];
            factor.u[1] = ((uint16_t *)m_readbuf)[ix - 1];
            return factor.f;
        }
    }

    void setNormalizationFactorToWritebuf(float newfactor) {
        size_t h = m_mfc->getHeight();
        if (h < m_factorSize) return;
        if (m_storageType != Compact) {
            ((float *)m_writebuf)[h - 1] = newfactor;
        } else {
            union {
                float f;
                uint16_t u[2];
            } factor;
            factor.f = newfactor;
            ((uint16_t *)m_writebuf)[h - 2] = factor.u[0];
            ((uint16_t *)m_writebuf)[h - 1] = factor.u[1];
        }
    }            

    MatrixFile *m_mfc;
    QMutex m_writeMutex;
    StorageType m_storageType;
    size_t m_factorSize;
};

#endif
