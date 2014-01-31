/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef TEST_RANGE_MAPPER_H
#define TEST_RANGE_MAPPER_H

#include "../RangeMapper.h"

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>

using namespace std;

class RangeMapperTest : public QObject
{
    Q_OBJECT

private slots:
    void linearUpForward()
    {
	LinearRangeMapper rm(1, 8, 0.5, 4.0, "x", false);
	QCOMPARE(rm.getUnit(), QString("x"));
	QCOMPARE(rm.getPositionForValue(0.5), 1);
	QCOMPARE(rm.getPositionForValue(4.0), 8);
	QCOMPARE(rm.getPositionForValue(3.0), 6);
	QCOMPARE(rm.getPositionForValue(3.1), 6);
	QCOMPARE(rm.getPositionForValue(3.4), 7);
	QCOMPARE(rm.getPositionForValue(0.2), 1);
	QCOMPARE(rm.getPositionForValue(-12), 1);
	QCOMPARE(rm.getPositionForValue(6.1), 8);
    }

    void linearDownForward()
    {
	LinearRangeMapper rm(1, 8, 0.5, 4.0, "x", true);
	QCOMPARE(rm.getUnit(), QString("x"));
	QCOMPARE(rm.getPositionForValue(0.5), 8);
	QCOMPARE(rm.getPositionForValue(4.0), 1);
	QCOMPARE(rm.getPositionForValue(3.0), 3);
	QCOMPARE(rm.getPositionForValue(3.1), 3);
	QCOMPARE(rm.getPositionForValue(3.4), 2);
	QCOMPARE(rm.getPositionForValue(0.2), 8);
	QCOMPARE(rm.getPositionForValue(-12), 8);
	QCOMPARE(rm.getPositionForValue(6.1), 1);
    }

    void linearUpBackward()
    {
	LinearRangeMapper rm(1, 8, 0.5, 4.0, "x", false);
	QCOMPARE(rm.getUnit(), QString("x"));
	QCOMPARE(rm.getValueForPosition(1), 0.5);
	QCOMPARE(rm.getValueForPosition(8), 4.0);
	QCOMPARE(rm.getValueForPosition(6), 3.0);
	QCOMPARE(rm.getValueForPosition(7), 3.5);
	QCOMPARE(rm.getValueForPosition(0), rm.getValueForPosition(1));
	QCOMPARE(rm.getValueForPosition(9), rm.getValueForPosition(8));
    }

    void linearDownBackward()
    {
	LinearRangeMapper rm(1, 8, 0.5, 4.0, "x", true);
	QCOMPARE(rm.getUnit(), QString("x"));
	QCOMPARE(rm.getValueForPosition(8), 0.5);
	QCOMPARE(rm.getValueForPosition(1), 4.0);
	QCOMPARE(rm.getValueForPosition(3), 3.0);
	QCOMPARE(rm.getValueForPosition(2), 3.5);
	QCOMPARE(rm.getValueForPosition(0), rm.getValueForPosition(1));
	QCOMPARE(rm.getValueForPosition(9), rm.getValueForPosition(8));
    }

    void logUpForward()
    {
	LogRangeMapper rm(3, 7, 10, 100000, "x", false);
	QCOMPARE(rm.getUnit(), QString("x"));
	QCOMPARE(rm.getPositionForValue(10.0), 3);
	QCOMPARE(rm.getPositionForValue(100000.0), 7);
	QCOMPARE(rm.getPositionForValue(1.0), 3);
	QCOMPARE(rm.getPositionForValue(1000000.0), 7);
	QCOMPARE(rm.getPositionForValue(1000.0), 5);
	QCOMPARE(rm.getPositionForValue(900.0), 5);
	QCOMPARE(rm.getPositionForValue(20000), 6);
    }

    void logDownForward()
    {
	LogRangeMapper rm(3, 7, 10, 100000, "x", true);
	QCOMPARE(rm.getUnit(), QString("x"));
	QCOMPARE(rm.getPositionForValue(10.0), 7);
	QCOMPARE(rm.getPositionForValue(100000.0), 3);
	QCOMPARE(rm.getPositionForValue(1.0), 7);
	QCOMPARE(rm.getPositionForValue(1000000.0), 3);
	QCOMPARE(rm.getPositionForValue(1000.0), 5);
	QCOMPARE(rm.getPositionForValue(900.0), 5);
	QCOMPARE(rm.getPositionForValue(20000), 4);
    }

    void logUpBackward()
    {
	LogRangeMapper rm(3, 7, 10, 100000, "x", false);
	QCOMPARE(rm.getUnit(), QString("x"));
	QCOMPARE(rm.getValueForPosition(3), 10.0);
	QCOMPARE(rm.getValueForPosition(7), 100000.0);
	QCOMPARE(rm.getValueForPosition(5), 1000.0);
	QCOMPARE(rm.getValueForPosition(6), 10000.0);
	QCOMPARE(rm.getValueForPosition(0), rm.getValueForPosition(3));
	QCOMPARE(rm.getValueForPosition(9), rm.getValueForPosition(7));
    }

    void logDownBackward()
    {
	LogRangeMapper rm(3, 7, 10, 100000, "x", true);
	QCOMPARE(rm.getUnit(), QString("x"));
	QCOMPARE(rm.getValueForPosition(7), 10.0);
	QCOMPARE(rm.getValueForPosition(3), 100000.0);
	QCOMPARE(rm.getValueForPosition(5), 1000.0);
	QCOMPARE(rm.getValueForPosition(4), 10000.0);
	QCOMPARE(rm.getValueForPosition(0), rm.getValueForPosition(3));
	QCOMPARE(rm.getValueForPosition(9), rm.getValueForPosition(7));
    }
};

#endif


