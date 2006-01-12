/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "PlayParameters.h"

void
PlayParameters::setPlayMuted(bool muted)
{
    m_playMuted = muted;
    emit playParametersChanged();
}


void
PlayParameters::setPlayPan(float pan)
{
    m_playPan = pan;
    emit playParametersChanged();
}


void
PlayParameters::setPlayGain(float gain)
{
    m_playGain = gain;
    emit playParametersChanged();
}


