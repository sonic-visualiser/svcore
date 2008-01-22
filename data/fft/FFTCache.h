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

#ifndef _FFT_CACHE_H_
#define _FFT_CACHE_H_

#include <cstdlib>
#include <cmath>

#include <stdint.h>

class FFTCache
{
public:
    virtual ~FFTCache() { }

    virtual size_t getWidth() const = 0;
    virtual size_t getHeight() const = 0;
	
    virtual void resize(size_t width, size_t height) = 0;
    virtual void reset() = 0; // zero-fill or 1-fill as appropriate without changing size
	
    virtual float getMagnitudeAt(size_t x, size_t y) const = 0;
    virtual float getNormalizedMagnitudeAt(size_t x, size_t y) const = 0;
    virtual float getMaximumMagnitudeAt(size_t x) const = 0;
    virtual float getPhaseAt(size_t x, size_t y) const = 0;

    virtual void getValuesAt(size_t x, size_t y, float &real, float &imaginary) const = 0;

    virtual bool haveSetColumnAt(size_t x) const = 0;

    // may modify argument arrays
    virtual void setColumnAt(size_t x, float *mags, float *phases, float factor) = 0;

    // may modify argument arrays
    virtual void setColumnAt(size_t x, float *reals, float *imags) = 0;

    virtual void suspend() { }

    enum Type { MemoryCache, FileCache };
    virtual Type getType() = 0;

protected:
    FFTCache() { }
};


#endif
