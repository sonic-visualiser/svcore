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

#ifndef _FFT_MODEL_H_
#define _FFT_MODEL_H_

#include "data/fft/FFTDataServer.h"
#include "DenseThreeDimensionalModel.h"

/**
 * An implementation of DenseThreeDimensionalModel that makes FFT data
 * derived from a DenseTimeValueModel available as a generic data grid.
 * The FFT data is acquired using FFTDataServer.
 */

class FFTModel : public DenseThreeDimensionalModel
{
    Q_OBJECT

public:
    /**
     * Construct an FFT model derived from the given
     * DenseTimeValueModel, with the given window parameters and FFT
     * size (which may exceed the window size, for zero-padded FFTs).
     * 
     * If the model has multiple channels use only the given channel,
     * unless the channel is -1 in which case merge all available
     * channels.
     * 
     * If polar is true, the data will normally be retrieved from the
     * FFT model in magnitude/phase form; otherwise it will normally
     * be retrieved in "cartesian" real/imaginary form.  The results
     * should be the same either way, but a "polar" model addressed in
     * "cartesian" form or vice versa may suffer a performance
     * penalty.
     *
     * The fillFromColumn argument gives a hint that the FFT data
     * server should aim to start calculating FFT data at that column
     * number if possible, as that is likely to be requested first.
     */
    FFTModel(const DenseTimeValueModel *model,
             int channel,
             WindowType windowType,
             size_t windowSize,
             size_t windowIncrement,
             size_t fftSize,
             bool polar,
             size_t fillFromColumn = 0);
    ~FFTModel();

    float getMagnitudeAt(size_t x, size_t y) {
        return m_server->getMagnitudeAt(x << m_xshift, y << m_yshift);
    }
    float getNormalizedMagnitudeAt(size_t x, size_t y) {
        return m_server->getNormalizedMagnitudeAt(x << m_xshift, y << m_yshift);
    }
    float getMaximumMagnitudeAt(size_t x) {
        return m_server->getMaximumMagnitudeAt(x << m_xshift);
    }
    float getPhaseAt(size_t x, size_t y) {
        return m_server->getPhaseAt(x << m_xshift, y << m_yshift);
    }
    void getValuesAt(size_t x, size_t y, float &real, float &imaginary) {
        m_server->getValuesAt(x << m_xshift, y << m_yshift, real, imaginary);
    }
    bool isColumnAvailable(size_t x) const {
        return m_server->isColumnReady(x << m_xshift);
    }

    size_t getFillExtent() const { return m_server->getFillExtent(); }

    // DenseThreeDimensionalModel and Model methods:
    //
    virtual size_t getWidth() const {
        return m_server->getWidth() >> m_xshift;
    }
    virtual size_t getHeight() const {
        // If there is no y-shift, the server's height (based on its
        // fftsize/2 + 1) is correct.  If there is a shift, then the
        // server is using a larger fft size than we want, so we shift
        // it right as many times as necessary, but then we need to
        // re-add the "+1" part (because ((fftsize*2)/2 + 1) / 2 !=
        // fftsize/2 + 1).
        return (m_server->getHeight() >> m_yshift) + (m_yshift > 0 ? 1 : 0);
    }
    virtual float getValueAt(size_t x, size_t y) const {
        return const_cast<FFTModel *>(this)->getMagnitudeAt(x, y);
    }
    virtual bool isOK() const {
        return m_server && m_server->getModel();
    }
    virtual size_t getStartFrame() const {
        return 0;
    }
    virtual size_t getEndFrame() const {
        return getWidth() * getResolution() + getResolution();
    }
    virtual size_t getSampleRate() const;
    virtual size_t getResolution() const {
        return m_server->getWindowIncrement() << m_xshift;
    }
    virtual size_t getYBinCount() const {
        return getHeight();
    }
    virtual float getMinimumLevel() const {
        return 0.f; // Can't provide
    }
    virtual float getMaximumLevel() const {
        return 1.f; // Can't provide
    }
    virtual void getColumn(size_t x, Column &result) const;
    virtual QString getBinName(size_t n) const;

    virtual int getCompletion() const { return m_server->getFillCompletion(); }

    virtual Model *clone() const;

    virtual void suspend() { m_server->suspend(); }
    virtual void suspendWrites() { m_server->suspendWrites(); }
    virtual void resume() { m_server->resume(); }

private:
    FFTModel(const FFTModel &);
    FFTModel &operator=(const FFTModel &); // not implemented

    FFTDataServer *m_server;
    int m_xshift;
    int m_yshift;
};

#endif
