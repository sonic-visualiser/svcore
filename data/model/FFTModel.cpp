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

#include "base/Profiler.h"
#include "base/Pitch.h"
#include "base/HitCount.h"
#include "base/Debug.h"
#include "base/MovingMedian.h"

#include "bqvec/VectorOpsComplex.h"

#include <algorithm>

#include <cassert>
#include <deque>

//#define DEBUG_FFT_MODEL 1

using namespace std;

static HitCount inSmallCache("FFTModel: Small FFT cache");
static HitCount inSourceCache("FFTModel: Source data cache");

FFTModel::FFTModel(ModelId modelId,
                   int channel,
                   WindowType windowType,
                   int windowSize,
                   int windowIncrement,
                   int fftSize) :
    m_model(modelId),
    m_sampleRate(0),
    m_channel(channel),
    m_windowType(windowType),
    m_windowSize(windowSize),
    m_windowIncrement(windowIncrement),
    m_fftSize(fftSize),
    m_windower(windowType, windowSize),
    m_fft(fftSize),
    m_maximumFrequency(0.0),
    m_cacheWriteIndex(0),
    m_cacheSize(3)
{
    clearCaches();
    
    if (m_windowSize > m_fftSize) {
        SVCERR << "ERROR: FFTModel::FFTModel: window size (" << m_windowSize
               << ") may not exceed FFT size (" << m_fftSize << ")" << endl;
        throw invalid_argument("FFTModel window size may not exceed FFT size");
    }

    m_fft.initFloat();

    auto model = ModelById::getAs<DenseTimeValueModel>(m_model);
    if (model) {
        m_sampleRate = model->getSampleRate();
        m_unit = model->getValueUnit();
        
        connect(model.get(), SIGNAL(modelChanged(ModelId)),
                this, SIGNAL(modelChanged(ModelId)));
        connect(model.get(), SIGNAL(modelChangedWithin(ModelId, sv_frame_t, sv_frame_t)),
                this, SIGNAL(modelChangedWithin(ModelId, sv_frame_t, sv_frame_t)));
    } else {
        m_error = QString("Model #%1 is not available").arg(m_model.untyped);
    }
}

FFTModel::~FFTModel()
{
}

void
FFTModel::clearCaches()
{
    m_cached.clear();
    while (m_cached.size() < m_cacheSize) {
        m_cached.push_back({ -1, doublecomplexvec_t(m_fftSize / 2 + 1) });
    }
    m_cacheWriteIndex = 0;
    m_savedData.range = { 0, 0 };
}

bool
FFTModel::isOK() const
{
    auto model = ModelById::getAs<DenseTimeValueModel>(m_model);
    if (!model) {
        m_error = QString("Model #%1 is not available").arg(m_model.untyped);
        return false;
    }
    if (!model->isOK()) {
        m_error = QString("Model #%1 is not OK").arg(m_model.untyped);
        return false;
    }
    return true;
}

int
FFTModel::getCompletion() const
{
    int c = 100;
    auto model = ModelById::getAs<DenseTimeValueModel>(m_model);
    if (model) {
        if (model->isReady(&c)) return 100;
    }
    return c;
}

void
FFTModel::setMaximumFrequency(double freq)
{
    m_maximumFrequency = freq;
    clearCaches();
}

int
FFTModel::getWidth() const
{
    auto model = ModelById::getAs<DenseTimeValueModel>(m_model);
    if (!model) return 0;
    return int((model->getEndFrame() - model->getStartFrame())
               / m_windowIncrement) + 1;
}

int
FFTModel::getHeight() const
{
    int height = m_fftSize / 2 + 1;
    if (m_maximumFrequency != 0.0) {
        int maxBin = int(ceil(m_maximumFrequency * m_fftSize) / m_sampleRate);
        if (maxBin >= 0 && maxBin < height) {
            return maxBin + 1;
        }
    }
    return height;
}

QString
FFTModel::getBinName(int n) const
{
    return tr("%1 Hz").arg(getBinValue(n));
}

float
FFTModel::getBinValue(int n) const
{
    return float((m_sampleRate * n) / m_fftSize);
}

FFTModel::Column
FFTModel::getColumn(int x) const
{
    auto cplx = getFFTColumn(x);
    Column col;
    col.reserve(cplx.size());
    for (auto c: cplx) {
        col.push_back(abs(c));
    }
    return col;
}

FFTModel::Column
FFTModel::getColumn(int x, int minbin, int nbins) const
{
    auto cplx = getFFTColumn(x);
    Column col;
    col.reserve(nbins);
    for (int i = 0; i < nbins; ++i) {
        col.push_back(abs(cplx[minbin + i]));
    }
    return col;
}

FFTModel::Column
FFTModel::getPhases(int x) const
{
    auto cplx = getFFTColumn(x);
    Column col;
    col.reserve(cplx.size());
    for (auto c: cplx) {
        col.push_back(arg(c));
    }
    return col;
}

float
FFTModel::getMagnitudeAt(int x, int y) const
{
    if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) {
        return 0.f;
    }
    auto col = getFFTColumn(x);
    return abs(col[y]);
}

float
FFTModel::getMaximumMagnitudeAt(int x) const
{
    Column col(getColumn(x));
    float max = 0.f;
    int n = int(col.size());
    for (int i = 0; i < n; ++i) {
        if (col[i] > max) max = col[i];
    }
    return max;
}

float
FFTModel::getPhaseAt(int x, int y) const
{
    if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) return 0.f;
    return arg(getFFTColumn(x)[y]);
}

void
FFTModel::getValuesAt(int x, int y, float &re, float &im) const
{
    if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) {
        re = 0.f;
        im = 0.f;
        return;
    }
    auto col = getFFTColumn(x);
    re = col[y].real();
    im = col[y].imag();
}

bool
FFTModel::getMagnitudesAt(int x, float *values, int minbin, int count) const
{
    if (count == 0) {
        count = getHeight() - minbin;
    }
    auto col = getFFTColumn(x);
    for (int i = 0; i < count; ++i) {
        values[i] = abs(col[minbin + i]);
    }
    return true;
}

bool
FFTModel::getPhasesAt(int x, float *values, int minbin, int count) const
{
    if (count == 0) count = getHeight();
    auto col = getFFTColumn(x);
    for (int i = 0; i < count; ++i) {
        values[i] = arg(col[minbin + i]);
    }
    return true;
}

bool
FFTModel::getValuesAt(int x, float *reals, float *imags, int minbin, int count) const
{
    if (count == 0) count = getHeight();
    auto col = getFFTColumn(x);
    for (int i = 0; i < count; ++i) {
        reals[i] = col[minbin + i].real();
    }
    for (int i = 0; i < count; ++i) {
        imags[i] = col[minbin + i].imag();
    }
    return true;
}

floatvec_t
FFTModel::getSourceSamples(int column) const
{
    // m_fftSize may be greater than m_windowSize, but not the reverse

#ifdef DEBUG_FFT_MODEL
    SVDEBUG << "getSourceSamples(" << column << ")" << endl;
#endif
    
    auto range = getSourceSampleRange(column);
    auto data = getSourceData(range);

    int off = (m_fftSize - m_windowSize) / 2;

    if (off == 0) {
        return data;
    } else {
        vector<float> pad(off, 0.f);
        floatvec_t padded;
        padded.reserve(m_fftSize);
        padded.insert(padded.end(), pad.begin(), pad.end());
        padded.insert(padded.end(), data.begin(), data.end());
        padded.insert(padded.end(), pad.begin(), pad.end());
        return padded;
    }
}

floatvec_t
FFTModel::getSourceData(pair<sv_frame_t, sv_frame_t> range) const
{
#ifdef DEBUG_FFT_MODEL
    SVDEBUG << "getSourceData(" << range.first << "," << range.second
            << "): saved range is (" << m_savedData.range.first
            << "," << m_savedData.range.second << ")" << endl;
#endif

    if (m_savedData.range == range) {
        inSourceCache.hit();
#ifdef DEBUG_FFT_MODEL
        SVDEBUG << "getSourceData(" << range.first << "," << range.second
                << "): source cache hit" << endl;
#endif
        return m_savedData.data;
    }

    Profiler profiler("FFTModel::getSourceData (cache miss)");
    
    if (range.first < m_savedData.range.second &&
        range.first >= m_savedData.range.first &&
        range.second > m_savedData.range.second) {

        inSourceCache.partial();

#ifdef DEBUG_FFT_MODEL
        SVDEBUG << "getSourceData(" << range.first << "," << range.second
                << "): source cache partial hit" << endl;
#endif
        
        sv_frame_t discard = range.first - m_savedData.range.first;

        floatvec_t data;
        data.reserve(range.second - range.first);

        data.insert(data.end(),
                    m_savedData.data.begin() + discard,
                    m_savedData.data.end());

        floatvec_t rest = getSourceDataUncached
            ({ m_savedData.range.second, range.second });

        data.insert(data.end(), rest.begin(), rest.end());
        
        m_savedData = { range, data };
        return data;

    } else {

        inSourceCache.miss();

#ifdef DEBUG_FFT_MODEL
        SVDEBUG << "getSourceData(" << range.first << "," << range.second
                << "): source cache miss" << endl;
#endif
        
        auto data = getSourceDataUncached(range);
        m_savedData = { range, data };
        return data;
    }
}

floatvec_t
FFTModel::getSourceDataUncached(pair<sv_frame_t, sv_frame_t> range) const
{
    Profiler profiler("FFTModel::getSourceDataUncached");

    auto model = ModelById::getAs<DenseTimeValueModel>(m_model);
    if (!model) return {};
    
    decltype(range.first) pfx = 0;
    if (range.first < 0) {
        pfx = -range.first;
        range = { 0, range.second };
    }

    auto data = model->getData(m_channel,
                               range.first,
                               range.second - range.first);

#ifdef DEBUG_FFT_MODEL
    if (data.empty()) {
        SVDEBUG << "NOTE: empty source data for range (" << range.first << ","
                << range.second << ") (model end frame "
                << model->getEndFrame() << ")" << endl;
    }
#endif
    
    // don't return a partial frame
    data.resize(range.second - range.first, 0.f);

    if (pfx > 0) {
        vector<float> pad(pfx, 0.f);
        data.insert(data.begin(), pad.begin(), pad.end());
    }
    
    if (m_channel == -1) {
        int channels = model->getChannelCount();
        if (channels > 1) {
            int n = int(data.size());
            float factor = 1.f / float(channels);
            // use mean instead of sum for fft model input
            for (int i = 0; i < n; ++i) {
                data[i] *= factor;
            }
        }
    }
    
    return data;
}

const doublecomplexvec_t &
FFTModel::getFFTColumn(int n) const
{
    // The small cache (i.e. the m_cached deque) is for cases where
    // values are looked up individually, and for e.g. peak-frequency
    // spectrograms where values from two consecutive columns are
    // needed at once. This cache gets essentially no hits when
    // scrolling through a magnitude spectrogram, but 95%+ hits with a
    // peak-frequency spectrogram or spectrum.
    for (const auto &incache : m_cached) {
        if (incache.n == n) {
            inSmallCache.hit();
            return incache.col;
        }
    }
    inSmallCache.miss();

    Profiler profiler("FFTModel::getFFTColumn (cache miss)");
    
    auto fsamples = getSourceSamples(n);

    // Ensure that windowing and FFT happen in double precision
    vector<double> samples;
    samples.reserve(fsamples.size());
    for (int i = 0; in_range_for(fsamples, i); ++i) {
        samples.push_back(fsamples[i]);
    }
    
    m_windower.cut(samples.data() + (m_fftSize - m_windowSize) / 2);
    breakfastquay::v_fftshift(samples.data(), m_fftSize);

    doublecomplexvec_t &col = m_cached[m_cacheWriteIndex].col;

    // expand to large enough for fft destination, if truncated previously
    col.resize(m_fftSize / 2 + 1);

    m_fft.forwardInterleaved(samples.data(),
                             reinterpret_cast<double *>(col.data()));

    // keep only the number of elements we need - so that we can
    // return a const ref without having to resize on a cache hit
    col.resize(getHeight());

#ifdef DEBUG_FFT_MODEL
    {
        vector<double> mags(getHeight(), 0.0);
        breakfastquay::v_cartesian_interleaved_to_magnitudes
            ((double *)mags.data(),
             (const double *)col.data(),
             getHeight());
        SVDEBUG << "FFTModel::getFFTColumn(" << n << "): fft size " << m_fftSize
                << ", height " << getHeight() << ", mag range "
                << breakfastquay::v_min(mags.data(), mags.size()) << " to "
                << breakfastquay::v_max(mags.data(), mags.size()) << endl;
    }
#endif
    
    m_cached[m_cacheWriteIndex].n = n;

    m_cacheWriteIndex = (m_cacheWriteIndex + 1) % m_cacheSize;

    return col;
}

bool
FFTModel::estimateStableFrequency(int x, int y, double &frequency)
{
    if (!isOK()) return false;

    frequency = double(y * getSampleRate()) / m_fftSize;

    if (x+1 >= getWidth()) return false;

    // At frequency f, a phase shift of 2pi (one cycle) happens in 1/f sec.
    // At hopsize h and sample rate sr, one hop happens in h/sr sec.
    // At window size w, for bin b, f is b*sr/w.
    // thus 2pi phase shift happens in w/(b*sr) sec.
    // We need to know what phase shift we expect from h/sr sec.
    // -> 2pi * ((h/sr) / (w/(b*sr)))
    //  = 2pi * ((h * b * sr) / (w * sr))
    //  = 2pi * (h * b) / w.

    double oldPhase = getPhaseAt(x, y);
    double newPhase = getPhaseAt(x+1, y);

    int incr = getResolution();

    double expectedPhase = oldPhase + (2.0 * M_PI * y * incr) / m_fftSize;

    double phaseError = princarg(newPhase - expectedPhase);

    // The new frequency estimate based on the phase error resulting
    // from assuming the "native" frequency of this bin

    frequency =
        (getSampleRate() * (expectedPhase + phaseError - oldPhase)) /
        (2.0 * M_PI * incr);

    return true;
}

FFTModel::PeakLocationSet
FFTModel::getPeaks(PeakPickType type, int x, int ymin, int ymax) const
{
    Profiler profiler("FFTModel::getPeaks");
    
    FFTModel::PeakLocationSet peaks;
    if (!isOK()) return peaks;

    if (ymax == 0 || ymax > getHeight() - 1) {
        ymax = getHeight() - 1;
    }

    if (type == AllPeaks) {
        int minbin = ymin;
        if (minbin > 0) minbin = minbin - 1;
        int maxbin = ymax;
        if (maxbin < getHeight() - 1) maxbin = maxbin + 1;
        const int n = maxbin - minbin + 1;
        float *values = new float[n];
        getMagnitudesAt(x, values, minbin, maxbin - minbin + 1);
        for (int bin = ymin; bin <= ymax; ++bin) {
            if (bin == minbin || bin == maxbin) continue;
            if (values[bin - minbin] > values[bin - minbin - 1] &&
                values[bin - minbin] > values[bin - minbin + 1]) {
                peaks.insert(bin);
            }
        }
        delete[] values;
        return peaks;
    }

    Column values = getColumn(x);
    int nv = int(values.size());

    float mean = 0.f;
    for (int i = 0; i < nv; ++i) mean += values[i];
    if (nv > 0) mean = mean / float(values.size());
    
    // For peak picking we use a moving median window, picking the
    // highest value within each continuous region of values that
    // exceed the median.  For pitch adaptivity, we adjust the window
    // size to a roughly constant pitch range (about four tones).

    sv_samplerate_t sampleRate = getSampleRate();

    vector<int> inrange;
    double dist = 0.5;

    int medianWinSize = getPeakPickWindowSize(type, sampleRate, ymin, dist);
    int halfWin = medianWinSize/2;

    MovingMedian<float> window(medianWinSize);

    int binmin;
    if (ymin > halfWin) binmin = ymin - halfWin;
    else binmin = 0;

    int binmax;
    if (ymax + halfWin < nv) binmax = ymax + halfWin;
    else binmax = nv - 1;

    int prevcentre = 0;

    for (int bin = binmin; bin <= binmax; ++bin) {

        float value = values[bin];

        // so-called median will actually be the dist*100'th percentile
        medianWinSize = getPeakPickWindowSize(type, sampleRate, bin, dist);
        halfWin = medianWinSize/2;

        int actualSize = std::min(medianWinSize, bin - binmin + 1);
        window.resize(actualSize);
        window.setPercentile(dist * 100.0);
        window.push(value);

        if (type == MajorPitchAdaptivePeaks) {
            if (ymax + halfWin < nv) binmax = ymax + halfWin;
            else binmax = nv - 1;
        }

        float median = window.get();

        int centrebin = 0;
        if (bin > actualSize/2) centrebin = bin - actualSize/2;
        
        while (centrebin > prevcentre || bin == binmin) {

            if (centrebin > prevcentre) ++prevcentre;

            float centre = values[prevcentre];

            if (centre > median) {
                inrange.push_back(centrebin);
            }

            if (centre <= median || centrebin+1 == nv) {
                if (!inrange.empty()) {
                    int peakbin = 0;
                    float peakval = 0.f;
                    for (int i = 0; i < (int)inrange.size(); ++i) {
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

            if (bin == binmin) break;
        }
    }

    return peaks;
}

int
FFTModel::getPeakPickWindowSize(PeakPickType type, sv_samplerate_t sampleRate,
                                int bin, double &dist) const
{
    dist = 0.5; // dist is percentile / 100.0
    if (type == MajorPeaks) return 10;
    if (bin == 0) return 3;

    double binfreq = (sampleRate * bin) / m_fftSize;
    double hifreq = Pitch::getFrequencyForPitch(73, 0, binfreq);

    int hibin = int(lrint((hifreq * m_fftSize) / sampleRate));
    int medianWinSize = hibin - bin;

    if (medianWinSize < 3) {
        medianWinSize = 3;
    }

    // We want to avoid the median window size changing too often, as
    // it requires a reallocation. So snap to a nearby round number.
    
    if (medianWinSize > 20) {
        medianWinSize = (1 + medianWinSize / 10) * 10;
    }
    if (medianWinSize > 200) {
        medianWinSize = (1 + medianWinSize / 100) * 100;
    }
    if (medianWinSize > 2000) {
        medianWinSize = (1 + medianWinSize / 1000) * 1000;
    }
    if (medianWinSize > 20000) {
        medianWinSize = 20000;
    }

    if (medianWinSize < 100) {
        dist = 1.0 - (4.0 / medianWinSize);
    } else {
        dist = 1.0 - (8.0 / medianWinSize);
    }        
    if (dist < 0.5) dist = 0.5;
    
    return medianWinSize;
}

FFTModel::PeakSet
FFTModel::getPeakFrequencies(PeakPickType type, int x,
                             int ymin, int ymax) const
{
    Profiler profiler("FFTModel::getPeakFrequencies");

    PeakSet peaks;
    if (!isOK()) return peaks;
    PeakLocationSet locations = getPeaks(type, x, ymin, ymax);

    sv_samplerate_t sampleRate = getSampleRate();
    int incr = getResolution();

    // This duplicates some of the work of estimateStableFrequency to
    // allow us to retrieve the phases in two separate vertical
    // columns, instead of jumping back and forth between columns x and
    // x+1, which may be significantly slower if re-seeking is needed

    vector<float> phases;
    for (PeakLocationSet::iterator i = locations.begin();
         i != locations.end(); ++i) {
        phases.push_back(getPhaseAt(x, *i));
    }

    int phaseIndex = 0;
    for (PeakLocationSet::iterator i = locations.begin();
         i != locations.end(); ++i) {
        double oldPhase = phases[phaseIndex];
        double newPhase = getPhaseAt(x+1, *i);
        double expectedPhase = oldPhase + (2.0 * M_PI * *i * incr) / m_fftSize;
        double phaseError = princarg(newPhase - expectedPhase);
        double frequency =
            (sampleRate * (expectedPhase + phaseError - oldPhase))
            / (2 * M_PI * incr);
        peaks[*i] = frequency;
        ++phaseIndex;
    }

    return peaks;
}

