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

OggVorbisFileReader::OggVorbisFileReader(std::string path,
                                         DecodeMode decodeMode,
                                         CacheMode mode) :
    CodedAudioFileReader(mode),
    m_path(path),
    m_progress(0),
    m_fileSize(0),
    m_bytesRead(0),
    m_commentsRead(false),
    m_cancelled(false),
    m_completion(0),
    m_decodeThread(0)
{
    m_frameCount = 0;
    m_channelCount = 0;
    m_sampleRate = 0;

    std::cerr << "OggVorbisFileReader::OggVorbisFileReader(" << path << "): now have " << (++instances) << " instances" << std::endl;

    Profiler profiler("OggVorbisFileReader::OggVorbisFileReader", true);

    QFileInfo info(path.c_str());
    m_fileSize = info.size();

    if (!(m_oggz = oggz_open(path.c_str(), OGGZ_READ))) {
        setError("File is not an OGG file", path);
	return;
    }

    FishSoundInfo fsinfo;
    m_fishSound = fish_sound_new(FISH_SOUND_DECODE, &fsinfo);

    fish_sound_set_decoded_callback(m_fishSound, acceptFrames, this);
    oggz_set_read_callback(m_oggz, -1, readPacket, this);

    if (decodeMode == DecodeAtOnce) {

	m_progress = new QProgressDialog
	    (QObject::tr("Decoding %1...").arg(QFileInfo(path.c_str()).fileName()),
	     QObject::tr("Stop"), 0, 100);
	m_progress->hide();

        while (oggz_read(m_oggz, 1024) > 0);
        
        fish_sound_delete(m_fishSound);
        m_fishSound = 0;
        oggz_close(m_oggz);
        m_oggz = 0;

        if (isDecodeCacheInitialised()) finishDecodeCache();

        delete m_progress;
        m_progress = 0;

    } else {

        while (oggz_read(m_oggz, 1024) > 0 &&
               m_channelCount == 0);

        if (m_channelCount > 0) {
            m_decodeThread = new DecodeThread(this);
            m_decodeThread->start();
        }
    }
}

OggVorbisFileReader::~OggVorbisFileReader()
{
    std::cerr << "OggVorbisFileReader::~OggVorbisFileReader(" << m_path << "): now have " << (--instances) << " instances" << std::endl;
    if (m_decodeThread) {
        m_cancelled = true;
        m_decodeThread->wait();
        delete m_decodeThread;
    }
}

void
OggVorbisFileReader::DecodeThread::run()
{
    while (oggz_read(m_reader->m_oggz, 1024) > 0);
        
    fish_sound_delete(m_reader->m_fishSound);
    m_reader->m_fishSound = 0;
    oggz_close(m_reader->m_oggz);
    m_reader->m_oggz = 0;
    
    if (m_reader->isDecodeCacheInitialised()) m_reader->finishDecodeCache();
    m_reader->m_completion = 100;
} 

int
OggVorbisFileReader::readPacket(OGGZ *, ogg_packet *packet, long, void *data)
{
    OggVorbisFileReader *reader = (OggVorbisFileReader *)data;
    FishSound *fs = reader->m_fishSound;

    fish_sound_prepare_truncation(fs, packet->granulepos, packet->e_o_s);
    fish_sound_decode(fs, packet->packet, packet->bytes);

    reader->m_bytesRead += packet->bytes;

    // The number of bytes read by this function is smaller than
    // the file size because of the packet headers
    int progress = lrint(double(reader->m_bytesRead) * 114 /
                         double(reader->m_fileSize));
    if (progress > 99) progress = 99;
    reader->m_completion = progress;
    
    if (reader->m_fileSize > 0 && reader->m_progress) {
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

    if (!reader->m_commentsRead) {
        const FishSoundComment *comment = fish_sound_comment_first_byname
            (fs, "TITLE");
        if (comment && comment->value) {
            reader->m_title = comment->value;
        }
        reader->m_commentsRead = true;
    }

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
	    }
	}

	MUNLOCK_SAMPLEBLOCK(reader->m_data);
    }

    if (reader->m_cancelled) return 1;
    return 0;
}

void
OggVorbisFileReader::getSupportedExtensions(std::set<std::string> &extensions)
{
    extensions.insert("ogg");
}

#endif
#endif
