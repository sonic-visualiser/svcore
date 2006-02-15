/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "PlayParameterRepository.h"
#include "PlayParameters.h"

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

PlayParameters *
PlayParameterRepository::getPlayParameters(const Model *model)
{
    if (m_playParameters.find(model) == m_playParameters.end()) {
	// Give all models the same type of play parameters for the moment
	m_playParameters[model] = new PlayParameters;
    }

    return m_playParameters[model];
}

void
PlayParameterRepository::clear()
{
    while (!m_playParameters.empty()) {
	delete m_playParameters.begin()->second;
	m_playParameters.erase(m_playParameters.begin());
    }
}

