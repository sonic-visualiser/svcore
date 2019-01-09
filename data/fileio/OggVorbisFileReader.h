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

#ifndef SV_OGG_VORBIS_FILE_READER_H
#define SV_OGG_VORBIS_FILE_READER_H

#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND

#include "CodedAudioFileReader.h"

#include "base/Thread.h"
#include <oggz/oggz.h>
#include <fishsound/fishsound.h>

#include <cstdio>

#include <set>

class ProgressReporter;

class OggVorbisFileReader : public CodedAudioFileReader
{
    Q_OBJECT

public:
    OggVorbisFileReader(FileSource source,
                        DecodeMode decodeMode,
                        CacheMode cacheMode,
                        sv_samplerate_t targetRate = 0,
                        bool normalised = false,
                        ProgressReporter *reporter = nullptr);
    virtual ~OggVorbisFileReader();

    QString getError() const override { return m_error; }

    QString getLocation() const override { return m_source.getLocation(); }
    QString getTitle() const override { return m_title; }
    QString getMaker() const override { return m_maker; }
    TagMap getTags() const override { return m_tags; }
    
    static void getSupportedExtensions(std::set<QString> &extensions);
    static bool supportsExtension(QString ext);
    static bool supportsContentType(QString type);
    static bool supports(FileSource &source);

    int getDecodeCompletion() const override { return m_completion; }

    bool isUpdating() const override {
        return m_decodeThread && m_decodeThread->isRunning();
    }

public slots:
    void cancelled();

protected:
    FileSource m_source;
    QString m_path;
    QString m_error;
    QString m_title;
    QString m_maker;
    TagMap m_tags;

    QFile *m_qfile;
    FILE *m_ffile;
    OGGZ *m_oggz;
    FishSound *m_fishSound;
    ProgressReporter *m_reporter;
    sv_frame_t m_fileSize;
    sv_frame_t m_bytesRead;
    bool m_commentsRead;
    bool m_cancelled;
    int m_completion;
 
    static int readPacket(OGGZ *, ogg_packet *, long, void *);
    static int acceptFrames(FishSound *, float **, long, void *);

    class DecodeThread : public Thread
    {
    public:
        DecodeThread(OggVorbisFileReader *reader) : m_reader(reader) { }
        void run() override;

    protected:
        OggVorbisFileReader *m_reader; 
    };

    DecodeThread *m_decodeThread;
};

#endif
#endif

#endif
