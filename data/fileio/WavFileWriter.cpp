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

#include "WavFileWriter.h"

#include "model/DenseTimeValueModel.h"
#include "base/Selection.h"
#include "base/TempWriteFile.h"
#include "base/Exceptions.h"
#include "base/Debug.h"

#include <bqvec/Allocators.h>
#include <bqvec/VectorOps.h>

#include <bqaudiostream/AudioWriteStream.h>
#include <bqaudiostream/AudioWriteStreamFactory.h>

#include <QFileInfo>

#include <iostream>
#include <cmath>
#include <string>

using namespace std;

namespace sv {

WavFileWriter::WavFileWriter(QString path,
                             sv_samplerate_t sampleRate,
                             int channels,
                             FileWriteMode mode) :
    m_path(path),
    m_sampleRate(sampleRate),
    m_channels(channels),
    m_temp(nullptr),
    m_stream(nullptr)
{
    int fileRate = int(round(m_sampleRate));
    if (m_sampleRate != sv_samplerate_t(fileRate)) {
        SVCERR << "WavFileWriter: WARNING: Non-integer sample rate "
             << m_sampleRate << " presented, rounding to " << fileRate
             << endl;
    }

    QString writePath = m_path;
    try {
        if (mode == WriteToTemporary) {
            m_temp = new TempWriteFile(m_path);
            writePath = m_temp->getTemporaryFilename();
        }
        m_stream = breakfastquay::AudioWriteStreamFactory::createWriteStream
            (writePath.toStdString(), m_channels, fileRate);
    } catch (const std::exception &e) {
        SVCERR << "WavFileWriter: Failed to create file of "
               << m_channels << " channels at rate " << fileRate << " ("
               << e.what() << ")" << endl;
        m_error = QString("Failed to open audio file '%1' for writing")
            .arg(writePath);
        if (m_temp) {
            delete m_temp;
            m_temp = nullptr;
        }
    }
}

WavFileWriter::~WavFileWriter()
{
    if (m_stream) close();
}

bool
WavFileWriter::isOK() const
{
    return (m_error.isEmpty());
}

QString
WavFileWriter::getError() const
{
    return m_error;
}

QString
WavFileWriter::getWriteFilename() const
{
    if (m_temp) {
        return m_temp->getTemporaryFilename();
    } else {
        return m_path;
    }
}

bool
WavFileWriter::writeModel(DenseTimeValueModel *source,
                          MultiSelection *selection)
{
    if (source->getChannelCount() != m_channels) {
        SVDEBUG << "WavFileWriter::writeModel: Wrong number of channels ("
                  << source->getChannelCount()  << " != " << m_channels << ")"
                  << endl;
        m_error = QString("Failed to write model to audio file '%1'")
            .arg(getWriteFilename());
        return false;
    }

    if (!m_stream) {
        m_error = QString("Failed to write model to audio file '%1': File not open")
            .arg(getWriteFilename());
        return false;
    }

    bool ownSelection = false;
    if (!selection) {
        selection = new MultiSelection;
        selection->setSelection(Selection(source->getStartFrame(),
                                          source->getEndFrame()));
        ownSelection = true;
    }

    sv_frame_t bs = 2048;

    for (MultiSelection::SelectionList::iterator i =
         selection->getSelections().begin();
         i != selection->getSelections().end(); ++i) {
        
        sv_frame_t f0(i->getStartFrame()), f1(i->getEndFrame());

        for (sv_frame_t f = f0; f < f1; f += bs) {
            
            sv_frame_t n = min(bs, f1 - f);
            floatvec_t interleaved(n * m_channels, 0.f);

            for (int c = 0; c < int(m_channels); ++c) {
                auto chanbuf = source->getData(c, f, n);
                for (int i = 0; in_range_for(chanbuf, i); ++i) {
                    interleaved[i * m_channels + c] = chanbuf[i];
                }
            }

            try {
                m_stream->putInterleavedFrames(n, interleaved.data());
            } catch (const std::exception &e) {
                m_error = QString("Exception during file write: %1").arg(e.what());
                break;
            }
        }
    }

    if (ownSelection) delete selection;

    return isOK();
}
        
bool
WavFileWriter::writeSamples(const float *const *samples, sv_frame_t count)
{
    if (!m_stream) {
        m_error = QString("Failed to write model to audio file '%1': File not open")
            .arg(getWriteFilename());
        return false;
    }

    float *b = new float[count * m_channels];
    for (sv_frame_t i = 0; i < count; ++i) {
        for (int c = 0; c < int(m_channels); ++c) {
            b[i * m_channels + c] = samples[c][i];
        }
    }

    try {
        m_stream->putInterleavedFrames(count, b);
    } catch (const std::exception &e) {
        m_error = QString("Exception during file write: %1").arg(e.what());
    }

    delete[] b;

    return isOK();
}

bool
WavFileWriter::putInterleavedFrames(const floatvec_t &frames)
{
    sv_frame_t count = frames.size() / m_channels;
    float **samples =
        breakfastquay::allocate_channels<float>(m_channels, count);
    breakfastquay::v_deinterleave
        (samples, frames.data(), m_channels, int(count));
    bool result = writeSamples(samples, count);
    breakfastquay::deallocate_channels(samples, m_channels);
    return result;
}

bool
WavFileWriter::close()
{
    if (m_stream) {
        delete m_stream;
        m_stream = nullptr;
    }
    if (m_temp) {
        m_temp->moveToTarget();
        delete m_temp;
        m_temp = nullptr;
    }
    return true;
}

} // end namespace sv

