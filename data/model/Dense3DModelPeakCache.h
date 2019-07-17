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
    Dense3DModelPeakCache(ModelId source, // a DenseThreeDimensionalModel
                          int columnsPerPeak);
    ~Dense3DModelPeakCache();

    bool isOK() const override {
        auto source = ModelById::get(m_source);
        return source && source->isOK(); 
    }

    sv_samplerate_t getSampleRate() const override {
        auto source = ModelById::get(m_source);
        return source ? source->getSampleRate() : 0;
    }

    sv_frame_t getStartFrame() const override {
        auto source = ModelById::get(m_source);
        return source ? source->getStartFrame() : 0;
    }

    sv_frame_t getTrueEndFrame() const override {
        auto source = ModelById::get(m_source);
        return source ? source->getTrueEndFrame() : 0;
    }

    int getResolution() const override {
        auto source = ModelById::getAs<DenseThreeDimensionalModel>(m_source);
        return source ? source->getResolution() * m_columnsPerPeak : 1;
    }

    virtual int getColumnsPerPeak() const {
        return m_columnsPerPeak;
    }
    
    int getWidth() const override {
        auto source = ModelById::getAs<DenseThreeDimensionalModel>(m_source);
        if (!source) return 0;
        int sourceWidth = source->getWidth();
        if ((sourceWidth % m_columnsPerPeak) == 0) {
            return sourceWidth / m_columnsPerPeak;
        } else {
            return sourceWidth / m_columnsPerPeak + 1;
        }
    }

    int getHeight() const override {
        auto source = ModelById::getAs<DenseThreeDimensionalModel>(m_source);
        return source ? source->getHeight() : 0;
    }

    float getMinimumLevel() const override {
        auto source = ModelById::getAs<DenseThreeDimensionalModel>(m_source);
        return source ? source->getMinimumLevel() : 0.f;
    }

    float getMaximumLevel() const override {
        auto source = ModelById::getAs<DenseThreeDimensionalModel>(m_source);
        return source ? source->getMaximumLevel() : 1.f;
    }

    /**
     * Retrieve the peaks column at peak-cache column number col. This
     * will consist of the peak values in the underlying model from
     * columns (col * getColumnsPerPeak()) to ((col+1) *
     * getColumnsPerPeak() - 1) inclusive.
     */
    Column getColumn(int col) const override;

    float getValueAt(int col, int n) const override;

    QString getBinName(int n) const override {
        auto source = ModelById::getAs<DenseThreeDimensionalModel>(m_source);
        return source ? source->getBinName(n) : "";
    }

    bool shouldUseLogValueScale() const override {
        auto source = ModelById::getAs<DenseThreeDimensionalModel>(m_source);
        return source ? source->shouldUseLogValueScale() : false;
    }

    QString getTypeName() const override { return tr("Dense 3-D Peak Cache"); }

    int getCompletion() const override {
        auto source = ModelById::get(m_source);
        return source ? source->getCompletion() : 100;
    }

    QString toDelimitedDataString(QString, DataExportOptions,
                                  sv_frame_t, sv_frame_t) const override {
        return "";
    }

protected slots:
    void sourceModelChanged(ModelId);

private:
    ModelId m_source;
    mutable std::unique_ptr<EditableDenseThreeDimensionalModel> m_cache;
    mutable std::vector<bool> m_coverage; // must be bool, for space efficiency
                                          // (vector of bool uses 1-bit elements)
    int m_columnsPerPeak;

    bool haveColumn(int column) const;
    void fillColumn(int column) const;
};


#endif
