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
#include "AggregateWaveModel.h"

#include "base/Profiler.h"
#include "base/Pitch.h"

#include <cassert>

FFTModel::FFTModel(const DenseTimeValueModel *model,
                   int channel,
                   WindowType windowType,
                   size_t windowSize,
                   size_t windowIncrement,
                   size_t fftSize,
                   bool polar,
                   StorageAdviser::Criteria criteria,
                   size_t fillFromColumn) :
    //!!! ZoomConstraint!
    m_server(0),
    m_xshift(0),
    m_yshift(0)
{
    setSourceModel(const_cast<DenseTimeValueModel *>(model)); //!!! hmm.

    m_server = getServer(model,
                         channel,
                         windowType,
                         windowSize,
                         windowIncrement,
                         fftSize,
                         polar,
                         criteria,
                         fillFromColumn);

    if (!m_server) return; // caller should check isOK()

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
    if (m_server) FFTDataServer::releaseInstance(m_server);
}

void
FFTModel::sourceModelAboutToBeDeleted()
{
    if (m_sourceModel) {
        std::cerr << "FFTModel[" << this << "]::sourceModelAboutToBeDeleted(" << m_sourceModel << ")" << std::endl;
        if (m_server) {
            FFTDataServer::releaseInstance(m_server);
            m_server = 0;
        }
        FFTDataServer::modelAboutToBeDeleted(m_sourceModel);
    }
}

FFTDataServer *
FFTModel::getServer(const DenseTimeValueModel *model,
                    int channel,
                    WindowType windowType,
                    size_t windowSize,
                    size_t windowIncrement,
                    size_t fftSize,
                    bool polar,
                    StorageAdviser::Criteria criteria,
                    size_t fillFromColumn)
{
    // Obviously, an FFT model of channel C (where C != -1) of an
    // aggregate model is the same as the FFT model of the appropriate
    // channel of whichever model that aggregate channel is drawn
    // from.  We should use that model here, in case we already have
    // the data for it or will be wanting the same data again later.

    // If the channel is -1 (i.e. mixture of all channels), then we
    // can't do this shortcut unless the aggregate model only has one
    // channel or contains exactly all of the channels of a single
    // other model.  That isn't very likely -- if it were the case,
    // why would we be using an aggregate model?

    if (channel >= 0) {

        const AggregateWaveModel *aggregate =
            dynamic_cast<const AggregateWaveModel *>(model);

        if (aggregate && channel < aggregate->getComponentCount()) {

            AggregateWaveModel::ModelChannelSpec spec =
                aggregate->getComponent(channel);

            return getServer(spec.model,
                             spec.channel,
                             windowType,
                             windowSize,
                             windowIncrement,
                             fftSize,
                             polar,
                             criteria,
                             fillFromColumn);
        }
    }

    // The normal case

    return FFTDataServer::getFuzzyInstance(model,
                                           channel,
                                           windowType,
                                           windowSize,
                                           windowIncrement,
                                           fftSize,
                                           polar,
                                           criteria,
                                           fillFromColumn);
}

size_t
FFTModel::getSampleRate() const
{
    return isOK() ? m_server->getModel()->getSampleRate() : 0;
}

void
FFTModel::getColumn(size_t x, Column &result) const
{
    Profiler profiler("FFTModel::getColumn", false);

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
    QString name = tr("%1 Hz").arg((n * sr) / ((getHeight()-1) * 2));
    return name;
}

bool
FFTModel::estimateStableFrequency(size_t x, size_t y, float &frequency)
{
    if (!isOK()) return false;

    size_t sampleRate = m_server->getModel()->getSampleRate();

    size_t fftSize = m_server->getFFTSize() >> m_yshift;
    frequency = (float(y) * sampleRate) / fftSize;

    if (x+1 >= getWidth()) return false;

    // At frequency f, a phase shift of 2pi (one cycle) happens in 1/f sec.
    // At hopsize h and sample rate sr, one hop happens in h/sr sec.
    // At window size w, for bin b, f is b*sr/w.
    // thus 2pi phase shift happens in w/(b*sr) sec.
    // We need to know what phase shift we expect from h/sr sec.
    // -> 2pi * ((h/sr) / (w/(b*sr)))
    //  = 2pi * ((h * b * sr) / (w * sr))
    //  = 2pi * (h * b) / w.

    float oldPhase = getPhaseAt(x, y);
    float newPhase = getPhaseAt(x+1, y);

    size_t incr = getResolution();

    float expectedPhase = oldPhase + (2.0 * M_PI * y * incr) / fftSize;

    float phaseError = princargf(newPhase - expectedPhase);

//    bool stable = (fabsf(phaseError) < (1.1f * (m_windowIncrement * M_PI) / m_fftSize));

    // The new frequency estimate based on the phase error resulting
    // from assuming the "native" frequency of this bin

    frequency =
        (sampleRate * (expectedPhase + phaseError - oldPhase)) /
        (2 * M_PI * incr);

    return true;
}

FFTModel::PeakLocationSet
FFTModel::getPeaks(PeakPickType type, size_t x, size_t ymin, size_t ymax)
{
    FFTModel::PeakLocationSet peaks;
    if (!isOK()) return peaks;

    if (ymax == 0 || ymax > getHeight() - 1) {
        ymax = getHeight() - 1;
    }

    Column values;

    if (type == AllPeaks) {
        for (size_t y = ymin; y <= ymax; ++y) {
            values.push_back(getMagnitudeAt(x, y));
        }
        size_t i = 0;
        for (size_t bin = ymin; bin <= ymax; ++bin) {
            if ((i == 0 || values[i] > values[i-1]) &&
                (i == values.size()-1 || values[i] >= values[i+1])) {
                peaks.insert(bin);
            }
            ++i;
        }
        return peaks;
    }

    getColumn(x, values);

    // For peak picking we use a moving median window, picking the
    // highest value within each continuous region of values that
    // exceed the median.  For pitch adaptivity, we adjust the window
    // size to a roughly constant pitch range (about four tones).

    size_t sampleRate = getSampleRate();

    std::deque<float> window;
    std::vector<size_t> inrange;
    float dist = 0.5;
    size_t medianWinSize = getPeakPickWindowSize(type, sampleRate, ymin, dist);
    size_t halfWin = medianWinSize/2;

    size_t binmin;
    if (ymin > halfWin) binmin = ymin - halfWin;
    else binmin = 0;

    size_t binmax;
    if (ymax + halfWin < values.size()) binmax = ymax + halfWin;
    else binmax = values.size()-1;

    for (size_t bin = binmin; bin <= binmax; ++bin) {

        float value = values[bin];

        window.push_back(value);

        // so-called median will actually be the dist*100'th percentile
        medianWinSize = getPeakPickWindowSize(type, sampleRate, bin, dist);
        halfWin = medianWinSize/2;

        while (window.size() > medianWinSize) window.pop_front();

        if (type == MajorPitchAdaptivePeaks) {
            if (ymax + halfWin < values.size()) binmax = ymax + halfWin;
            else binmax = values.size()-1;
        }

        std::deque<float> sorted(window);
        std::sort(sorted.begin(), sorted.end());
        float median = sorted[int(sorted.size() * dist)];

        if (value > median) {
            inrange.push_back(bin);
        }

        if (value <= median || bin+1 == values.size()) {
            size_t peakbin = 0;
            float peakval = 0.f;
            if (!inrange.empty()) {
                for (size_t i = 0; i < inrange.size(); ++i) {
                    if (i == 0 || values[inrange[i]] > peakval) {
                        peakval = values[inrange[i]];
                        peakbin = inrange[i];
                    }
                }
                inrange.clear();
                if (peakbin >= ymin && peakbin <= ymax) {
                    peaks.insert(peakbin);
                }
            }
        }
    }

    return peaks;
}

size_t
FFTModel::getPeakPickWindowSize(PeakPickType type, size_t sampleRate,
                                size_t bin, float &percentile) const
{
    percentile = 0.5;
    if (type == MajorPeaks) return 10;
    if (bin == 0) return 3;

    size_t fftSize = m_server->getFFTSize() >> m_yshift;
    float binfreq = (sampleRate * bin) / fftSize;
    float hifreq = Pitch::getFrequencyForPitch(73, 0, binfreq);

    int hibin = lrintf((hifreq * fftSize) / sampleRate);
    int medianWinSize = hibin - bin;
    if (medianWinSize < 3) medianWinSize = 3;

    percentile = 0.5 + (binfreq / sampleRate);

    return medianWinSize;
}

FFTModel::PeakSet
FFTModel::getPeakFrequencies(PeakPickType type, size_t x,
                             size_t ymin, size_t ymax)
{
    PeakSet peaks;
    if (!isOK()) return peaks;
    PeakLocationSet locations = getPeaks(type, x, ymin, ymax);

    size_t sampleRate = getSampleRate();
    size_t fftSize = m_server->getFFTSize() >> m_yshift;
    size_t incr = getResolution();

    // This duplicates some of the work of estimateStableFrequency to
    // allow us to retrieve the phases in two separate vertical
    // columns, instead of jumping back and forth between columns x and
    // x+1, which may be significantly slower if re-seeking is needed

    std::vector<float> phases;
    for (PeakLocationSet::iterator i = locations.begin();
         i != locations.end(); ++i) {
        phases.push_back(getPhaseAt(x, *i));
    }

    size_t phaseIndex = 0;
    for (PeakLocationSet::iterator i = locations.begin();
         i != locations.end(); ++i) {
        float oldPhase = phases[phaseIndex];
        float newPhase = getPhaseAt(x+1, *i);
        float expectedPhase = oldPhase + (2.0 * M_PI * *i * incr) / fftSize;
        float phaseError = princargf(newPhase - expectedPhase);
        float frequency =
            (sampleRate * (expectedPhase + phaseError - oldPhase))
            / (2 * M_PI * incr);
//        bool stable = (fabsf(phaseError) < (1.1f * (incr * M_PI) / fftSize));
//        if (stable)
        peaks[*i] = frequency;
        ++phaseIndex;
    }

    return peaks;
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

