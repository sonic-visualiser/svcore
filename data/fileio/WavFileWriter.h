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

#ifndef _WAV_FILE_WRITER_H_
#define _WAV_FILE_WRITER_H_

#include <QString>

class DenseTimeValueModel;
class MultiSelection;

class WavFileWriter
{
public:
    WavFileWriter(QString path, size_t sampleRate,
		  DenseTimeValueModel *source,
		  MultiSelection *selection);
    virtual ~WavFileWriter();

    bool isOK() const;

    virtual QString getError() const;

    void write();

protected:
    QString m_path;
    size_t m_sampleRate;
    DenseTimeValueModel *m_model;
    MultiSelection *m_selection;
    QString m_error;
};


#endif
