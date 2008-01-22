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

#include "FFTMemoryCache.h"
#include "system/System.h"

#include <iostream>

FFTMemoryCache::FFTMemoryCache(StorageType storageType) :
    m_width(0),
    m_height(0),
    m_magnitude(0),
    m_phase(0),
    m_fmagnitude(0),
    m_fphase(0),
    m_freal(0),
    m_fimag(0),
    m_factor(0),
    m_storageType(storageType)
{
    std::cerr << "FFTMemoryCache[" << this << "]::FFTMemoryCache (type "
              << m_storageType << ")" << std::endl;
}

FFTMemoryCache::~FFTMemoryCache()
{
//    std::cerr << "FFTMemoryCache[" << this << "]::~FFTMemoryCache" << std::endl;

    for (size_t i = 0; i < m_width; ++i) {
	if (m_magnitude && m_magnitude[i]) free(m_magnitude[i]);
	if (m_phase && m_phase[i]) free(m_phase[i]);
	if (m_fmagnitude && m_fmagnitude[i]) free(m_fmagnitude[i]);
	if (m_fphase && m_fphase[i]) free(m_fphase[i]);
        if (m_freal && m_freal[i]) free(m_freal[i]);
        if (m_fimag && m_fimag[i]) free(m_fimag[i]);
    }

    if (m_magnitude) free(m_magnitude);
    if (m_phase) free(m_phase);
    if (m_fmagnitude) free(m_fmagnitude);
    if (m_fphase) free(m_fphase);
    if (m_freal) free(m_freal);
    if (m_fimag) free(m_fimag);
    if (m_factor) free(m_factor);
}

void
FFTMemoryCache::resize(size_t width, size_t height)
{
    std::cerr << "FFTMemoryCache[" << this << "]::resize(" << width << "x" << height << " = " << width*height << ")" << std::endl;
    
    if (m_width == width && m_height == height) return;

    if (m_storageType == Compact) {
        resize(m_magnitude, width, height);
        resize(m_phase, width, height);
    } else if (m_storageType == Polar) {
        resize(m_fmagnitude, width, height);
        resize(m_fphase, width, height);
    } else {
        resize(m_freal, width, height);
        resize(m_fimag, width, height);
    }

    m_colset.resize(width);

    m_factor = (float *)realloc(m_factor, width * sizeof(float));

    m_width = width;
    m_height = height;

//    std::cerr << "done, width = " << m_width << " height = " << m_height << std::endl;
}

void
FFTMemoryCache::resize(uint16_t **&array, size_t width, size_t height)
{
    for (size_t i = width; i < m_width; ++i) {
	free(array[i]);
    }

    if (width != m_width) {
	array = (uint16_t **)realloc(array, width * sizeof(uint16_t *));
	if (!array) throw std::bad_alloc();
	MUNLOCK(array, width * sizeof(uint16_t *));
    }

    for (size_t i = m_width; i < width; ++i) {
	array[i] = 0;
    }

    for (size_t i = 0; i < width; ++i) {
	array[i] = (uint16_t *)realloc(array[i], height * sizeof(uint16_t));
	if (!array[i]) throw std::bad_alloc();
	MUNLOCK(array[i], height * sizeof(uint16_t));
    }
}

void
FFTMemoryCache::resize(float **&array, size_t width, size_t height)
{
    for (size_t i = width; i < m_width; ++i) {
	free(array[i]);
    }

    if (width != m_width) {
	array = (float **)realloc(array, width * sizeof(float *));
	if (!array) throw std::bad_alloc();
	MUNLOCK(array, width * sizeof(float *));
    }

    for (size_t i = m_width; i < width; ++i) {
	array[i] = 0;
    }

    for (size_t i = 0; i < width; ++i) {
	array[i] = (float *)realloc(array[i], height * sizeof(float));
	if (!array[i]) throw std::bad_alloc();
	MUNLOCK(array[i], height * sizeof(float));
    }
}

void
FFTMemoryCache::reset()
{
    switch (m_storageType) {

    case Compact:
        for (size_t x = 0; x < m_width; ++x) {
            for (size_t y = 0; y < m_height; ++y) {
                m_magnitude[x][y] = 0;
                m_phase[x][y] = 0;
            }
            m_factor[x] = 1.0;
        }
        break;
        
    case Polar:
        for (size_t x = 0; x < m_width; ++x) {
            for (size_t y = 0; y < m_height; ++y) {
                m_fmagnitude[x][y] = 0;
                m_fphase[x][y] = 0;
            }
            m_factor[x] = 1.0;
        }
        break;

    case Rectangular:
        for (size_t x = 0; x < m_width; ++x) {
            for (size_t y = 0; y < m_height; ++y) {
                m_freal[x][y] = 0;
                m_fimag[x][y] = 0;
            }
            m_factor[x] = 1.0;
        }
        break;        
    }
}	    

void
FFTMemoryCache::setColumnAt(size_t x, float *mags, float *phases, float factor)
{
    setNormalizationFactor(x, factor);

    if (m_storageType == Rectangular) {
        for (size_t y = 0; y < m_height; ++y) {
            m_freal[x][y] = mags[y] * cosf(phases[y]);
            m_fimag[x][y] = mags[y] * sinf(phases[y]);
        }
    } else {
        for (size_t y = 0; y < m_height; ++y) {
            setMagnitudeAt(x, y, mags[y]);
            setPhaseAt(x, y, phases[y]);
        }
    }

    m_colset.set(x);
}

void
FFTMemoryCache::setColumnAt(size_t x, float *reals, float *imags)
{
    float max = 0.0;

    switch (m_storageType) {

    case Rectangular:
        for (size_t y = 0; y < m_height; ++y) {
            m_freal[x][y] = reals[y];
            m_fimag[x][y] = imags[y];
            float mag = sqrtf(reals[y] * reals[y] + imags[y] * imags[y]);
            if (mag > max) max = mag;
        }
        break;

    case Compact:
    case Polar:
        for (size_t y = 0; y < m_height; ++y) {
            float mag = sqrtf(reals[y] * reals[y] + imags[y] * imags[y]);
            float phase = atan2f(imags[y], reals[y]);
            phase = princargf(phase);
            reals[y] = mag;
            imags[y] = phase;
            if (mag > max) max = mag;
        }
        break;
    };

    if (m_storageType == Rectangular) {
        m_factor[x] = max;
        m_colset.set(x);
    } else {
        setColumnAt(x, reals, imags, max);
    }
}

size_t
FFTMemoryCache::getCacheSize(size_t width, size_t height, StorageType type)
{
    size_t sz = 0;

    switch (type) {

    case Compact:
        sz = (height * 2 + 1) * width * sizeof(uint16_t);

    case Polar:
    case Rectangular:
        sz = (height * 2 + 1) * width * sizeof(float);
    }

    return sz;
}

