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

#ifndef _SPARSE_TIME_VALUE_MODEL_H_
#define _SPARSE_TIME_VALUE_MODEL_H_

#include "SparseValueModel.h"
#include "base/PlayParameterRepository.h"
#include "base/RealTime.h"

/**
 * Time/value point type for use in a SparseModel or SparseValueModel.
 * With this point type, the model basically represents a wiggly-line
 * plot with points at arbitrary intervals of the model resolution.
 */

struct TimeValuePoint
{
public:
    TimeValuePoint(long _frame) : frame(_frame), value(0.0f) { }
    TimeValuePoint(long _frame, float _value, QString _label) : 
	frame(_frame), value(_value), label(_label) { }

    int getDimensions() const { return 2; }
    
    long frame;
    float value;
    QString label;
    
    void toXml(QTextStream &stream, QString indent = "",
               QString extraAttributes = "") const
    {
        stream << QString("%1<point frame=\"%2\" value=\"%3\" label=\"%4\" %5/>\n")
	    .arg(indent).arg(frame).arg(value).arg(label).arg(extraAttributes);
    }

    QString toDelimitedDataString(QString delimiter, size_t sampleRate) const
    {
        QStringList list;
        list << RealTime::frame2RealTime(frame, sampleRate).toString().c_str();
        list << QString("%1").arg(value);
        list << label;
        return list.join(delimiter);
    }

    struct Comparator {
	bool operator()(const TimeValuePoint &p1,
			const TimeValuePoint &p2) const {
	    if (p1.frame != p2.frame) return p1.frame < p2.frame;
	    if (p1.value != p2.value) return p1.value < p2.value;
	    return p1.label < p2.label;
	}
    };
    
    struct OrderComparator {
	bool operator()(const TimeValuePoint &p1,
			const TimeValuePoint &p2) const {
	    return p1.frame < p2.frame;
	}
    };
};


class SparseTimeValueModel : public SparseValueModel<TimeValuePoint>
{
public:
    SparseTimeValueModel(size_t sampleRate, size_t resolution,
			 bool notifyOnAdd = true) :
	SparseValueModel<TimeValuePoint>(sampleRate, resolution,
					 notifyOnAdd)
    {
	PlayParameterRepository::getInstance()->addModel(this);
    }

    SparseTimeValueModel(size_t sampleRate, size_t resolution,
			 float valueMinimum, float valueMaximum,
			 bool notifyOnAdd = true) :
	SparseValueModel<TimeValuePoint>(sampleRate, resolution,
					 valueMinimum, valueMaximum,
					 notifyOnAdd)
    {
	PlayParameterRepository::getInstance()->addModel(this);
    }
};


#endif


    
