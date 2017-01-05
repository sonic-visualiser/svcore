/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2009 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef DENSE_3D_MODEL_PEAK_CACHE_H
#define DENSE_3D_MODEL_PEAK_CACHE_H

#include "DenseThreeDimensionalModel.h"
#include "EditableDenseThreeDimensionalModel.h"

class Dense3DModelPeakCache : public DenseThreeDimensionalModel
{
    Q_OBJECT

public:
    Dense3DModelPeakCache(const DenseThreeDimensionalModel *source,
                          int columnsPerPeak);
    ~Dense3DModelPeakCache();

    virtual bool isOK() const {
        return m_source && m_source->isOK(); 
    }

    virtual sv_samplerate_t getSampleRate() const {
        return m_source->getSampleRate();
    }

    virtual sv_frame_t getStartFrame() const {
        return m_source->getStartFrame();
    }

    virtual sv_frame_t getEndFrame() const {
        return m_source->getEndFrame();
    }

    virtual int getResolution() const {
        return m_source->getResolution() * m_columnsPerPeak;
    }

    virtual int getColumnsPerPeak() const {
        return m_columnsPerPeak;
    }
    
    virtual int getWidth() const {
        int sourceWidth = m_source->getWidth();
        if ((sourceWidth % m_columnsPerPeak) == 0) {
            return sourceWidth / m_columnsPerPeak;
        } else {
            return sourceWidth / m_columnsPerPeak + 1;
        }
    }

    virtual int getHeight() const {
        return m_source->getHeight();
    }

    virtual float getMinimumLevel() const {
        return m_source->getMinimumLevel();
    }

    virtual float getMaximumLevel() const {
        return m_source->getMaximumLevel();
    }

    /**
     * Retrieve the peaks column at peak-cache column number col. This
     * will consist of the peak values in the underlying model from
     * columns (col * getColumnsPerPeak()) to ((col+1) *
     * getColumnsPerPeak() - 1) inclusive.
     */
    virtual Column getColumn(int col) const;

    virtual float getValueAt(int col, int n) const;

    virtual QString getBinName(int n) const {
        return m_source->getBinName(n);
    }

    virtual bool shouldUseLogValueScale() const {
        return m_source->shouldUseLogValueScale();
    }

    QString getTypeName() const { return tr("Dense 3-D Peak Cache"); }

    virtual int getCompletion() const {
        return m_source->getCompletion();
    }

protected slots:
    void sourceModelChanged();
    void sourceModelAboutToBeDeleted();

private:
    const DenseThreeDimensionalModel *m_source;
    mutable EditableDenseThreeDimensionalModel *m_cache;
    mutable std::vector<bool> m_coverage; // must be bool, for space efficiency
                                          // (vector of bool uses 1-bit elements)
    int m_columnsPerPeak;

    bool haveColumn(int column) const;
    void fillColumn(int column) const;
};


#endif
