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

    reader = new WavFileReader(path);
    if (reader->isOK()) return reader;
    if (reader->getError() != "") err = reader->getError();
    delete reader;

	if (err != "") {
	std::cerr << "AudioFileReaderFactory: WAV file reader error: \""
			<< err.toStdString() << "\"" << std::endl;
	}


#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND
    reader = new OggVorbisFileReader(path, true,
                                     OggVorbisFileReader::CacheInTemporaryFile);
    if (reader->isOK()) return reader;
    if (reader->getError() != "") err = reader->getError();
    delete reader;

	if (err != "") {
	std::cerr << "AudioFileReaderFactory: Ogg file reader error: \""
			<< err.toStdString() << "\"" << std::endl;
	}

#endif
#endif
 
#ifdef HAVE_MAD
    reader = new MP3FileReader(path, true,
                               MP3FileReader::CacheInTemporaryFile);
    if (reader->isOK()) return reader;
    if (reader->getError() != "") err = reader->getError();
    delete reader;

	if (err != "") {
	std::cerr << "AudioFileReaderFactory: mp3 file reader error: \""
			<< err.toStdString() << "\"" << std::endl;
	}

#endif

    return 0;
}

