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

#include "WavFileWriter.h"

#include "model/DenseTimeValueModel.h"
#include "base/Selection.h"

#include <QFileInfo>
#include <sndfile.h>

#include <iostream>

WavFileWriter::WavFileWriter(QString path,
			     size_t sampleRate,
			     DenseTimeValueModel *source,
			     MultiSelection *selection) :
    m_path(path),
    m_sampleRate(sampleRate),
    m_model(source),
    m_selection(selection)
{
}

WavFileWriter::~WavFileWriter()
{
}

bool
WavFileWriter::isOK() const
{
    return (m_error.isEmpty());
}

QString
WavFileWriter::getError() const
{
    return m_error;
}

void
WavFileWriter::write()
{
    int channels = m_model->getChannelCount();

    SF_INFO fileInfo;
    fileInfo.samplerate = m_sampleRate;
    fileInfo.channels = channels;
    fileInfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    
    SNDFILE *file = sf_open(m_path.toLocal8Bit(), SFM_WRITE, &fileInfo);
    if (!file) {
	std::cerr << "WavFileWriter::write: Failed to open file ("
		  << sf_strerror(file) << ")" << std::endl;
	m_error = QString("Failed to open audio file '%1' for writing")
	    .arg(m_path);
	return;
    }

    MultiSelection *selection = m_selection;

    if (!m_selection) {
	selection = new MultiSelection;
	selection->setSelection(Selection(m_model->getStartFrame(),
					  m_model->getEndFrame()));
    }

    size_t bs = 2048;
    float *ub = new float[bs]; // uninterleaved buffer (one channel)
    float *ib = new float[bs * channels]; // interleaved buffer

    for (MultiSelection::SelectionList::iterator i =
	     selection->getSelections().begin();
	 i != selection->getSelections().end(); ++i) {
	
	size_t f0(i->getStartFrame()), f1(i->getEndFrame());

	for (size_t f = f0; f < f1; f += bs) {
	    
	    size_t n = std::min(bs, f1 - f);

	    for (int c = 0; c < channels; ++c) {
		m_model->getValues(c, f, f + n, ub);
		for (size_t i = 0; i < n; ++i) {
		    ib[i * channels + c] = ub[i];
		}
	    }	    

	    sf_count_t written = sf_writef_float(file, ib, n);

	    if (written < n) {
		m_error = QString("Only wrote %1 of %2 frames at file frame %3")
		    .arg(written).arg(n).arg(f);
		break;
	    }
	}
    }

    sf_close(file);

    delete[] ub;
    delete[] ib;
    if (!m_selection) delete selection;
}


	    
	    
