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

#include "DataFileReaderFactory.h"
#include "MIDIFileReader.h"
#include "CSVFileReader.h"

#include "model/Model.h"

#include <QString>

QString
DataFileReaderFactory::getKnownExtensions()
{
    return "*.svl *.csv *.lab *.mid *.txt";
}

DataFileReader *
DataFileReaderFactory::createReader(QString path, size_t mainModelSampleRate)
{
    QString err;

    DataFileReader *reader = 0;

    reader = new MIDIFileReader(path, mainModelSampleRate);
    if (reader->isOK()) return reader;
    if (reader->getError() != "") err = reader->getError();
    delete reader;

    reader = new CSVFileReader(path, mainModelSampleRate);
    if (reader->isOK()) return reader;
    if (reader->getError() != "") err = reader->getError();
    delete reader;

    return 0;
}

Model *
DataFileReaderFactory::load(QString path, size_t mainModelSampleRate)
{
    DataFileReader *reader = createReader(path, mainModelSampleRate);
    if (!reader) return NULL;

    try {
        Model *model = reader->load();
        delete reader;
        return model;
    } catch (Exception) {
        delete reader;
        throw;
    }
}

