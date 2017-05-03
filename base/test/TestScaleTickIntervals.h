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

#ifndef TEST_SCALE_TICK_INTERVALS_H
#define TEST_SCALE_TICK_INTERVALS_H

#include "../ScaleTickIntervals.h"

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>

using namespace std;

class TestScaleTickIntervals : public QObject
{
    Q_OBJECT

    void printDiff(vector<ScaleTickIntervals::Tick> ticks,
		   vector<ScaleTickIntervals::Tick> expected) {

	cerr << "Have " << ticks.size() << " ticks, expected "
	     << expected.size() << endl;
	for (int i = 0; i < int(expected.size()); ++i) {
	    if (i < int(ticks.size())) {
		cerr << i << ": have " << ticks[i].value << " \""
		     << ticks[i].label << "\", expected "
		     << expected[i].value << " \"" << expected[i].label
		     << "\"" << endl;
	    }
	}
    }
    
    void compareTicks(vector<ScaleTickIntervals::Tick> ticks,
		      vector<ScaleTickIntervals::Tick> expected)
    {
        double eps = 1e-7;
	for (int i = 0; i < int(expected.size()); ++i) {
	    if (i < int(ticks.size())) {
		if (ticks[i].label != expected[i].label ||
		    fabs(ticks[i].value - expected[i].value) > eps) {
		    printDiff(ticks, expected);
		}
		QCOMPARE(ticks[i].label, expected[i].label);
		QCOMPARE(ticks[i].value, expected[i].value);
	    }
	}
        if (ticks.size() != expected.size()) {
            printDiff(ticks, expected);
        }
	QCOMPARE(ticks.size(), expected.size());
    }
    
private slots:
    void linear_0_1_10()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 1, 10 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.0, "0.00" },
	    { 0.1, "0.10" },
	    { 0.2, "0.20" },
	    { 0.3, "0.30" },
	    { 0.4, "0.40" },
	    { 0.5, "0.50" },
	    { 0.6, "0.60" },
	    { 0.7, "0.70" },
	    { 0.8, "0.80" },
	    { 0.9, "0.90" },
	    { 1.0, "1.00" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_0_0p1_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 0.1, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.0, "0.00" },
	    { 0.02, "0.02" },
	    { 0.04, "0.04" },
	    { 0.06, "0.06" },
	    { 0.08, "0.08" },
	    { 0.1, "0.10" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_0_0p01_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 0.01, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.00, "0.000" },
	    { 0.002, "0.002" },
	    { 0.004, "0.004" },
	    { 0.006, "0.006" },
	    { 0.008, "0.008" },
	    { 0.01, "0.010" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_0_0p001_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 0.001, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.000, "0.0e+00" },
	    { 0.0002, "2.0e-04" },
	    { 0.0004, "4.0e-04" },
	    { 0.0006, "6.0e-04" },
	    { 0.0008, "8.0e-04" },
	    { 0.001, "1.0e-03" }
	};
	compareTicks(ticks.ticks, expected);
    }
    
    void linear_1_1p001_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 1, 1.001, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 1.000, "1.0000" },
	    { 1.0002, "1.0002" },
	    { 1.0004, "1.0004" },
	    { 1.0006, "1.0006" },
	    { 1.0008, "1.0008" },
	    { 1.001, "1.0010" }
	};
	compareTicks(ticks.ticks, expected);
    }
};

#endif


