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
    if (m_playPan != pan) {
        m_playPan = pan;
        emit playPanChanged(pan);
        emit playParametersChanged();
    }
}

void
PlayParameters::setPlayGain(float gain)
{
    if (m_playGain != gain) {
        m_playGain = gain;
        emit playGainChanged(gain);
        emit playParametersChanged();
    }
}

void
PlayParameters::setPlayPluginId(QString id)
{
    if (m_playPluginId != id) {
        m_playPluginId = id;
        emit playPluginIdChanged(id);
        emit playParametersChanged();
    }
}

void
PlayParameters::setPlayPluginConfiguration(QString configuration)
{
    if (m_playPluginConfiguration != configuration) {
        m_playPluginConfiguration = configuration;
        emit playPluginConfigurationChanged(configuration);
        emit playParametersChanged();
    }
}


