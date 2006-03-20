/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2006 Chris Cannam.
*/

#include "RealTimePluginInstance.h"
#include "RealTimePluginFactory.h"

#include <iostream>


RealTimePluginInstance::~RealTimePluginInstance()
{
    std::cerr << "RealTimePluginInstance::~RealTimePluginInstance" << std::endl;

    if (m_factory) {
	std::cerr << "Asking factory to release " << m_identifier.toStdString() << std::endl;

	m_factory->releasePlugin(this, m_identifier);
    }
}


