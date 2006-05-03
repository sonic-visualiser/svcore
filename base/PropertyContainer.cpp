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

#include "PropertyContainer.h"
#include "CommandHistory.h"

#include <iostream>

PropertyContainer::PropertyList
PropertyContainer::getProperties() const
{
    return PropertyList();
}

//QString
//PropertyContainer::getPropertyLabel(const PropertyName &) const
//{
//    return "";
//}

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
PropertyContainer::setProperty(const PropertyName &name, int) 
{
    std::cerr << "WARNING: PropertyContainer[" << getPropertyContainerName().toStdString() << "]::setProperty(" << name.toStdString() << "): no implementation in subclass!" << std::endl;
}

void
PropertyContainer::setPropertyWithCommand(const PropertyName &name, int value)
{
    int currentValue = getPropertyRangeAndValue(name, 0, 0);
    if (value == currentValue) return;

    CommandHistory::getInstance()->addCommand
	(new SetPropertyCommand(this, name, value), true, true); // bundled
}

PropertyContainer::SetPropertyCommand::SetPropertyCommand(PropertyContainer *pc,
							  const PropertyName &pn,
							  int value) :
    m_pc(pc),
    m_pn(pn),
    m_value(value),
    m_oldValue(0)
{
}

void
PropertyContainer::SetPropertyCommand::execute()
{
    m_oldValue = m_pc->getPropertyRangeAndValue(m_pn, 0, 0);
    m_pc->setProperty(m_pn, m_value);
}

void
PropertyContainer::SetPropertyCommand::unexecute() 
{
    m_pc->setProperty(m_pn, m_oldValue);
}

QString
PropertyContainer::SetPropertyCommand::getName() const
{
    return m_pc->tr("Set %1 Property").arg(m_pn);
}

