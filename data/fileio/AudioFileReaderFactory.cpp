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

#include "AudioFileReaderFactory.h"

#include "WavFileReader.h"
#include "OggVorbisFileReader.h"
#include "MP3FileReader.h"

#include <QString>
#include <QFileInfo>
#include <iostream>

QString
AudioFileReaderFactory::getKnownExtensions()
{
    std::set<QString> extensions;

    WavFileReader::getSupportedExtensions(extensions);
#ifdef HAVE_MAD
    MP3FileReader::getSupportedExtensions(extensions);
#endif
#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND
    OggVorbisFileReader::getSupportedExtensions(extensions);
#endif
#endif

    QString rv;
    for (std::set<QString>::const_iterator i = extensions.begin();
         i != extensions.end(); ++i) {
        if (i != extensions.begin()) rv += " ";
        rv += "*." + *i;
    }

    return rv;
}

AudioFileReader *
AudioFileReaderFactory::createReader(QString path)
{
    QString err;

    AudioFileReader *reader = 0;

    // First try to construct a preferred reader based on the
    // extension.  If we can't identify one or it fails to load the
    // file, fall back to trying all readers in no particular order.

    QString ext = QFileInfo(path).suffix().toLower();
    std::set<QString> extensions;

    WavFileReader::getSupportedExtensions(extensions);
    if (extensions.find(ext) != extensions.end()) {
        reader = new WavFileReader(path);
    }
    
#ifdef HAVE_MAD
    if (!reader) {
        extensions.clear();
        MP3FileReader::getSupportedExtensions(extensions);
        if (extensions.find(ext) != extensions.end()) {
            reader = new MP3FileReader
                (path,
                 MP3FileReader::DecodeAtOnce,
                 MP3FileReader::CacheInTemporaryFile);
        }
    }
#endif
#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND
    if (!reader) {
        extensions.clear();
        OggVorbisFileReader::getSupportedExtensions(extensions);
        if (extensions.find(ext) != extensions.end()) {
            reader = new OggVorbisFileReader
                (path, 
                 OggVorbisFileReader::DecodeAtOnce,
                 OggVorbisFileReader::CacheInTemporaryFile);
        }
    }
#endif
#endif

    if (reader) {
        if (reader->isOK()) return reader;
        if (reader->getError() != "") {
            std::cerr << "AudioFileReaderFactory: Preferred reader for "
                      << "extension \"" << ext.toStdString() << "\" failed: \""
                      << reader->getError().toStdString() << "\"" << std::endl;
        } else {
            std::cerr << "AudioFileReaderFactory: Preferred reader for "
                      << "extension \"" << ext.toStdString() << "\" failed"
                      << std::endl;
        }            
        delete reader;
        reader = 0;
    }

    reader = new WavFileReader(path);
    if (reader->isOK()) return reader;
    if (reader->getError() != "") {
	std::cerr << "AudioFileReaderFactory: WAV file reader error: \""
                  << reader->getError().toStdString() << "\"" << std::endl;
    } else {
	std::cerr << "AudioFileReaderFactory: WAV file reader failed"
                  << std::endl;
    }        
    delete reader;

#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND
    reader = new OggVorbisFileReader
        (path,
         OggVorbisFileReader::DecodeAtOnce,
         OggVorbisFileReader::CacheInTemporaryFile);
    if (reader->isOK()) return reader;
    if (reader->getError() != "") {
	std::cerr << "AudioFileReaderFactory: Ogg file reader error: \""
                  << reader->getError().toStdString() << "\"" << std::endl;
    } else {
	std::cerr << "AudioFileReaderFactory: Ogg file reader failed"
                  << std::endl;
    }        
    delete reader;
#endif
#endif
 
#ifdef HAVE_MAD
    reader = new MP3FileReader
        (path,
         MP3FileReader::DecodeAtOnce,
         MP3FileReader::CacheInTemporaryFile);
    if (reader->isOK()) return reader;
    if (reader->getError() != "") {
	std::cerr << "AudioFileReaderFactory: MP3 file reader error: \""
                  << reader->getError().toStdString() << "\"" << std::endl;
    } else {
	std::cerr << "AudioFileReaderFactory: MP3 file reader failed"
                  << std::endl;
    }        
    delete reader;
#endif

    return 0;
}

