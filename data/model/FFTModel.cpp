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

#include "FFTModel.h"
#include "DenseTimeValueModel.h"

#include <cassert>

FFTModel::FFTModel(const DenseTimeValueModel *model,
                   int channel,
                   WindowType windowType,
                   size_t windowSize,
                   size_t windowIncrement,
                   size_t fftSize,
                   bool polar,
                   size_t fillFromColumn) :
    //!!! ZoomConstraint!
    m_server(0),
    m_xshift(0),
    m_yshift(0)
{
    m_server = FFTDataServer::getFuzzyInstance(model,
                                               channel,
                                               windowType,
                                               windowSize,
                                               windowIncrement,
                                               fftSize,
                                               polar,
                                               fillFromColumn);

    size_t xratio = windowIncrement / m_server->getWindowIncrement();
    size_t yratio = m_server->getFFTSize() / fftSize;

    while (xratio > 1) {
        if (xratio & 0x1) {
            std::cerr << "ERROR: FFTModel: Window increment ratio "
                      << windowIncrement << " / "
                      << m_server->getWindowIncrement()
                      << " must be a power of two" << std::endl;
            assert(!(xratio & 0x1));
        }
        ++m_xshift;
        xratio >>= 1;
    }

    while (yratio > 1) {
        if (yratio & 0x1) {
            std::cerr << "ERROR: FFTModel: FFT size ratio "
                      << m_server->getFFTSize() << " / " << fftSize
                      << " must be a power of two" << std::endl;
            assert(!(yratio & 0x1));
        }
        ++m_yshift;
        yratio >>= 1;
    }
}

FFTModel::~FFTModel()
{
    FFTDataServer::releaseInstance(m_server);
}

size_t
FFTModel::getSampleRate() const
{
    return isOK() ? m_server->getModel()->getSampleRate() : 0;
}

void
FFTModel::getColumn(size_t x, Column &result) const
{
    result.clear();
    size_t height(getHeight());
    for (size_t y = 0; y < height; ++y) {
        result.push_back(const_cast<FFTModel *>(this)->getMagnitudeAt(x, y));
    }
}

QString
FFTModel::getBinName(size_t n) const
{
    size_t sr = getSampleRate();
    if (!sr) return "";
    QString name = tr("%1 Hz").arg((n * sr) / (getHeight() * 2));
    return name;
}

Model *
FFTModel::clone() const
{
    return new FFTModel(*this);
}

FFTModel::FFTModel(const FFTModel &model) :
    DenseThreeDimensionalModel(),
    m_server(model.m_server),
    m_xshift(model.m_xshift),
    m_yshift(model.m_yshift)
{
    FFTDataServer::claimInstance(m_server);
}

