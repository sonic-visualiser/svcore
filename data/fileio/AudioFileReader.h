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

#ifndef _AUDIO_FILE_READER_H_
#define _AUDIO_FILE_READER_H_

#include <QString>
#include "model/Model.h" // for SampleBlock

class AudioFileReader : public QObject
{
    Q_OBJECT

public:
    virtual ~AudioFileReader() { }

    bool isOK() const { return (m_channelCount > 0); }

    virtual QString getError() const { return ""; }

    size_t getFrameCount() const { return m_frameCount; }
    size_t getChannelCount() const { return m_channelCount; }
    size_t getSampleRate() const { return m_sampleRate; }
    
    /** 
     * The subclass implementations of this function must be
     * thread-safe -- that is, safe to call from multiple threads with
     * different arguments on the same object at the same time.
     */
    virtual void getInterleavedFrames(size_t start, size_t count,
				      SampleBlock &frames) const = 0;

signals:
    void frameCountChanged();
    
protected:
    size_t m_frameCount;
    size_t m_channelCount;
    size_t m_sampleRate;
};

#endif
