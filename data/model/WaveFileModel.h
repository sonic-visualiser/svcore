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

#ifndef _WAVE_FILE_MODEL_H_
#define _WAVE_FILE_MODEL_H_

#include "base/Thread.h"
#include <QMutex>
#include <QTimer>

#include "data/fileio/FileSource.h"

#include "RangeSummarisableTimeValueModel.h"
#include "PowerOfSqrtTwoZoomConstraint.h"

#include <stdlib.h>

class AudioFileReader;

class WaveFileModel : public RangeSummarisableTimeValueModel
{
    Q_OBJECT

public:
    WaveFileModel(FileSource source, int targetRate = 0);
    WaveFileModel(FileSource source, AudioFileReader *reader);
    ~WaveFileModel();

    bool isOK() const;
    bool isReady(int *) const;

    const ZoomConstraint *getZoomConstraint() const { return &m_zoomConstraint; }

    int getFrameCount() const;
    int getChannelCount() const;
    int getSampleRate() const;
    int getNativeRate() const;

    QString getTitle() const;
    QString getMaker() const;
    QString getLocation() const;

    virtual Model *clone() const;

    float getValueMinimum() const { return -1.0f; }
    float getValueMaximum() const { return  1.0f; }

    virtual int getStartFrame() const { return m_startFrame; }
    virtual int getEndFrame() const { return m_startFrame + getFrameCount(); }

    void setStartFrame(int startFrame) { m_startFrame = startFrame; }

    virtual int getData(int channel, int start, int count,
                        float *buffer) const;

    virtual int getData(int channel, int start, int count,
                        double *buffer) const;

    virtual int getData(int fromchannel, int tochannel,
                        int start, int count,
                        float **buffers) const;

    virtual int getSummaryBlockSize(int desired) const;

    virtual void getSummaries(int channel, int start, int count,
                              RangeBlock &ranges,
                              int &blockSize) const;

    virtual Range getSummary(int channel, int start, int count) const;

    QString getTypeName() const { return tr("Wave File"); }

    virtual void toXml(QTextStream &out,
                       QString indent = "",
                       QString extraAttributes = "") const;

protected slots:
    void fillTimerTimedOut();
    void cacheFilled();
    
protected:
    void initialize();

    class RangeCacheFillThread : public Thread
    {
    public:
        RangeCacheFillThread(WaveFileModel &model) :
	    m_model(model), m_fillExtent(0),
            m_frameCount(model.getFrameCount()) { }
    
	int getFillExtent() const { return m_fillExtent; }
        virtual void run();

    protected:
        WaveFileModel &m_model;
	int m_fillExtent;
        int m_frameCount;
    };
         
    void fillCache();

    FileSource m_source;
    QString m_path;
    AudioFileReader *m_reader;
    bool m_myReader;

    int m_startFrame;

    RangeBlock m_cache[2]; // interleaved at two base resolutions
    mutable QMutex m_mutex;
    RangeCacheFillThread *m_fillThread;
    QTimer *m_updateTimer;
    int m_lastFillExtent;
    bool m_exiting;
    static PowerOfSqrtTwoZoomConstraint m_zoomConstraint;

    mutable SampleBlock m_directRead;
    mutable int m_lastDirectReadStart;
    mutable int m_lastDirectReadCount;
    mutable QMutex m_directReadMutex;
};    

#endif
