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

#ifndef MOCK_WAVE_MODEL_H
#define MOCK_WAVE_MODEL_H

#include "../DenseTimeValueModel.h"

#include <vector>

enum Sort {
    DC,
    Sine,
    Cosine,
    Nyquist,
    Dirac
};

class MockWaveModel : public DenseTimeValueModel
{
    Q_OBJECT

public:
    /** One Sort per channel! Length is in samples, and is in addition
     * to "pad" number of zero samples at the start and end */
    MockWaveModel(std::vector<Sort> sorts, int length, int pad);

    virtual float getValueMinimum() const { return -1.f; }
    virtual float getValueMaximum() const { return  1.f; }
    virtual int getChannelCount() const { return int(m_data.size()); }
    
    virtual floatvec_t getData(int channel, sv_frame_t start, sv_frame_t count) const;
    virtual std::vector<floatvec_t> getMultiChannelData(int fromchannel, int tochannel, sv_frame_t start, sv_frame_t count) const;

    virtual bool canPlay() const { return true; }
    virtual QString getDefaultPlayClipId() const { return ""; }

    virtual sv_frame_t getStartFrame() const { return 0; }
    virtual sv_frame_t getEndFrame() const { return m_data[0].size(); }
    virtual sv_samplerate_t getSampleRate() const { return 44100; }
    virtual bool isOK() const { return true; }
    
    QString getTypeName() const { return tr("Mock Wave"); }

private:
    std::vector<std::vector<float> > m_data;
    std::vector<float> generate(Sort sort, int length, int pad) const;
};

#endif
