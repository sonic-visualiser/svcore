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

#include "WritableWaveFileModel.h"

#include "base/TempDirectory.h"
#include "base/Exceptions.h"

#include "fileio/WavFileWriter.h"
#include "fileio/WavFileReader.h"

#include <QDir>

#include <cassert>
#include <iostream>

WritableWaveFileModel::WritableWaveFileModel(size_t sampleRate,
					     size_t channels,
					     QString path) :
    m_model(0),
    m_writer(0),
    m_reader(0),
    m_sampleRate(sampleRate),
    m_channels(channels),
    m_frameCount(0)
{
    if (path.isEmpty()) {
        try {
            QDir dir(TempDirectory::getInstance()->getPath());
            path = dir.filePath(QString("written_%1.wav")
                                .arg((intptr_t)this));
        } catch (DirectoryCreationFailed f) {
            std::cerr << "WritableWaveFileModel: Failed to create temporary directory" << std::endl;
            return;
        }
    }

    m_writer = new WavFileWriter(path, sampleRate, channels);
    if (!m_writer->isOK()) {
        std::cerr << "WritableWaveFileModel: Error in creating WAV file writer: " << m_writer->getError().toStdString() << std::endl;
        delete m_writer; 
        m_writer = 0;
        return;
    }
}

WritableWaveFileModel::~WritableWaveFileModel()
{
    delete m_model;
    delete m_writer;
    delete m_reader;
}

bool
WritableWaveFileModel::addSamples(float **samples, size_t count)
{
    if (!m_writer) return false;

    if (!m_writer->writeSamples(samples, count)) {
        std::cerr << "ERROR: WritableWaveFileModel::addSamples: writer failed: " << m_writer->getError().toStdString() << std::endl;
        return false;
    }

    m_frameCount += count;

    if (!m_model) {

        m_reader = new WavFileReader(m_writer->getPath(), true);
        if (!m_reader->getError().isEmpty()) {
            std::cerr << "WritableWaveFileModel: Error in creating wave file reader" << std::endl;
            delete m_reader;
            m_reader = 0;
            return false;
        }

        m_model = new WaveFileModel(m_writer->getPath(), m_reader);
        if (!m_model->isOK()) {
            std::cerr << "WritableWaveFileModel: Error in creating wave file model" << std::endl;
            delete m_model;
            m_model = 0;
            delete m_reader;
            m_reader = 0;
            return false;
        }
    }
    
    static int updateCounter = 0;
    if (++updateCounter == 100) {
        if (m_reader) m_reader->updateFrameCount();
        updateCounter = 0;
    }

    return true;
}

void
WritableWaveFileModel::sync()
{
    //!!! use setCompletion instead
    if (m_reader) m_reader->updateDone();
}    

bool
WritableWaveFileModel::isOK() const
{
    bool ok = (m_model && m_model->isOK());
    std::cerr << "WritableWaveFileModel::isOK(): ok = " << ok << std::endl;
    return ok;
}

bool
WritableWaveFileModel::isReady(int *completion) const
{
    bool ready = (m_model && m_model->isReady(completion));
    std::cerr << "WritableWaveFileModel::isReady(): ready = " << ready << std::endl;
    return ready;
}

size_t
WritableWaveFileModel::getFrameCount() const
{
    std::cerr << "WritableWaveFileModel::getFrameCount: count = " << m_frameCount << std::endl;
    return m_frameCount;
}

Model *
WritableWaveFileModel::clone() const
{
    assert(0); //!!!
}

size_t
WritableWaveFileModel::getValues(int channel, size_t start, size_t end,
                                 float *buffer) const
{
    if (!m_model) return 0;
    return m_model->getValues(channel, start, end, buffer);
}

size_t
WritableWaveFileModel::getValues(int channel, size_t start, size_t end,
                                 double *buffer) const
{
    if (!m_model) return 0;
//    std::cerr << "WritableWaveFileModel::getValues(" << channel << ", "
//              << start << ", " << end << "): calling model" << std::endl;
    return m_model->getValues(channel, start, end, buffer);
}

WritableWaveFileModel::RangeBlock
WritableWaveFileModel::getRanges(size_t channel, size_t start, size_t end,
                                 size_t &blockSize) const
{
    if (!m_model) return RangeBlock();
    return m_model->getRanges(channel, start, end, blockSize);
}

WritableWaveFileModel::Range
WritableWaveFileModel::getRange(size_t channel, size_t start, size_t end) const
{
    if (!m_model) return Range();
    return m_model->getRange(channel, start, end);
}

void
WritableWaveFileModel::toXml(QTextStream &out,
                             QString indent,
                             QString extraAttributes) const
{
    assert(0); //!!!
}

QString
WritableWaveFileModel::toXmlString(QString indent,
                                   QString extraAttributes) const
{
    assert(0); //!!!
    return "";
}

