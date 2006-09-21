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

#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

#include "PropertyContainer.h"

#include "Window.h"

class Preferences : public PropertyContainer
{
    Q_OBJECT

public:
    static Preferences *getInstance();

    virtual PropertyList getProperties() const;
    virtual QString getPropertyLabel(const PropertyName &) const;
    virtual PropertyType getPropertyType(const PropertyName &) const;
    virtual int getPropertyRangeAndValue(const PropertyName &, int *, int *) const;
    virtual QString getPropertyValueLabel(const PropertyName &, int value) const;
    virtual QString getPropertyContainerName() const;
    virtual QString getPropertyContainerIconName() const;

    bool getSmoothSpectrogram() const { return m_smoothSpectrogram; }
    float getTuningFrequency() const { return m_tuningFrequency; }
    WindowType getWindowType() const { return m_windowType; }
    int getResampleQuality() const { return m_resampleQuality; }

    //!!! harmonise with PaneStack
    enum PropertyBoxLayout {
        VerticallyStacked,
        Layered
    };
    PropertyBoxLayout getPropertyBoxLayout() const { return m_propertyBoxLayout; }

public slots:
    virtual void setProperty(const PropertyName &, int);

    void setSmoothSpectrogram(bool smooth);
    void setTuningFrequency(float freq);
    void setPropertyBoxLayout(PropertyBoxLayout layout);
    void setWindowType(WindowType type);
    void setResampleQuality(int quality);

private:
    Preferences(); // may throw DirectoryCreationFailed
    virtual ~Preferences();

    static Preferences *m_instance;

    bool m_smoothSpectrogram;
    float m_tuningFrequency;
    PropertyBoxLayout m_propertyBoxLayout;
    WindowType m_windowType;
    int m_resampleQuality;
};

#endif
