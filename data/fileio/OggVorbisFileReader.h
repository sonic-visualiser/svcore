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

#ifndef _OGG_VORBIS_FILE_READER_H_
#define _OGG_VORBIS_FILE_READER_H_

#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND

#include "CodedAudioFileReader.h"

#include <oggz/oggz.h>
#include <fishsound/fishsound.h>

#include <set>

class QProgressDialog;

class OggVorbisFileReader : public CodedAudioFileReader
{
public:
    OggVorbisFileReader(QString path, bool showProgress, CacheMode cacheMode);
    virtual ~OggVorbisFileReader();

    virtual QString getError() const { return m_error; }

    static void getSupportedExtensions(std::set<QString> &extensions);

protected:
    QString m_path;
    QString m_error;

    FishSound *m_fishSound;
    QProgressDialog *m_progress;
    size_t m_fileSize;
    size_t m_bytesRead;
    bool m_cancelled;
 
    static int readPacket(OGGZ *, ogg_packet *, long, void *);
    static int acceptFrames(FishSound *, float **, long, void *);
};

#endif
#endif

#endif
