/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "Model.h"
#include "PlayParameterRepository.h"

const int Model::COMPLETION_UNKNOWN = -1;

Model::~Model()
{
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
#ifdef INCLUDE_MOCFILES
#include "Model.moc.cpp"
#endif
#endif

