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

#ifndef _CODED_AUDIO_FILE_READER_H_
#define _CODED_AUDIO_FILE_READER_H_

#include "AudioFileReader.h"

#include <sndfile.h>
#include <QMutex>

class WavFileReader;

class CodedAudioFileReader : public AudioFileReader
{
public:
    virtual ~CodedAudioFileReader();

    enum CacheMode {
        CacheInTemporaryFile,
        CacheInMemory
    };

    virtual void getInterleavedFrames(size_t start, size_t count,
				      SampleBlock &frames) const;

protected:
    CodedAudioFileReader(CacheMode cacheMode);

    void initialiseDecodeCache(); // samplerate, channels must have been set
    void addSampleToDecodeCache(float sample);
    void finishDecodeCache();
    bool isDecodeCacheInitialised() const { return m_initialised; }

    QMutex m_cacheMutex;
    CacheMode m_cacheMode;
    SampleBlock m_data;
    bool m_initialised;

    QString m_cacheFileName;
    SNDFILE *m_cacheFileWritePtr;
    WavFileReader *m_cacheFileReader;
    float *m_cacheWriteBuffer;
    size_t m_cacheWriteBufferIndex;
    size_t m_cacheWriteBufferSize; // frames
};

#endif
