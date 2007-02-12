
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

#ifdef HAVE_MAD

#include "MP3FileReader.h"
#include "system/System.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>

#include <QApplication>
#include <QFileInfo>
#include <QProgressDialog>

MP3FileReader::MP3FileReader(QString path, bool showProgress, CacheMode mode) :
    CodedAudioFileReader(mode),
    m_path(path)
{
    m_frameCount = 0;
    m_channelCount = 0;
    m_sampleRate = 0;
    m_fileSize = 0;
    m_bitrateNum = 0;
    m_bitrateDenom = 0;
    m_frameCount = 0;
    m_cancelled = false;

    struct stat stat;
    if (::stat(path.toLocal8Bit().data(), &stat) == -1 || stat.st_size == 0) {
	m_error = QString("File %1 does not exist.").arg(path);
	return;
    }

    m_fileSize = stat.st_size;

    int fd = -1;
    if ((fd = ::open(path.toLocal8Bit().data(), O_RDONLY
#ifdef _WIN32
                     | O_BINARY
#endif
                     , 0)) < 0) {
	m_error = QString("Failed to open file %1 for reading.").arg(path);
	return;
    }	

    unsigned char *filebuffer = 0;

    try {
        filebuffer = new unsigned char[m_fileSize];
    } catch (...) {
        m_error = QString("Out of memory");
        ::close(fd);
	return;
    }
    
    ssize_t sz = 0;
    size_t offset = 0;
    while (offset < m_fileSize) {
        sz = ::read(fd, filebuffer + offset, m_fileSize - offset);
        if (sz < 0) {
            m_error = QString("Read error for file %1 (after %2 bytes)")
                .arg(path).arg(offset);
            delete[] filebuffer;
            ::close(fd);
            return;
        } else if (sz == 0) {
            std::cerr << QString("MP3FileReader::MP3FileReader: Warning: reached EOF after only %1 of %2 bytes")
                .arg(offset).arg(m_fileSize).toStdString() << std::endl;
            m_fileSize = offset;
            break;
        }
        offset += sz;
    }

    ::close(fd);

    if (showProgress) {
	m_progress = new QProgressDialog
	    (QObject::tr("Decoding %1...").arg(QFileInfo(path).fileName()),
	     QObject::tr("Stop"), 0, 100);
	m_progress->hide();
    }

    if (!decode(filebuffer, m_fileSize)) {
	m_error = QString("Failed to decode file %1.").arg(path);
        delete[] filebuffer;
	return;
    }
    
    if (isDecodeCacheInitialised()) finishDecodeCache();

    if (showProgress) {
	delete m_progress;
	m_progress = 0;
    }

    delete[] filebuffer;
}

MP3FileReader::~MP3FileReader()
{
}

bool
MP3FileReader::decode(void *mm, size_t sz)
{
    DecoderData data;
    struct mad_decoder decoder;

    data.start = (unsigned char const *)mm;
    data.length = (unsigned long)sz;
    data.reader = this;

    mad_decoder_init(&decoder, &data, input, 0, 0, output, error, 0);
    mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
    mad_decoder_finish(&decoder);

    return true;
}

enum mad_flow
MP3FileReader::input(void *dp, struct mad_stream *stream)
{
    DecoderData *data = (DecoderData *)dp;

    if (!data->length) return MAD_FLOW_STOP;
    mad_stream_buffer(stream, data->start, data->length);
    data->length = 0;

    return MAD_FLOW_CONTINUE;
}

enum mad_flow
MP3FileReader::output(void *dp,
		      struct mad_header const *header,
		      struct mad_pcm *pcm)
{
    DecoderData *data = (DecoderData *)dp;
    return data->reader->accept(header, pcm);
}

enum mad_flow
MP3FileReader::accept(struct mad_header const *header,
		      struct mad_pcm *pcm)
{
    int channels = pcm->channels;
    int frames = pcm->length;

    if (header) {
        m_bitrateNum += header->bitrate;
        m_bitrateDenom ++;
    }

    if (frames < 1) return MAD_FLOW_CONTINUE;

    if (m_channelCount == 0) {
        m_channelCount = channels;
        m_sampleRate = pcm->samplerate;
    }
    
    if (m_bitrateDenom > 0) {
        double bitrate = m_bitrateNum / m_bitrateDenom;
        double duration = double(m_fileSize * 8) / bitrate;
        double elapsed = double(m_frameCount) / m_sampleRate;
        double percent = ((elapsed * 100.0) / duration);
        int progress = int(percent);
        if (progress < 1) progress = 1;
        if (progress > 99) progress = 99;
        if (progress > m_progress->value()) {
            m_progress->setValue(progress);
            m_progress->show();
            m_progress->raise();
            qApp->processEvents();
            if (m_progress->wasCanceled()) {
                m_cancelled = true;
            }
        }
    }

    if (m_cancelled) return MAD_FLOW_STOP;

    m_frameCount += frames;

    if (!isDecodeCacheInitialised()) {
        initialiseDecodeCache();
    }

    for (int i = 0; i < frames; ++i) {

	for (int ch = 0; ch < channels; ++ch) {
	    mad_fixed_t sample = 0;
	    if (ch < int(sizeof(pcm->samples) / sizeof(pcm->samples[0]))) {
		sample = pcm->samples[ch][i];
	    }
	    float fsample = float(sample) / float(MAD_F_ONE);
            addSampleToDecodeCache(fsample);
	}

	if (! (i & 0xffff)) {
	    // periodically munlock to ensure we don't exhaust real memory
	    // if running with memory locked down
	    MUNLOCK_SAMPLEBLOCK(m_data);
	}
    }

    if (frames > 0) {
	MUNLOCK_SAMPLEBLOCK(m_data);
    }

    return MAD_FLOW_CONTINUE;
}

enum mad_flow
MP3FileReader::error(void *dp,
		     struct mad_stream *stream,
		     struct mad_frame *)
{
    DecoderData *data = (DecoderData *)dp;

    fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
	    stream->error, mad_stream_errorstr(stream),
	    stream->this_frame - data->start);

    return MAD_FLOW_CONTINUE;
}

void
MP3FileReader::getSupportedExtensions(std::set<QString> &extensions)
{
    extensions.insert("mp3");
}

#endif
