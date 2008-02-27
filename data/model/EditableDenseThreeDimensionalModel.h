/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _EDITABLE_DENSE_THREE_DIMENSIONAL_MODEL_H_
#define _EDITABLE_DENSE_THREE_DIMENSIONAL_MODEL_H_

#include "DenseThreeDimensionalModel.h"

class EditableDenseThreeDimensionalModel : public DenseThreeDimensionalModel
{
    Q_OBJECT

public:
    EditableDenseThreeDimensionalModel(size_t sampleRate,
				       size_t resolution,
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
    virtual size_t getResolution() const;

    /**
     * Set the number of sample frames covered by each set of bins.
     */
    virtual void setResolution(size_t sz);

    /**
     * Return the number of columns.
     */
    virtual size_t getWidth() const;

    /**
     * Return the number of bins in each set of bins.
     */
    virtual size_t getHeight() const; 

    /**
     * Set the number of bins in each set of bins.
     */
    virtual void setHeight(size_t sz);

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

    /**
     * Return true if there are data available for the given column.
     */
    virtual bool isColumnAvailable(size_t x) const { return x < getWidth(); }

    /**
     * Get the set of bin values at the given column.
     */
    virtual void getColumn(size_t x, Column &) const;

    /**
     * Get a single value, from the n'th bin of the given column.
     */
    virtual float getValueAt(size_t x, size_t n) const;

    /**
     * Set the entire set of bin values at the given column.
     */
    virtual void setColumn(size_t x, const Column &values);

    virtual QString getBinName(size_t n) const;
    virtual void setBinName(size_t n, QString);
    virtual void setBinNames(std::vector<QString> names);

    virtual void setCompletion(int completion, bool update = true);
    virtual int getCompletion() const { return m_completion; }

    QString getTypeName() const { return tr("Editable Dense 3-D"); }

    virtual QString toDelimitedDataString(QString delimiter) const;

    virtual void toXml(QTextStream &out,
                       QString indent = "",
                       QString extraAttributes = "") const;

protected:
    typedef std::vector<Column> ValueMatrix;
    ValueMatrix m_data;

    std::vector<QString> m_binNames;

    size_t m_sampleRate;
    size_t m_resolution;
    size_t m_yBinCount;
    float m_minimum;
    float m_maximum;
    bool m_haveExtents;
    bool m_notifyOnAdd;
    long m_sinceLastNotifyMin;
    long m_sinceLastNotifyMax;
    int m_completion;

    mutable QMutex m_mutex;
};

#endif
