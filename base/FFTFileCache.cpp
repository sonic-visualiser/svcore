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

#include "FFTFileCache.h"

#include "MatrixFile.h"

#include <iostream>

//!!! This class is a work in progress -- it does only as much as we
// need for the current SpectrogramLayer.  Slated for substantial
// refactoring and extension.

// The underlying matrix has height (m_height * 2 + 1).  In each
// column we store magnitude at [0], [2] etc and phase at [1], [3]
// etc, and then store the normalization factor (maximum magnitude) at
// [m_height * 2].

FFTFileCache::FFTFileCache(QString fileBase, MatrixFile::Mode mode) :
    m_colbuf(0),
    m_mfc(new MatrixFile(fileBase, mode))
{
}

FFTFileCache::~FFTFileCache()
{
    delete m_colbuf;
    delete m_mfc;
}

size_t
FFTFileCache::getWidth() const
{
    return m_mfc->getWidth();
}

size_t
FFTFileCache::getHeight() const
{
    size_t mh = m_mfc->getHeight();
    if (mh > 0) return (mh - 1) / 2;
    else return 0;
}

void
FFTFileCache::resize(size_t width, size_t height)
{
    m_mfc->resize(width, height * 2 + 1);
    delete m_colbuf;
    m_colbuf = new float[height * 2 + 1];
}

void
FFTFileCache::reset()
{
    m_mfc->reset();
}

float
FFTFileCache::getMagnitudeAt(size_t x, size_t y) const
{
    return m_mfc->getValueAt(x, y * 2);
}

float
FFTFileCache::getNormalizedMagnitudeAt(size_t x, size_t y) const
{
    float factor = m_mfc->getValueAt(x, m_mfc->getHeight() - 1);
    float mag = m_mfc->getValueAt(x, y * 2);
    if (factor != 0) return mag / factor;
    else return 0.f;
}

float
FFTFileCache::getPhaseAt(size_t x, size_t y) const
{
    return m_mfc->getValueAt(x, y * 2 + 1);
}

void
FFTFileCache::setNormalizationFactor(size_t x, float factor)
{
    m_mfc->setValueAt(x, m_mfc->getHeight() - 1, factor);
}

void
FFTFileCache::setMagnitudeAt(size_t x, size_t y, float mag)
{
    m_mfc->setValueAt(x, y * 2, mag);
}

void
FFTFileCache::setNormalizedMagnitudeAt(size_t x, size_t y, float norm)
{
    float factor = m_mfc->getValueAt(x, m_mfc->getHeight() - 1);
    m_mfc->setValueAt(x, y * 2, norm * factor);
}

void
FFTFileCache::setPhaseAt(size_t x, size_t y, float phase)
{
    m_mfc->setValueAt(x, y * 2 + 1, phase);
}

void
FFTFileCache::setColumnAt(size_t x, float *mags, float *phases, float factor)
{
    size_t h = getHeight();
    for (size_t y = 0; y < h; ++y) {
        m_colbuf[y * 2] = mags[y];
        m_colbuf[y * 2 + 1] = phases[y];
    }
    m_colbuf[h * 2] = factor;
    m_mfc->setColumnAt(x, m_colbuf);
}

