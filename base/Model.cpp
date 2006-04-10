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

#include "Model.h"
#include "PlayParameterRepository.h"

#include <iostream>

const int Model::COMPLETION_UNKNOWN = -1;

Model::~Model()
{
    std::cerr << "Model::~Model(" << this << ")" << std::endl;

    // Subclasses have to handle adding themselves to the repository,
    // if they want to be played.  We can't do it from here because
    // the repository would be unable to tell whether we were playable
    // or not (because dynamic_cast won't work from the base class ctor)
    PlayParameterRepository::instance()->removeModel(this);
}

QString
Model::toXmlString(QString indent, QString extraAttributes) const
{
    QString s;
    
    s += indent;

    s += QString("<model id=\"%1\" name=\"%2\" sampleRate=\"%3\" start=\"%4\" end=\"%5\" %6/>\n")
	.arg(getObjectExportId(this))
	.arg(objectName())
	.arg(getSampleRate())
	.arg(getStartFrame())
	.arg(getEndFrame())
	.arg(extraAttributes);

    return s;
}

#ifdef INCLUDE_MOCFILES
#include "Model.moc.cpp"
#endif

