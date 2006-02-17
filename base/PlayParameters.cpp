/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "PlayParameters.h"

#include <iostream>

void
PlayParameters::setPlayMuted(bool muted)
{
    std::cerr << "PlayParameters: setPlayMuted(" << muted << ")" << std::endl;
    m_playMuted = muted;
    emit playMutedChanged(muted);
    emit playAudibleChanged(!muted);
    emit playParametersChanged();
}

void
PlayParameters::setPlayAudible(bool audible)
{
    std::cerr << "PlayParameters(" << this << "): setPlayAudible(" << audible << ")" << std::endl;
    setPlayMuted(!audible);
}

void
PlayParameters::setPlayPan(float pan)
{
    m_playPan = pan;
    emit playPanChanged(pan);
    emit playParametersChanged();
}

void
PlayParameters::setPlayGain(float gain)
{
    m_playGain = gain;
    emit playGainChanged(gain);
    emit playParametersChanged();
}


