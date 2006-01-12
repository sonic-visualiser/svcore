/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "Layer.h"
#include "View.h"

#include <iostream>

Layer::Layer(View *w)
{
    m_view = w;

    // Subclass must call this:
//    w->addLayer(this);
}

Layer::~Layer()
{
    m_view->removeLayer(this);
}

void
Layer::setObjectName(const QString &name)
{
    QObject::setObjectName(name);
    emit layerNameChanged();
}


#ifdef INCLUDE_MOCFILES
#include "Layer.moc.cpp"
#endif

