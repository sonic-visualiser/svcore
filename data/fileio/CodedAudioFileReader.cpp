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

#include "CodedAudioFileReader.h"

#include "WavFileReader.h"
#include "base/TempDirectory.h"
#include "base/Exceptions.h"

#include <iostream>
#include <QDir>

CodedAudioFileReader::CodedAudioFileReader(CacheMode cacheMode) :
    m_cacheMode(cacheMode),
    m_initialised(false),
    m_cacheFileWritePtr(0),
    m_cacheFileReader(0),
    m_cacheWriteBuffer(0),
    m_cacheWriteBufferIndex(0),
    m_cacheWriteBufferSize(16384)
{
}

CodedAudioFileReader::~CodedAudioFileReader()
{
    if (m_cacheFileWritePtr) sf_close(m_cacheFileWritePtr);
    if (m_cacheFileReader) delete m_cacheFileReader;
    if (m_cacheWriteBuffer) delete[] m_cacheWriteBuffer;

    if (m_cacheFileName != "") {
        if (!QFile(m_cacheFileName).remove()) {
            std::cerr << "WARNING: CodedAudioFileReader::~CodedAudioFileReader: Failed to delete cache file \"" << m_cacheFileName.toStdString() << "\"" << std::endl;
        }
    }
}

void
CodedAudioFileReader::initialiseDecodeCache()
{
    if (m_cacheMode == CacheInTemporaryFile) {

        m_cacheWriteBuffer = new float[m_cacheWriteBufferSize * m_channelCount];
        m_cacheWriteBufferIndex = 0;

        try {
            QDir dir(TempDirectory::getInstance()->getPath());
            m_cacheFileName = dir.filePath(QString("decoded_%1.wav")
                                           .arg((intptr_t)this));

            SF_INFO fileInfo;
            fileInfo.samplerate = m_sampleRate;
            fileInfo.channels = m_channelCount;
            fileInfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    
            m_cacheFileWritePtr = sf_open(m_cacheFileName.toLocal8Bit(),
                                          SFM_WRITE, &fileInfo);

            if (!m_cacheFileWritePtr) {
                std::cerr << "CodedAudioFileReader::initialiseDecodeCache: failed to open cache file \"" << m_cacheFileName.toStdString() << "\" (" << m_channelCount << " channels, sample rate " << m_sampleRate << " for writing, falling back to in-memory cache" << std::endl;
                m_cacheMode = CacheInMemory;
            }
        } catch (DirectoryCreationFailed f) {
            std::cerr << "CodedAudioFileReader::initialiseDecodeCache: failed to create temporary directory! Falling back to in-memory cache" << std::endl;
            m_cacheMode = CacheInMemory;
        }
    }

    if (m_cacheMode == CacheInMemory) {
        m_data.clear();
    }

    m_initialised = true;
}

void
CodedAudioFileReader::addSampleToDecodeCache(float sample)
{
    if (!m_initialised) return;

    switch (m_cacheMode) {

    case CacheInTemporaryFile:

        m_cacheWriteBuffer[m_cacheWriteBufferIndex++] = sample;

        if (m_cacheWriteBufferIndex ==
            m_cacheWriteBufferSize * m_channelCount) {

            //!!! check for return value! out of disk space, etc!
            sf_writef_float(m_cacheFileWritePtr,
                            m_cacheWriteBuffer,
                            m_cacheWriteBufferSize);

            m_cacheWriteBufferIndex = 0;
        }
        break;

    case CacheInMemory:
        m_data.push_back(sample);
        break;
    }
}

void
CodedAudioFileReader::finishDecodeCache()
{
    if (!m_initialised) {
        std::cerr << "WARNING: CodedAudioFileReader::finishDecodeCache: Cache was never initialised!" << std::endl;
        return;
    }

    switch (m_cacheMode) {

    case CacheInTemporaryFile:

        if (m_cacheWriteBufferIndex > 0) {
            //!!! check for return value! out of disk space, etc!
            sf_writef_float(m_cacheFileWritePtr,
                            m_cacheWriteBuffer,
                            m_cacheWriteBufferIndex / m_channelCount);
        }

        if (m_cacheWriteBuffer) {
            delete[] m_cacheWriteBuffer;
            m_cacheWriteBuffer = 0;
        }

        m_cacheWriteBufferIndex = 0;

        sf_close(m_cacheFileWritePtr);
        m_cacheFileWritePtr = 0;

        m_cacheFileReader = new WavFileReader(m_cacheFileName);

        if (!m_cacheFileReader->isOK()) {
            std::cerr << "ERROR: CodedAudioFileReader::finishDecodeCache: Failed to construct WAV file reader for temporary file: " << m_cacheFileReader->getError().toStdString() << std::endl;
            delete m_cacheFileReader;
            m_cacheFileReader = 0;
        }
        break;

    case CacheInMemory:
        // nothing to do 
        break;
    }
}

void
CodedAudioFileReader::getInterleavedFrames(size_t start, size_t count,
                                           SampleBlock &frames) const
{
    if (!m_initialised) return;

    switch (m_cacheMode) {

    case CacheInTemporaryFile:
        if (m_cacheFileReader) {
            m_cacheFileReader->getInterleavedFrames(start, count, frames);
        }
        break;

    case CacheInMemory:
    {
        frames.clear();
        if (!isOK()) return;
        if (count == 0) return;

        // slownessabounds

        for (size_t i = start; i < start + count; ++i) {
            for (size_t ch = 0; ch < m_channelCount; ++ch) {
                size_t index = i * m_channelCount + ch;
                if (index >= m_data.size()) return;
                frames.push_back(m_data[index]);
            }
        }
    }
    }
}

