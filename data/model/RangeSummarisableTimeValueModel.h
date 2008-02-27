/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2007 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RANGE_SUMMARISABLE_TIME_VALUE_MODEL_H_
#define _RANGE_SUMMARISABLE_TIME_VALUE_MODEL_H_

#include <QObject>

#include "DenseTimeValueModel.h"
#include "base/ZoomConstraint.h"

/**
 * Base class for models containing dense two-dimensional data (value
 * against time) that may be meaningfully represented in a zoomed view
 * using min/max range summaries.  Audio waveform data is an obvious
 * example: think "peaks and minima" for "ranges".
 */

class RangeSummarisableTimeValueModel : public DenseTimeValueModel
{
    Q_OBJECT

public:
    RangeSummarisableTimeValueModel() { }

    struct Range
    {
        float min;
        float max;
        float absmean;
        Range() : 
            min(0.f), max(0.f), absmean(0.f) { }
        Range(const Range &r) : 
            min(r.min), max(r.max), absmean(r.absmean) { }
        Range(float min_, float max_, float absmean_) :
            min(min_), max(max_), absmean(absmean_) { }
    };

    typedef std::vector<Range> RangeBlock;

    /**
     * Return ranges from the given start frame, corresponding to the
     * given number of underlying sample frames, summarised at the
     * given block size.  duration / blockSize ranges should ideally
     * be returned.
     *
     * If the given block size is not supported by this model
     * (according to its zoom constraint), also modify the blockSize
     * parameter so as to return the block size that was actually
     * obtained.
     */
    virtual void getSummaries(size_t channel, size_t start, size_t count,
                              RangeBlock &ranges,
                              size_t &blockSize) const = 0;

    /**
     * Return the range from the given start frame, corresponding to
     * the given number of underlying sample frames, summarised at a
     * block size equal to the distance between start and end frames.
     */
    virtual Range getSummary(size_t channel, size_t start, size_t count) const = 0;

    virtual size_t getSummaryBlockSize(size_t desired) const = 0;

    QString getTypeName() const { return tr("Range-Summarisable Time-Value"); }
};

#endif

