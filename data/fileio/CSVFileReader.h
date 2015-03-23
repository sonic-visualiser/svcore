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

#ifndef _CSV_FILE_READER_H_
#define _CSV_FILE_READER_H_

#include "DataFileReader.h"

#include "CSVFormat.h"

#include <QList>
#include <QStringList>

class QFile;

class CSVFileReader : public DataFileReader
{
public:
    /**
     * Construct a CSVFileReader to read the CSV file at the given
     * path, with the given format.
     */
    CSVFileReader(QString path, CSVFormat format, int mainModelSampleRate);

    /**
     * Construct a CSVFileReader to read from the given
     * QIODevice. Caller retains ownership of the QIODevice: the
     * CSVFileReader will not close or delete it and it must outlive
     * the CSVFileReader.
     */
    CSVFileReader(QIODevice *device, CSVFormat format, int mainModelSampleRate);

    virtual ~CSVFileReader();

    virtual bool isOK() const;
    virtual QString getError() const;

    virtual Model *load() const;

protected:
    CSVFormat m_format;
    QIODevice *m_device;
    bool m_ownDevice;
    QString m_filename;
    QString m_error;
    mutable int m_warnings;
    int m_mainModelSampleRate;

    int convertTimeValue(QString, int lineno, int sampleRate,
                            int windowSize) const;
};


#endif

