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

#ifndef _AUDIO_FILE_READER_FACTORY_H_
#define _AUDIO_FILE_READER_FACTORY_H_

#include <QString>

#include "FileSource.h"

class AudioFileReader;

class AudioFileReaderFactory
{
public:
    /**
     * Return the file extensions that we have audio file readers for,
     * in a format suitable for use with QFileDialog.  For example,
     * "*.wav *.aiff *.ogg".
     */
    static QString getKnownExtensions();

    /**
     * Return an audio file reader initialised to the file at the
     * given path, or NULL if no suitable reader for this path is
     * available or the file cannot be opened.
     *
     * If targetRate is non-zero, the file will be resampled to that
     * rate (transparently).  You can query reader->getNativeRate()
     * if you want to find out whether the file is being resampled
     * or not.
     *
     * Caller owns the returned object and must delete it after use.
     */
    static AudioFileReader *createReader(FileSource source, size_t targetRate = 0);
};

#endif

