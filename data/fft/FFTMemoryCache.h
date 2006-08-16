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

#ifndef _FFT_MEMORY_CACHE_H_
#define _FFT_MEMORY_CACHE_H_

#include "FFTCache.h"

#include "base/ResizeableBitset.h"

/**
 * For the in-memory FFT cache, we would like to cache magnitude with
 * enough resolution to have gain applied afterwards and determine
 * whether something is a peak or not, and also cache phase rather
 * than only phase-adjusted frequency so that we don't have to
 * recalculate if switching between phase and magnitude displays.  At
 * the same time, we don't want to take up too much memory.  It's not
 * expected to be accurate enough to be used as input for DSP or
 * resynthesis code.
 *
 * This implies probably 16 bits for a normalized magnitude and at
 * most 16 bits for phase.
 *
 * Each column's magnitudes are expected to be stored normalized
 * to [0,1] with respect to the column, so the normalization
 * factor should be calculated before all values in a column, and
 * set appropriately.
 */

class FFTMemoryCache : public FFTCache
{
public:
    FFTMemoryCache(); // of size zero, call resize() before using
    virtual ~FFTMemoryCache();
	
    virtual size_t getWidth() const { return m_width; }
    virtual size_t getHeight() const { return m_height; }
	
    virtual void resize(size_t width, size_t height);
    virtual void reset(); // zero-fill or 1-fill as appropriate without changing size
    
    virtual float getMagnitudeAt(size_t x, size_t y) const {
        return getNormalizedMagnitudeAt(x, y) * m_factor[x];
    }
    
    virtual float getNormalizedMagnitudeAt(size_t x, size_t y) const {
        return float(m_magnitude[x][y]) / 65535.0;
    }
    
    virtual float getMaximumMagnitudeAt(size_t x) const {
        return m_factor[x];
    }
    
    virtual float getPhaseAt(size_t x, size_t y) const {
        int16_t i = (int16_t)m_phase[x][y];
        return (float(i) / 32767.0) * M_PI;
    }
    
    virtual void getValuesAt(size_t x, size_t y, float &real, float &imag) const {
        float mag = getMagnitudeAt(x, y);
        float phase = getPhaseAt(x, y);
        real = mag * cosf(phase);
        imag = mag * sinf(phase);
    }

    virtual void setNormalizationFactor(size_t x, float factor) {
        if (x < m_width) m_factor[x] = factor;
    }
    
    virtual void setMagnitudeAt(size_t x, size_t y, float mag) {
         // norm factor must already be set
        setNormalizedMagnitudeAt(x, y, mag / m_factor[x]);
    }
    
    virtual void setNormalizedMagnitudeAt(size_t x, size_t y, float norm) {
        if (x < m_width && y < m_height) {
            m_magnitude[x][y] = uint16_t(norm * 65535.0);
        }
    }
    
    virtual void setPhaseAt(size_t x, size_t y, float phase) {
        // phase in range -pi -> pi
        if (x < m_width && y < m_height) {
            m_phase[x][y] = uint16_t(int16_t((phase * 32767) / M_PI));
        }
    }
    
    virtual bool haveSetColumnAt(size_t x) const {
        return m_colset.get(x);
    }

    virtual void setColumnAt(size_t x, float *mags, float *phases, float factor) {
        setNormalizationFactor(x, factor);
        for (size_t y = 0; y < m_height; ++y) {
            setMagnitudeAt(x, y, mags[y]);
            setPhaseAt(x, y, phases[y]);
        }
        m_colset.set(x);
    }

    virtual void setColumnAt(size_t x, float *reals, float *imags);

private:
    size_t m_width;
    size_t m_height;
    uint16_t **m_magnitude;
    uint16_t **m_phase;
    float *m_factor;
    ResizeableBitset m_colset;

    void resize(uint16_t **&, size_t, size_t);
};


#endif

