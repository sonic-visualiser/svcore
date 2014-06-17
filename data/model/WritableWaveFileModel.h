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
    WritableWaveFileModel(int sampleRate, int channels, QString path = "");
    ~WritableWaveFileModel();

    /**
     * Call addSamples to append a block of samples to the end of the
     * file.  Caller should also call setCompletion to update the
     * progress of this file, if it has a known end point, and should
     * call setCompletion(100) when the file has been written.
     */
    virtual bool addSamples(float **samples, int count);
    
    bool isOK() const;
    bool isReady(int *) const;

    virtual void setCompletion(int completion); // percentage
    virtual int getCompletion() const { return m_completion; }

    const ZoomConstraint *getZoomConstraint() const {
        static PowerOfSqrtTwoZoomConstraint zc;
        return &zc;
    }

    int getFrameCount() const;
    int getChannelCount() const { return m_channels; }
    int getSampleRate() const { return m_sampleRate; }

    virtual Model *clone() const;

    float getValueMinimum() const { return -1.0f; }
    float getValueMaximum() const { return  1.0f; }

    virtual int getStartFrame() const { return m_startFrame; }
    virtual int getEndFrame() const { return m_startFrame + getFrameCount(); }

    void setStartFrame(int startFrame);

    virtual int getData(int channel, int start, int count,
                           float *buffer) const;

    virtual int getData(int channel, int start, int count,
                           double *buffer) const;

    virtual int getData(int fromchannel, int tochannel,
                           int start, int count,
                           float **buffer) const;

    virtual int getSummaryBlockSize(int desired) const;

    virtual void getSummaries(int channel, int start, int count,
                              RangeBlock &ranges, int &blockSize) const;

    virtual Range getSummary(int channel, int start, int count) const;

    QString getTypeName() const { return tr("Writable Wave File"); }

    virtual void toXml(QTextStream &out,
                       QString indent = "",
                       QString extraAttributes = "") const;

protected:
    WaveFileModel *m_model;
    WavFileWriter *m_writer;
    WavFileReader *m_reader;
    int m_sampleRate;
    int m_channels;
    int m_frameCount;
    int m_startFrame;
    int m_completion;
};

#endif

