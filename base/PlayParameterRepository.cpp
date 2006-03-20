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

#include "PlayParameterRepository.h"
#include "PlayParameters.h"

//!!! shouldn't be including this here -- restructure needed
#include "audioio/AudioGenerator.h"

#include <iostream>

PlayParameterRepository *
PlayParameterRepository::m_instance = new PlayParameterRepository;

PlayParameterRepository *
PlayParameterRepository::instance()
{
    return m_instance;
}

PlayParameterRepository::~PlayParameterRepository()
{
}

void
PlayParameterRepository::addModel(const Model *model)
{
    if (!getPlayParameters(model)) {

	// Give all models the same type of play parameters for the
	// moment, provided they can be played at all

	if (AudioGenerator::canPlay(model)) {

	    std::cerr << "PlayParameterRepository: Adding play parameters for " << model << std::endl;

	    m_playParameters[model] = new PlayParameters;

	    connect(m_playParameters[model], SIGNAL(playParametersChanged()),
		    this, SLOT(playParametersChanged()));

	} else {

	    std::cerr << "PlayParameterRepository: Model " << model << " not playable" <<  std::endl;
	}	    
    }
}    

void
PlayParameterRepository::removeModel(const Model *model)
{
    delete m_playParameters[model];
    m_playParameters.erase(model);
}

PlayParameters *
PlayParameterRepository::getPlayParameters(const Model *model) const
{
    if (m_playParameters.find(model) == m_playParameters.end()) return 0;
    return m_playParameters.find(model)->second;
}

void
PlayParameterRepository::playParametersChanged()
{
    emit playParametersChanged(dynamic_cast<PlayParameters *>(sender()));
}

void
PlayParameterRepository::clear()
{
    std::cerr << "PlayParameterRepository: PlayParameterRepository::clear" << std::endl;
    while (!m_playParameters.empty()) {
	delete m_playParameters.begin()->second;
	m_playParameters.erase(m_playParameters.begin());
    }
}

