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

#ifndef _FFT_FILE_CACHE_H_
#define _FFT_FILE_CACHE_H_

#include "FFTCache.h"
#include "MatrixFile.h"

class FFTFileCache : public FFTCacheBase
{
public:
    //!!! This is very much a work in progress.
    //
    // Initially, make this take a string for the filename,
    // and make the spectrogram layer have two, one for the main
    // thread and one for the fill thread, one RO and one RW, both
    // using the same string based off spectrogram layer address
    // or export ID.
    // Subsequently factor out into reader and writer;
    // make take arguments to ctor describing FFT parameters and
    // calculate its own string and eventually do its own FFT as
    // well.  Intention is to make it able ultimately to write
    // its own cache so it can do it in the background while e.g.
    // plugins get data from it -- need the reader thread to be able
    // to block waiting for the writer thread as appropriate.

    FFTFileCache(QString fileBase, MatrixFile::Mode mode);
    virtual ~FFTFileCache();

    virtual size_t getWidth() const;
    virtual size_t getHeight() const;
	
    virtual void resize(size_t width, size_t height);
    virtual void reset(); // zero-fill or 1-fill as appropriate without changing size
	
    virtual float getMagnitudeAt(size_t x, size_t y) const;
    virtual float getNormalizedMagnitudeAt(size_t x, size_t y) const;
    virtual float getPhaseAt(size_t x, size_t y) const;

    virtual void setNormalizationFactor(size_t x, float factor);
    virtual void setMagnitudeAt(size_t x, size_t y, float mag);
    virtual void setNormalizedMagnitudeAt(size_t x, size_t y, float norm);
    virtual void setPhaseAt(size_t x, size_t y, float phase);

    //!!! not thread safe (but then neither is m_mfc)
    virtual void setColumnAt(size_t x, float *mags, float *phases, float factor);

protected:
    float *m_colbuf;
    MatrixFile *m_mfc;
};

#endif
