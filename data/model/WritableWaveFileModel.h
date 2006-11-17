/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _WRITABLE_WAVE_FILE_MODEL_H_
#define _WRITABLE_WAVE_FILE_MODEL_H_

#include "WaveFileModel.h"

class WavFileWriter;
class WavFileReader;

class WritableWaveFileModel : public RangeSummarisableTimeValueModel
{
    Q_OBJECT

public:
    WritableWaveFileModel(size_t sampleRate, size_t channels, QString path = "");
    ~WritableWaveFileModel();

    /**
     * Call addSamples to append a block of samples to the end of the
     * file.  Caller should also call setCompletion to update the
     * progress of this file, if it has a known end point, and should
     * call setCompletion(100) when the file has been written.
     */
    virtual bool addSamples(float **samples, size_t count);
    
    bool isOK() const;
    bool isReady(int *) const;

    virtual void setCompletion(int completion); // percentage
    virtual int getCompletion() const { return m_completion; }

    const ZoomConstraint *getZoomConstraint() const {
        static PowerOfSqrtTwoZoomConstraint zc;
        return &zc;
    }

    size_t getFrameCount() const;
    size_t getChannelCount() const { return m_channels; }
    size_t getSampleRate() const { return m_sampleRate; }

    virtual Model *clone() const;

    float getValueMinimum() const { return -1.0f; }
    float getValueMaximum() const { return  1.0f; }

    virtual size_t getStartFrame() const { return 0; }
    virtual size_t getEndFrame() const { return getFrameCount(); }

    virtual size_t getValues(int channel, size_t start, size_t end,
			     float *buffer) const;

    virtual size_t getValues(int channel, size_t start, size_t end,
			     double *buffer) const;

    virtual RangeBlock getRanges(size_t channel, size_t start, size_t end,
				 size_t &blockSize) const;

    virtual Range getRange(size_t channel, size_t start, size_t end) const;

    virtual void toXml(QTextStream &out,
                       QString indent = "",
                       QString extraAttributes = "") const;

    virtual QString toXmlString(QString indent = "",
				QString extraAttributes = "") const;

protected:
    WaveFileModel *m_model;
    WavFileWriter *m_writer;
    WavFileReader *m_reader;
    size_t m_sampleRate;
    size_t m_channels;
    size_t m_frameCount;
    int m_completion;
};

#endif

