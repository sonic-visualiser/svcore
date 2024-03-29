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

#ifndef SV_PROPERTY_CONTAINER_H
#define SV_PROPERTY_CONTAINER_H

#include "Command.h"

#include <QString>
#include <QObject>
#include <vector>
#include <memory>

namespace sv {

class PlayParameters;
class RangeMapper;

class PropertyContainer : public QObject
{
    Q_OBJECT

public:
    virtual ~PropertyContainer() { }

    typedef QString PropertyName;
    typedef std::vector<PropertyName> PropertyList;
    
    enum PropertyType {
        ToggleProperty, // on or off
        RangeProperty, // range of integers
        ValueProperty, // range of integers given string labels
        ColourProperty, // colours, get/set as ColourDatabase indices
        ColourMapProperty, // colour maps, get/set as ColourMapper::StandardMap enum
        UnitsProperty, // unit from UnitDatabase, get/set unit id
        InvalidProperty, // property not found!
    };

    /**
     * Get a list of the names of all the supported properties on this
     * container.  These should be fixed (i.e. not internationalized).
     */
    virtual PropertyList getProperties() const;

    /**
     * Return the human-readable (and i18n'ised) name of a property.
     */
    virtual QString getPropertyLabel(const PropertyName &) const = 0;

    /**
     * Return the type of the given property, or InvalidProperty if
     * the property is not supported on this container.
     */
    virtual PropertyType getPropertyType(const PropertyName &) const;

    /**
     * Return an icon for the property, if any.
     */
    virtual QString getPropertyIconName(const PropertyName &) const;

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
                                         int *min, int *max, int *deflt) const;

    /**
     * If the given property is a ValueProperty, return the display
     * label to be used for the given value for that property.
     */
    virtual QString getPropertyValueLabel(const PropertyName &,
                                          int value) const;

    /**
     * If the given property is a ValueProperty, return the icon to be
     * used for the given value for that property, if any.
     */
    virtual QString getPropertyValueIconName(const PropertyName &,
                                             int value) const;

    /**
     * If the given property is a RangeProperty, return a new
     * RangeMapper object mapping its integer range onto an underlying
     * floating point value range for human-intelligible display, if
     * appropriate.  The RangeMapper should be allocated with new, and
     * the caller takes responsibility for deleting it.  Return NULL
     * (as in the default implementation) if there is no such mapping.
     */
    virtual RangeMapper *getNewPropertyRangeMapper(const PropertyName &) const;

    virtual QString getPropertyContainerName() const = 0;
    virtual QString getPropertyContainerIconName() const = 0;

    /**
     * Return the play parameters for this layer, if any. The return
     * value is a shared_ptr that, if not null, can be passed to
     * e.g. PlayParameterRepository::EditCommand to change the
     * parameters.
     */
    virtual std::shared_ptr<PlayParameters> getPlayParameters() { return {}; }

signals:
    void propertyChanged(PropertyContainer::PropertyName);

public slots:
    /**
     * Set a property.  This is used for all property types.  For
     * boolean properties, zero is false and non-zero true; for
     * colours, the integer value is an index into the colours in the
     * global ColourDatabase.
     */
    virtual void setProperty(const PropertyName &, int value);

    /**
     * Obtain a command that sets the given property, which can be
     * added to the command history for undo/redo.  Returns NULL
     * if the property is already set to the given value.
     */
    virtual Command *getSetPropertyCommand(const PropertyName &, int value);

    /**
     * Set a property using a fuzzy match.  Compare nameString with
     * the property labels and underlying names, and if it matches one
     * (with preference given to labels), try to convert valueString
     * appropriately and set it.  The valueString should contain a
     * value label for value properties, a mapped value for range
     * properties, "on" or "off" for toggle properties, a colour or
     * unit name, or the underlying integer value for the property.
     *
     * Note that as property and value labels may be translatable, the
     * results of this function may vary by locale.  It is intended
     * for handling user-originated strings, _not_ persistent storage.
     *
     * The default implementation should work for most subclasses.
     */
    virtual void setPropertyFuzzy(QString nameString, QString valueString);

    /**
     * As above, but returning a command.
     */
    virtual Command *getSetPropertyCommand(QString nameString, QString valueString);

protected:

    class SetPropertyCommand : public Command
    {
    public:
        SetPropertyCommand(PropertyContainer *pc, const PropertyName &pn, int);
        virtual ~SetPropertyCommand() { }

        void execute() override;
        void unexecute() override;
        QString getName() const override;

    protected:
        PropertyContainer *m_pc;
        PropertyName m_pn;
        int m_value;
        int m_oldValue;
    };

    virtual bool convertPropertyStrings(QString nameString, QString valueString,
                                        PropertyName &name, int &value);
};

} // end namespace sv

#endif
