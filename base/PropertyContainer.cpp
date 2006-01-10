/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005
    
    This is experimental software.  Not for distribution.
*/

#include "PropertyContainer.h"

PropertyContainer::PropertyList
PropertyContainer::getProperties() const
{
    return PropertyList();
}

PropertyContainer::PropertyType
PropertyContainer::getPropertyType(const PropertyName &) const
{
    return InvalidProperty;
}

QString
PropertyContainer::getPropertyGroupName(const PropertyName &) const
{
    return QString();
}

int
PropertyContainer::getPropertyRangeAndValue(const PropertyName &, int *min, int *max) const
{
    if (min) *min = 0;
    if (max) *max = 0;
    return 0;
}

QString
PropertyContainer::getPropertyValueLabel(const PropertyName &, int) const
{
    return QString();
}

void
PropertyContainer::setProperty(const PropertyName &, int) 
{
}

