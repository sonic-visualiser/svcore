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

#ifndef _SPARSE_ONE_DIMENSIONAL_MODEL_H_
#define _SPARSE_ONE_DIMENSIONAL_MODEL_H_

#include "SparseModel.h"
#include "base/PlayParameterRepository.h"
#include "base/RealTime.h"

struct OneDimensionalPoint
{
public:
    OneDimensionalPoint(long _frame) : frame(_frame) { }
    OneDimensionalPoint(long _frame, QString _label) : frame(_frame), label(_label) { }

    int getDimensions() const { return 1; }
    
    long frame;
    QString label;

    QString getLabel() const { return label; }

    void toXml(QTextStream &stream,
               QString indent = "",
               QString extraAttributes = "") const
    {
        stream << QString("%1<point frame=\"%2\" label=\"%3\" %4/>\n")
	    .arg(indent).arg(frame).arg(label).arg(extraAttributes);
    }

    QString toDelimitedDataString(QString delimiter, size_t sampleRate) const
    {
        QStringList list;
        list << RealTime::frame2RealTime(frame, sampleRate).toString().c_str();
        if (label != "") list << label;
        return list.join(delimiter);
    }

    struct Comparator {
	bool operator()(const OneDimensionalPoint &p1,
			const OneDimensionalPoint &p2) const {
	    if (p1.frame != p2.frame) return p1.frame < p2.frame;
	    return p1.label < p2.label;
	}
    };
    
    struct OrderComparator {
	bool operator()(const OneDimensionalPoint &p1,
			const OneDimensionalPoint &p2) const {
	    return p1.frame < p2.frame;
	}
    };
};


class SparseOneDimensionalModel : public SparseModel<OneDimensionalPoint>
{
public:
    SparseOneDimensionalModel(size_t sampleRate, size_t resolution,
			      bool notifyOnAdd = true) :
	SparseModel<OneDimensionalPoint>(sampleRate, resolution, notifyOnAdd)
    {
	PlayParameterRepository::getInstance()->addModel(this);
    }

    int getIndexOf(const Point &point) {
	// slow
	int i = 0;
	Point::Comparator comparator;
	for (PointList::const_iterator j = m_points.begin();
	     j != m_points.end(); ++j, ++i) {
	    if (!comparator(*j, point) && !comparator(point, *j)) return i;
	}
	return -1;
    }

    QString getTypeName() const { return tr("Sparse 1-D"); }
};

#endif


    
