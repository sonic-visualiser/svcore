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

#include "Preferences.h"

Preferences *
Preferences::m_instance = new Preferences();

Preferences::Preferences() :
    m_smoothSpectrogram(false),
    m_tuningFrequency(440)
{
}

Preferences::PropertyList
Preferences::getProperties() const
{
    PropertyList props;
    props.push_back("Smooth Spectrogram");
    props.push_back("Tuning Frequency");
    return props;
}

QString
Preferences::getPropertyLabel(const PropertyName &name) const
{
    if (name == "Smooth Spectrogram") {
        return tr("Spectrogram Display Smoothing");
    }
    if (name == "Tuning Frequency") {
        return tr("Tuning Frequency (concert A)");
    }
    return name;
}

Preferences::PropertyType
Preferences::getPropertyType(const PropertyName &name) const
{
    if (name == "Smooth Spectrogram") {
        return ToggleProperty;
    }
    if (name == "Tuning Frequency") {
        return RangeProperty;
    }
    return InvalidProperty;
}

int
Preferences::getPropertyRangeAndValue(const PropertyName &name,
                                      int *min, int *max) const
{
    if (name == "Smooth Spectrogram") {
        if (min) *min = 0;
        if (max) *max = 1;
        return m_smoothSpectrogram ? 1 : 0;
    }

    //!!! freq mapping

    return 0;
}

QString
Preferences::getPropertyValueLabel(const PropertyName &name,
                                   int value) const
{
    //!!!
    return "";
}

QString
Preferences::getPropertyContainerName() const
{
    return tr("Preferences");
}

QString
Preferences::getPropertyContainerIconName() const
{
    return "preferences";
}

void
Preferences::setProperty(const PropertyName &name, int value) 
{
    if (name == "Smooth Spectrogram") {
        setSmoothSpectrogram(value > 0.1);
    } else if (name == "Tuning Frequency") {
        //!!!
    }
}

void
Preferences::setSmoothSpectrogram(bool smooth)
{
    m_smoothSpectrogram = smooth;
//!!!    emit 
}

void
Preferences::setTuningFrequency(float freq)
{
    m_tuningFrequency = freq;
    //!!! emit
}

