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

#include "RangeSummarisableTimeValueModel.h"
#include "PowerOfSqrtTwoZoomConstraint.h"

#include <stdlib.h>

class AudioFileReader;

class WaveFileModel : public RangeSummarisableTimeValueModel
{
    Q_OBJECT

public:
    WaveFileModel(QString path);
    WaveFileModel(QString path, AudioFileReader *reader);
    ~WaveFileModel();

    bool isOK() const;
    bool isReady(int *) const;

    const ZoomConstraint *getZoomConstraint() const { return &m_zoomConstraint; }

    size_t getFrameCount() const;
    size_t getChannelCount() const;
    size_t getSampleRate() const;

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
    
	size_t getFillExtent() const { return m_fillExtent; }
        virtual void run();

    protected:
        WaveFileModel &m_model;
	size_t m_fillExtent;
        size_t m_frameCount;
    };
         
    void fillCache();
    
    QString m_path;
    AudioFileReader *m_reader;
    bool m_myReader;

    RangeBlock m_cache[2]; // interleaved at two base resolutions
    mutable QMutex m_mutex;
    RangeCacheFillThread *m_fillThread;
    QTimer *m_updateTimer;
    size_t m_lastFillExtent;
    bool m_exiting;
    static PowerOfSqrtTwoZoomConstraint m_zoomConstraint;
};    

#endif
