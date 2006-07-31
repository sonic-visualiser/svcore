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
    //!!! need to reconcile terminology -- windowSize here, resolution in sparse models
    DenseThreeDimensionalModel(size_t sampleRate,
			       size_t windowSize,
			       size_t yBinCount,
			       bool notifyOnAdd = true);

    virtual bool isOK() const;

    virtual size_t getSampleRate() const;
    virtual size_t getStartFrame() const;
    virtual size_t getEndFrame() const;

    virtual Model *clone() const;
    

    /**
     * Return the number of sample frames covered by each set of bins.
     */
    virtual size_t getWindowSize() const;

    /**
     * Set the number of sample frames covered by each set of bins.
     */
    virtual void setWindowSize(size_t sz);

    /**
     * Return the number of bins in each set of bins.
     */
    virtual size_t getYBinCount() const; 

    /**
     * Set the number of bins in each set of bins.
     */
    virtual void setYBinCount(size_t sz);

    /**
     * Return the minimum value of the value in each bin.
     */
    virtual float getMinimumLevel() const;

    /**
     * Set the minimum value of the value in a bin.
     */
    virtual void setMinimumLevel(float sz);

    /**
     * Return the maximum value of the value in each bin.
     */
    virtual float getMaximumLevel() const;

    /**
     * Set the maximum value of the value in a bin.
     */
    virtual void setMaximumLevel(float sz);

    typedef std::vector<float> BinValueSet;

    /**
     * Get the set of bin values at the given sample frame (i.e. the
     * windowStartFrame/getWindowSize()'th set of bins).
     */
    virtual void getBinValues(long windowStartFrame, BinValueSet &result) const;

    /**
     * Get a single value, the one at the n'th bin of the set of bins
     * starting at the given sample frame.
     */
    virtual float getBinValue(long windowStartFrame, size_t n) const;

    /**
     * Set the entire set of bin values at the given sample frame.
     */
    virtual void setBinValues(long windowStartFrame, const BinValueSet &values);

    virtual QString getBinName(size_t n) const;
    virtual void setBinName(size_t n, QString);
    virtual void setBinNames(std::vector<QString> names);

    virtual void setCompletion(int completion);
    virtual int getCompletion() const { return m_completion; }

    virtual void toXml(QTextStream &out,
                       QString indent = "",
                       QString extraAttributes = "") const;

    virtual QString toXmlString(QString indent = "",
				QString extraAttributes = "") const;

protected:
    typedef std::vector<BinValueSet> ValueMatrix;
    ValueMatrix m_data;

    std::vector<QString> m_binNames;

    size_t m_sampleRate;
    size_t m_windowSize;
    size_t m_yBinCount;
    float m_minimum;
    float m_maximum;
    bool m_notifyOnAdd;
    long m_sinceLastNotifyMin;
    long m_sinceLastNotifyMax;
    int m_completion;

    mutable QMutex m_mutex;
};

#endif
