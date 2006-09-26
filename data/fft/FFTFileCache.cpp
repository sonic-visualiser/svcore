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

#include "fileio/MatrixFile.h"

#include "base/Profiler.h"

#include <iostream>

#include <QMutexLocker>

// The underlying matrix has height (m_height * 2 + 1).  In each
// column we store magnitude at [0], [2] etc and phase at [1], [3]
// etc, and then store the normalization factor (maximum magnitude) at
// [m_height * 2].

FFTFileCache::FFTFileCache(QString fileBase, MatrixFile::Mode mode,
                           StorageType storageType) :
    m_writebuf(0),
    m_readbuf(0),
    m_readbufCol(0),
    m_readbufWidth(0),
    m_mfc(new MatrixFile
          (fileBase, mode, 
           storageType == Compact ? sizeof(uint16_t) : sizeof(float),
           mode == MatrixFile::ReadOnly)),
    m_storageType(storageType)
{
    std::cerr << "FFTFileCache: storage type is " << (storageType == Compact ? "Compact" : storageType == Polar ? "Polar" : "Rectangular") << std::endl;
}

FFTFileCache::~FFTFileCache()
{
    if (m_readbuf) delete[] m_readbuf;
    if (m_writebuf) delete[] m_writebuf;
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
    QMutexLocker locker(&m_writeMutex);

    m_mfc->resize(width, height * 2 + 1);
    if (m_readbuf) {
        delete[] m_readbuf;
        m_readbuf = 0;
    }
    if (m_writebuf) {
        delete[] m_writebuf;
    }
    m_writebuf = new char[(height * 2 + 1) * m_mfc->getCellSize()];
}

void
FFTFileCache::reset()
{
    m_mfc->reset();
}

float
FFTFileCache::getMagnitudeAt(size_t x, size_t y) const
{
    float value = 0.f;

    switch (m_storageType) {

    case Compact:
        value = (getFromReadBufCompactUnsigned(x, y * 2) / 65535.0)
            * getNormalizationFactor(x);
        break;

    case Rectangular:
    {
        float real, imag;
        getValuesAt(x, y, real, imag);
        value = sqrtf(real * real + imag * imag);
        break;
    }

    case Polar:
        value = getFromReadBufStandard(x, y * 2);
        break;
    }

    return value;
}

float
FFTFileCache::getNormalizedMagnitudeAt(size_t x, size_t y) const
{
    float value = 0.f;

    switch (m_storageType) {

    case Compact:
        value = getFromReadBufCompactUnsigned(x, y * 2) / 65535.0;
        break;

    default:
    {
        float mag = getMagnitudeAt(x, y);
        float factor = getNormalizationFactor(x);
        if (factor != 0) value = mag / factor;
        else value = 0.f;
        break;
    }
    }

    return value;
}

float
FFTFileCache::getMaximumMagnitudeAt(size_t x) const
{
    return getNormalizationFactor(x);
}

float
FFTFileCache::getPhaseAt(size_t x, size_t y) const
{
    float value = 0.f;
    
    switch (m_storageType) {

    case Compact:
        value = (getFromReadBufCompactSigned(x, y * 2 + 1) / 32767.0) * M_PI;
        break;

    case Rectangular:
    {
        float real, imag;
        getValuesAt(x, y, real, imag);
        value = princargf(atan2f(imag, real));
        break;
    }

    case Polar:
        value = getFromReadBufStandard(x, y * 2 + 1);
        break;
    }

    return value;
}

void
FFTFileCache::getValuesAt(size_t x, size_t y, float &real, float &imag) const
{
    switch (m_storageType) {

    case Rectangular:
        real = getFromReadBufStandard(x, y * 2);
        imag = getFromReadBufStandard(x, y * 2 + 1);
        return;

    default:
        float mag = getMagnitudeAt(x, y);
        float phase = getPhaseAt(x, y);
        real = mag * cosf(phase);
        imag = mag * sinf(phase);
        return;
    }
}

bool
FFTFileCache::haveSetColumnAt(size_t x) const
{
    return m_mfc->haveSetColumnAt(x);
}

void
FFTFileCache::setColumnAt(size_t x, float *mags, float *phases, float factor)
{
    QMutexLocker locker(&m_writeMutex);

    size_t h = getHeight();

    switch (m_storageType) {

    case Compact:
        for (size_t y = 0; y < h; ++y) {
            ((uint16_t *)m_writebuf)[y * 2] = uint16_t((mags[y] / factor) * 65535.0);
            ((uint16_t *)m_writebuf)[y * 2 + 1] = uint16_t(int16_t((phases[y] * 32767) / M_PI));
        }
        break;

    case Rectangular:
        for (size_t y = 0; y < h; ++y) {
            ((float *)m_writebuf)[y * 2] = mags[y] * cosf(phases[y]);
            ((float *)m_writebuf)[y * 2 + 1] = mags[y] * sinf(phases[y]);
        }
        break;

    case Polar:
        for (size_t y = 0; y < h; ++y) {
            ((float *)m_writebuf)[y * 2] = mags[y];
            ((float *)m_writebuf)[y * 2 + 1] = phases[y];
        }
        break;
    }

    static float maxFactor = 0;
    if (factor > maxFactor) maxFactor = factor;
//    std::cerr << "Normalization factor: " << factor << ", max " << maxFactor << " (height " << getHeight() << ")" << std::endl;

    if (m_storageType == Compact) {
        ((uint16_t *)m_writebuf)[h * 2] = factor * 65535.0;
    } else {
        ((float *)m_writebuf)[h * 2] = factor;
    }
    m_mfc->setColumnAt(x, m_writebuf);
}

void
FFTFileCache::setColumnAt(size_t x, float *real, float *imag)
{
    QMutexLocker locker(&m_writeMutex);

    size_t h = getHeight();

    float max = 0.0f;

    switch (m_storageType) {

    case Compact:
        for (size_t y = 0; y < h; ++y) {
            float mag = sqrtf(real[y] * real[y] + imag[y] * imag[y]);
            if (mag > max) max = mag;
        }
        for (size_t y = 0; y < h; ++y) {
            float mag = sqrtf(real[y] * real[y] + imag[y] * imag[y]);
            float phase = princargf(atan2f(imag[y], real[y]));
            ((uint16_t *)m_writebuf)[y * 2] = uint16_t((mag / max) * 65535.0);
            ((uint16_t *)m_writebuf)[y * 2 + 1] = uint16_t(int16_t((phase * 32767) / M_PI));
        }
        break;

    case Rectangular:
        for (size_t y = 0; y < h; ++y) {
            ((float *)m_writebuf)[y * 2] = real[y];
            ((float *)m_writebuf)[y * 2 + 1] = imag[y];
            float mag = sqrtf(real[y] * real[y] + imag[y] * imag[y]);
            if (mag > max) max = mag;
        }
        break;

    case Polar:
        for (size_t y = 0; y < h; ++y) {
            float mag = sqrtf(real[y] * real[y] + imag[y] * imag[y]);
            if (mag > max) max = mag;
            ((float *)m_writebuf)[y * 2] = mag;
            ((float *)m_writebuf)[y * 2 + 1] = princargf(atan2f(imag[y], real[y]));
        }
        break;
    }

    ((float *)m_writebuf)[h * 2] = max;
    m_mfc->setColumnAt(x, m_writebuf);
}

size_t
FFTFileCache::getCacheSize(size_t width, size_t height, StorageType type)
{
    return (height * 2 + 1) * width *
        (type == Compact ? sizeof(uint16_t) : sizeof(float)) +
        2 * sizeof(size_t); // matrix file header size
}

