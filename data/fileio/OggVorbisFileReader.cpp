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

#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND

#include "OggVorbisFileReader.h"
#include "base/Profiler.h"
#include "system/System.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cmath>

#include <QApplication>
#include <QFileInfo>
#include <QProgressDialog>

static int instances = 0;

OggVorbisFileReader::OggVorbisFileReader(QString path, bool showProgress,
                                         CacheMode mode) :
    CodedAudioFileReader(mode),
    m_path(path),
    m_progress(0),
    m_fileSize(0),
    m_bytesRead(0),
    m_cancelled(false)
{
    m_frameCount = 0;
    m_channelCount = 0;
    m_sampleRate = 0;

    std::cerr << "OggVorbisFileReader::OggVorbisFileReader(" << path.toLocal8Bit().data() << "): now have " << (++instances) << " instances" << std::endl;

    Profiler profiler("OggVorbisFileReader::OggVorbisFileReader", true);

    QFileInfo info(path);
    m_fileSize = info.size();

    OGGZ *oggz;
    if (!(oggz = oggz_open(path.toLocal8Bit().data(), OGGZ_READ))) {
	m_error = QString("File %1 is not an OGG file.").arg(path);
	return;
    }

    FishSoundInfo fsinfo;
    m_fishSound = fish_sound_new(FISH_SOUND_DECODE, &fsinfo);

    fish_sound_set_decoded_callback(m_fishSound, acceptFrames, this);
    oggz_set_read_callback(oggz, -1, readPacket, this);

    if (showProgress) {
	m_progress = new QProgressDialog
	    (QObject::tr("Decoding %1...").arg(QFileInfo(path).fileName()),
	     QObject::tr("Stop"), 0, 100);
	m_progress->hide();
    }

    while (oggz_read(oggz, 1024) > 0);

    fish_sound_delete(m_fishSound);
    m_fishSound = 0;
    oggz_close(oggz);

    if (isDecodeCacheInitialised()) finishDecodeCache();

    if (showProgress) {
	delete m_progress;
	m_progress = 0;
    }
}

OggVorbisFileReader::~OggVorbisFileReader()
{
    std::cerr << "OggVorbisFileReader::~OggVorbisFileReader(" << m_path.toLocal8Bit().data() << "): now have " << (--instances) << " instances" << std::endl;
}

int
OggVorbisFileReader::readPacket(OGGZ *, ogg_packet *packet, long, void *data)
{
    OggVorbisFileReader *reader = (OggVorbisFileReader *)data;
    FishSound *fs = reader->m_fishSound;

    fish_sound_prepare_truncation(fs, packet->granulepos, packet->e_o_s);
    fish_sound_decode(fs, packet->packet, packet->bytes);

    reader->m_bytesRead += packet->bytes;
    
    if (reader->m_fileSize > 0 && reader->m_progress) {
	// The number of bytes read by this function is smaller than
	// the file size because of the packet headers
	int progress = lrint(double(reader->m_bytesRead) * 114 /
			     double(reader->m_fileSize));
	if (progress > 99) progress = 99;
	if (progress > reader->m_progress->value()) {
	    reader->m_progress->setValue(progress);
	    reader->m_progress->show();
	    reader->m_progress->raise();
	    qApp->processEvents();
	    if (reader->m_progress->wasCanceled()) {
		reader->m_cancelled = true;
	    }
	}
    } 

    if (reader->m_cancelled) return 1;
    return 0;
}

int
OggVorbisFileReader::acceptFrames(FishSound *fs, float **frames, long nframes,
				  void *data)
{
    OggVorbisFileReader *reader = (OggVorbisFileReader *)data;

    if (reader->m_channelCount == 0) {
	FishSoundInfo fsinfo;
	fish_sound_command(fs, FISH_SOUND_GET_INFO,
			   &fsinfo, sizeof(FishSoundInfo));
	reader->m_channelCount = fsinfo.channels;
	reader->m_sampleRate = fsinfo.samplerate;
        reader->initialiseDecodeCache();
    }

    if (nframes > 0) {

	reader->m_frameCount += nframes;
    
	for (long i = 0; i < nframes; ++i) {
	    for (size_t c = 0; c < reader->m_channelCount; ++c) {
                reader->addSampleToDecodeCache(frames[c][i]);
//		reader->m_data.push_back(frames[c][i]);
	    }
	}

	MUNLOCK_SAMPLEBLOCK(reader->m_data);
    }

    if (reader->m_cancelled) return 1;
    return 0;
}

void
OggVorbisFileReader::getSupportedExtensions(std::set<QString> &extensions)
{
    extensions.insert("ogg");
}

#endif
#endif
