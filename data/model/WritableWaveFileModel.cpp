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

#include "WritableWaveFileModel.h"

#include "ReadOnlyWaveFileModel.h"

#include "base/TempDirectory.h"
#include "base/Exceptions.h"

#include "fileio/WavFileWriter.h"
#include "fileio/WavFileReader.h"

#include <QDir>
#include <QTextStream>

#include <cassert>
#include <iostream>
#include <stdint.h>

using namespace std;

const int WritableWaveFileModel::PROPORTION_UNKNOWN = -1;

//#define DEBUG_WRITABLE_WAVE_FILE_MODEL 1

WritableWaveFileModel::WritableWaveFileModel(sv_samplerate_t sampleRate,
					     int channels,
					     QString path) :
    m_model(0),
    m_writer(0),
    m_reader(0),
    m_sampleRate(sampleRate),
    m_channels(channels),
    m_frameCount(0),
    m_startFrame(0),
    m_proportion(PROPORTION_UNKNOWN)
{
    if (path.isEmpty()) {
        try {
            QDir dir(TempDirectory::getInstance()->getPath());
            path = dir.filePath(QString("written_%1.wav")
                                .arg((intptr_t)this));
        } catch (DirectoryCreationFailed f) {
            cerr << "WritableWaveFileModel: Failed to create temporary directory" << endl;
            return;
        }
    }

    // Write directly to the target file, so that we can do
    // incremental writes and concurrent reads
    m_writer = new WavFileWriter(path, sampleRate, channels,
                                 WavFileWriter::WriteToTarget);
    if (!m_writer->isOK()) {
        cerr << "WritableWaveFileModel: Error in creating WAV file writer: " << m_writer->getError() << endl;
        delete m_writer; 
        m_writer = 0;
        return;
    }

    FileSource source(m_writer->getPath());

    m_reader = new WavFileReader(source, true);
    if (!m_reader->getError().isEmpty()) {
        cerr << "WritableWaveFileModel: Error in creating wave file reader" << endl;
        delete m_reader;
        m_reader = 0;
        return;
    }
    
    m_model = new ReadOnlyWaveFileModel(source, m_reader);
    if (!m_model->isOK()) {
        cerr << "WritableWaveFileModel: Error in creating wave file model" << endl;
        delete m_model;
        m_model = 0;
        delete m_reader;
        m_reader = 0;
        return;
    }
    m_model->setStartFrame(m_startFrame);

    connect(m_model, SIGNAL(modelChanged()), this, SIGNAL(modelChanged()));
    connect(m_model, SIGNAL(modelChangedWithin(sv_frame_t, sv_frame_t)),
            this, SIGNAL(modelChangedWithin(sv_frame_t, sv_frame_t)));
}

WritableWaveFileModel::~WritableWaveFileModel()
{
    delete m_model;
    delete m_writer;
    delete m_reader;
}

void
WritableWaveFileModel::setStartFrame(sv_frame_t startFrame)
{
    m_startFrame = startFrame;
    if (m_model) m_model->setStartFrame(startFrame);
}

bool
WritableWaveFileModel::addSamples(const float *const *samples, sv_frame_t count)
{
    if (!m_writer) return false;

#ifdef DEBUG_WRITABLE_WAVE_FILE_MODEL
//    SVDEBUG << "WritableWaveFileModel::addSamples(" << count << ")" << endl;
#endif

    if (!m_writer->writeSamples(samples, count)) {
        SVCERR << "ERROR: WritableWaveFileModel::addSamples: writer failed: " << m_writer->getError() << endl;
        return false;
    }

    m_frameCount += count;

    if (m_reader && m_reader->getChannelCount() == 0) {
        m_reader->updateFrameCount();
    }

    return true;
}

void
WritableWaveFileModel::updateModel()
{
    if (m_reader) {
        m_reader->updateFrameCount();
    }
}

bool
WritableWaveFileModel::isOK() const
{
    bool ok = (m_writer && m_writer->isOK());
//    SVDEBUG << "WritableWaveFileModel::isOK(): ok = " << ok << endl;
    return ok;
}

bool
WritableWaveFileModel::isReady(int *completion) const
{
    int c = getCompletion();
    if (completion) *completion = c;
    if (!isOK()) return false;
    return (c == 100);
}

void
WritableWaveFileModel::setWriteProportion(int proportion)
{
    m_proportion = proportion;
}

int
WritableWaveFileModel::getWriteProportion() const
{
    return m_proportion;
}

void
WritableWaveFileModel::writeComplete()
{
    m_writer->close();
    if (m_reader) m_reader->updateDone();
    m_proportion = 100;
    emit modelChanged();
}

sv_frame_t
WritableWaveFileModel::getFrameCount() const
{
//    SVDEBUG << "WritableWaveFileModel::getFrameCount: count = " << m_frameCount << endl;
    return m_frameCount;
}

floatvec_t
WritableWaveFileModel::getData(int channel, sv_frame_t start, sv_frame_t count) const
{
    if (!m_model || m_model->getChannelCount() == 0) return {};
    return m_model->getData(channel, start, count);
}

vector<floatvec_t>
WritableWaveFileModel::getMultiChannelData(int fromchannel, int tochannel,
                                           sv_frame_t start, sv_frame_t count) const
{
    if (!m_model || m_model->getChannelCount() == 0) return {};
    return m_model->getMultiChannelData(fromchannel, tochannel, start, count);
}    

int
WritableWaveFileModel::getSummaryBlockSize(int desired) const
{
    if (!m_model) return desired;
    return m_model->getSummaryBlockSize(desired);
}

void
WritableWaveFileModel::getSummaries(int channel, sv_frame_t start, sv_frame_t count,
                                    RangeBlock &ranges,
                                    int &blockSize) const
{
    ranges.clear();
    if (!m_model || m_model->getChannelCount() == 0) return;
    m_model->getSummaries(channel, start, count, ranges, blockSize);
}

WritableWaveFileModel::Range
WritableWaveFileModel::getSummary(int channel, sv_frame_t start, sv_frame_t count) const
{
    if (!m_model || m_model->getChannelCount() == 0) return Range();
    return m_model->getSummary(channel, start, count);
}

void
WritableWaveFileModel::toXml(QTextStream &out,
                             QString indent,
                             QString extraAttributes) const
{
    // The assumption here is that the underlying wave file has
    // already been saved somewhere (its location is available through
    // getLocation()) and that the code that uses this class is
    // dealing with the problem of making sure it remains available.
    // We just write this out as if it were a normal wave file.

    Model::toXml
        (out, indent,
         QString("type=\"wavefile\" file=\"%1\" subtype=\"writable\" %2")
         .arg(encodeEntities(m_writer->getPath()))
         .arg(extraAttributes));
}

