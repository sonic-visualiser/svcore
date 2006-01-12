/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
   
    This is experimental software.  Not for distribution.
*/

#include "Transform.h"

Transform::Transform(Model *m) :
    m_input(m),
    m_output(0),
    m_detached(false),
    m_deleting(false)
{
}

Transform::~Transform()
{
    m_deleting = true;
    wait();
    if (!m_detached) delete m_output;
}

