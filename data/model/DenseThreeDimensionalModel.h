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

#ifndef _DENSE_THREE_DIMENSIONAL_MODEL_H_
#define _DENSE_THREE_DIMENSIONAL_MODEL_H_

#include "Model.h"
#include "base/ZoomConstraint.h"

#include <QMutex>
#include <vector>

class DenseThreeDimensionalModel : public Model,
				   virtual public ZoomConstraint,
				   virtual public QObject
{
    Q_OBJECT

public:
    /**
     * Return the number of sample frames covered by each set of bins.
     */
    virtual size_t getResolution() const = 0;

    /**
     * Return the number of bins in each set of bins.
     */
    virtual size_t getYBinCount() const = 0; 

    /**
     * Return the minimum value of the value in each bin.
     */
    virtual float getMinimumLevel() const = 0;

    /**
     * Return the maximum value of the value in each bin.
     */
    virtual float getMaximumLevel() const = 0;

    typedef std::vector<float> BinValueSet;

    /**
     * Get the set of bin values at the given sample frame (i.e. the
     * windowStartFrame/getWindowSize()'th set of bins).
     */
    virtual void getBinValues(long windowStartFrame, BinValueSet &result) const = 0;

    /**
     * Get a single value, the one at the n'th bin of the set of bins
     * starting at the given sample frame.
     */
    virtual float getBinValue(long windowStartFrame, size_t n) const = 0;

    virtual QString getBinName(size_t n) const = 0;

    virtual int getCompletion() const = 0;

protected:
    DenseThreeDimensionalModel() { }
};

#endif
