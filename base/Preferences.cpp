/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Preferences.h"

#include "Exceptions.h"

#include "TempDirectory.h"

#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QSettings>

Preferences *
Preferences::m_instance = 0;

Preferences *
Preferences::getInstance()
{
    if (!m_instance) m_instance = new Preferences();
    return m_instance;
}

Preferences::Preferences() :
    m_smoothSpectrogram(true),
    m_tuningFrequency(440),
    m_propertyBoxLayout(VerticallyStacked),
    m_windowType(HanningWindow),
    m_resampleQuality(1)
{
    QSettings settings;
    settings.beginGroup("Preferences");
    m_smoothSpectrogram = settings.value("smooth-spectrogram", true).toBool();
    m_tuningFrequency = settings.value("tuning-frequency", 440.f).toDouble();
    m_propertyBoxLayout = PropertyBoxLayout
        (settings.value("property-box-layout", int(VerticallyStacked)).toInt());
    m_windowType = WindowType
        (settings.value("window-type", int(HanningWindow)).toInt());
    m_resampleQuality = settings.value("resample-quality", 1).toInt();
    settings.endGroup();
}

Preferences::~Preferences()
{
}

Preferences::PropertyList
Preferences::getProperties() const
{
    PropertyList props;
    props.push_back("Smooth Spectrogram");
    props.push_back("Tuning Frequency");
    props.push_back("Property Box Layout");
    props.push_back("Window Type");
    props.push_back("Resample Quality");
    return props;
}

QString
Preferences::getPropertyLabel(const PropertyName &name) const
{
    if (name == "Smooth Spectrogram") {
        return tr("Smooth spectrogram display by zero padding FFT");
    }
    if (name == "Tuning Frequency") {
        return tr("Frequency of concert A");
    }
    if (name == "Property Box Layout") {
        return tr("Property box layout");
    }
    if (name == "Window Type") {
        return tr("Spectral analysis window shape");
    }
    if (name == "Resample Quality") {
        return tr("Playback resampler type");
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
    if (name == "Property Box Layout") {
        return ValueProperty;
    }
    if (name == "Window Type") {
        return ValueProperty;
    }
    if (name == "Resample Quality") {
        return ValueProperty;
    }
    return InvalidProperty;
}

int
Preferences::getPropertyRangeAndValue(const PropertyName &name,
                                      int *min, int *max, int *deflt) const
{
    if (name == "Smooth Spectrogram") {
        if (min) *min = 0;
        if (max) *max = 1;
        if (deflt) *deflt = 1;
        return m_smoothSpectrogram ? 1 : 0;
    }

    //!!! freq mapping

    if (name == "Property Box Layout") {
        if (min) *min = 0;
        if (max) *max = 1;
        if (deflt) *deflt = 0;
        return m_propertyBoxLayout == Layered ? 1 : 0;
    }        

    if (name == "Window Type") {
        if (min) *min = int(RectangularWindow);
        if (max) *max = int(BlackmanHarrisWindow);
        if (deflt) *deflt = int(HanningWindow);
        return int(m_windowType);
    }

    if (name == "Resample Quality") {
        if (min) *min = 0;
        if (max) *max = 2;
        if (deflt) *deflt = 1;
        return m_resampleQuality;
    }

    return 0;
}

QString
Preferences::getPropertyValueLabel(const PropertyName &name,
                                   int value) const
{
    if (name == "Property Box Layout") {
        if (value == 0) return tr("Show boxes for all panes");
        else return tr("Show box for current pane only");
    }
    if (name == "Window Type") {
        switch (WindowType(value)) {
        case RectangularWindow: return tr("Rectangular");
        case BartlettWindow: return tr("Triangular");
        case HammingWindow: return tr("Hamming");
        case HanningWindow: return tr("Hanning");
        case BlackmanWindow: return tr("Blackman");
        case GaussianWindow: return tr("Gaussian");
        case ParzenWindow: return tr("Parzen");
        case NuttallWindow: return tr("Nuttall");
        case BlackmanHarrisWindow: return tr("Blackman-Harris");
        }
    }
    if (name == "Resample Quality") {
        switch (value) {
        case 0: return tr("Fastest");
        case 1: return tr("Standard");
        case 2: return tr("Highest quality");
        }
    }
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
    } else if (name == "Property Box Layout") {
        setPropertyBoxLayout(value == 0 ? VerticallyStacked : Layered);
    } else if (name == "Window Type") {
        setWindowType(WindowType(value));
    } else if (name == "Resample Quality") {
        setResampleQuality(value);
    }
}

void
Preferences::setSmoothSpectrogram(bool smooth)
{
    if (m_smoothSpectrogram != smooth) {
        m_smoothSpectrogram = smooth;
        QSettings settings;
        settings.beginGroup("Preferences");
        settings.setValue("smooth-spectrogram", smooth);
        settings.endGroup();
        emit propertyChanged("Smooth Spectrogram");
    }
}

void
Preferences::setTuningFrequency(float freq)
{
    if (m_tuningFrequency != freq) {
        m_tuningFrequency = freq;
        QSettings settings;
        settings.beginGroup("Preferences");
        settings.setValue("tuning-frequency", freq);
        settings.endGroup();
        emit propertyChanged("Tuning Frequency");
    }
}

void
Preferences::setPropertyBoxLayout(PropertyBoxLayout layout)
{
    if (m_propertyBoxLayout != layout) {
        m_propertyBoxLayout = layout;
        QSettings settings;
        settings.beginGroup("Preferences");
        settings.setValue("property-box-layout", int(layout));
        settings.endGroup();
        emit propertyChanged("Property Box Layout");
    }
}

void
Preferences::setWindowType(WindowType type)
{
    if (m_windowType != type) {
        m_windowType = type;
        QSettings settings;
        settings.beginGroup("Preferences");
        settings.setValue("window-type", int(type));
        settings.endGroup();
        emit propertyChanged("Window Type");
    }
}

void
Preferences::setResampleQuality(int q)
{
    if (m_resampleQuality != q) {
        m_resampleQuality = q;
        QSettings settings;
        settings.beginGroup("Preferences");
        settings.setValue("resample-quality", q);
        settings.endGroup();
        emit propertyChanged("Resample Quality");
    }
}
