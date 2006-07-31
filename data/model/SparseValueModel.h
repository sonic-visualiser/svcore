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

#ifndef _SPARSE_VALUE_MODEL_H_
#define _SPARSE_VALUE_MODEL_H_

#include "SparseModel.h"
#include "UnitDatabase.h"

/**
 * Model containing sparse data (points with some properties) of which
 * one of the properties is an arbitrary float value.  The other
 * properties depend on the point type.
 */

template <typename PointType>
class SparseValueModel : public SparseModel<PointType>
{
public:
    SparseValueModel(size_t sampleRate, size_t resolution,
		     float valueMinimum, float valueMaximum,
		     bool notifyOnAdd = true) :
	SparseModel<PointType>(sampleRate, resolution, notifyOnAdd),
	m_valueMinimum(valueMinimum),
	m_valueMaximum(valueMaximum)
    { }

    using SparseModel<PointType>::m_points;
    using SparseModel<PointType>::modelChanged;

    virtual float getValueMinimum() const { return m_valueMinimum; }
    virtual float getValueMaximum() const { return m_valueMaximum; }

    virtual QString getScaleUnits() const { return m_units; }
    virtual void setScaleUnits(QString units) {
        m_units = units;
        UnitDatabase::getInstance()->registerUnit(units);
    }

    virtual void addPoint(const PointType &point)
    {
	bool allChange = false;
	if (m_points.empty() || point.value < m_valueMinimum) {
	    m_valueMinimum = point.value; allChange = true;
	}
	if (m_points.empty() || point.value > m_valueMaximum) {
	    m_valueMaximum = point.value; allChange = true;
	}

	SparseModel<PointType>::addPoint(point);
	if (allChange) emit modelChanged();
    }

    virtual void deletePoint(const PointType &point)
    {
	SparseModel<PointType>::deletePoint(point);

	if (point.value == m_valueMinimum ||
	    point.value == m_valueMaximum) {

	    float formerMin = m_valueMinimum, formerMax = m_valueMaximum;

	    for (typename SparseModel<PointType>::PointList::const_iterator i
		     = m_points.begin();
		 i != m_points.end(); ++i) {

		if (i == m_points.begin() || i->value < m_valueMinimum) {
		    m_valueMinimum = i->value;
		} 
		if (i == m_points.begin() || i->value > m_valueMaximum) {
		    m_valueMaximum = i->value;
		} 
	    }

	    if (formerMin != m_valueMinimum || formerMax != m_valueMaximum) {
		emit modelChanged();
	    }
	}
    }

    virtual QString toXmlString(QString indent = "",
				QString extraAttributes = "") const
    {
	return SparseModel<PointType>::toXmlString
	    (indent,
	     QString("%1 minimum=\"%2\" maximum=\"%3\" units=\"%4\"")
	     .arg(extraAttributes).arg(m_valueMinimum).arg(m_valueMaximum)
             .arg(this->encodeEntities(m_units)));
    }

protected:
    float m_valueMinimum;
    float m_valueMaximum;
    QString m_units;
};


#endif

