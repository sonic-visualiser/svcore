/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _PROPERTY_CONTAINER_H_
#define _PROPERTY_CONTAINER_H_

#include <QString>
#include <QObject>
#include <vector>

class PlayParameters;

class PropertyContainer
{
public:
    virtual ~PropertyContainer() { }

    typedef QString PropertyName;
    typedef std::vector<PropertyName> PropertyList;
    
    enum PropertyType {
	ToggleProperty, // on or off
	RangeProperty, // range of integers
	ValueProperty, // range of integers given string labels
	ColourProperty, // colours, get/set as qRgb
	InvalidProperty, // property not found!
    };

    /**
     * Get a list of the names of all the supported properties on this
     * container.  Note that these should already have been
     * internationalized with a call to tr() or equivalent.  If the
     * container needs to test for equality with string literals
     * subsequently, it must be sure to call tr() again on the strings
     * in order to ensure they match.
     */
    virtual PropertyList getProperties() const;

    /**
     * Return the type of the given property, or InvalidProperty if
     * the property is not supported on this container.
     */
    virtual PropertyType getPropertyType(const PropertyName &) const;

    /**
     * If this property has something in common with other properties
     * on this container, return a name that can be used to group them
     * (in order to save screen space, for example).  e.g. "Window
     * Type" and "Window Size" might both have a group name of "Window".
     * If this property is not groupable, return the empty string.
     */
    virtual QString getPropertyGroupName(const PropertyName &) const;

    /**
     * Return the minimum and maximum values for the given property
     * and its current value in this container.  Min and/or max may be
     * passed as NULL if their values are not required.
     */
    virtual int getPropertyRangeAndValue(const PropertyName &,
					 int *min, int *max) const;

    /**
     * If the given property is a ValueProperty, return the display
     * label to be used for the given value for that property.
     */
    virtual QString getPropertyValueLabel(const PropertyName &,
					  int value) const;

    /**
     * Set a property.  This is used for all property types.  For
     * boolean properties, zero is false and non-zero true; for
     * colours, the integer value should be treated as a qRgb.
     */
    virtual void setProperty(const PropertyName &, int value);

    virtual QString getPropertyContainerName() const = 0;
    virtual QString getPropertyContainerIconName() const = 0;

    virtual PlayParameters *getPlayParameters() const { return 0; }
};

#endif
