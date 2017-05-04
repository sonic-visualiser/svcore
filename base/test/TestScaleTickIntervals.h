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
	for (int i = 0; i < int(ticks.size()); ++i) {
            cerr << i << ": have " << ticks[i].value << " \""
                 << ticks[i].label << "\", expected ";
	    if (i < int(expected.size())) {
                cerr << expected[i].value << " \"" << expected[i].label
		     << "\"" << endl;
            } else {
                cerr << "(n/a)" << endl;
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
	    { 0.0, "0.0" },
	    { 0.1, "0.1" },
	    { 0.2, "0.2" },
	    { 0.3, "0.3" },
	    { 0.4, "0.4" },
	    { 0.5, "0.5" },
	    { 0.6, "0.6" },
	    { 0.7, "0.7" },
	    { 0.8, "0.8" },
	    { 0.9, "0.9" },
	    { 1.0, "1.0" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_0_5_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 5, 5 });
        // generally if we have some activity in the units column, we
        // should add .0 to satisfy the human worry that we aren't
        // being told the whole story...
	vector<ScaleTickIntervals::Tick> expected {
	    { 0, "0.0" },
	    { 1, "1.0" },
	    { 2, "2.0" },
	    { 3, "3.0" },
	    { 4, "4.0" },
	    { 5, "5.0" },
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_0_10_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 10, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0, "0.0" },
	    { 2, "2.0" },
	    { 4, "4.0" },
	    { 6, "6.0" },
	    { 8, "8.0" },
	    { 10, "10.0" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_10_0_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 10, 0, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0, "0.0" },
	    { 2, "2.0" },
	    { 4, "4.0" },
	    { 6, "6.0" },
	    { 8, "8.0" },
	    { 10, "10.0" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_m10_0_5()
    {
	auto ticks = ScaleTickIntervals::linear({ -10, 0, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { -10, "-10.0" },
	    { -8, "-8.0" },
	    { -6, "-6.0" },
	    { -4, "-4.0" },
	    { -2, "-2.0" },
	    { 0, "0.0" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_0_m10_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, -10, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { -10, "-10.0" },
	    { -8, "-8.0" },
	    { -6, "-6.0" },
	    { -4, "-4.0" },
	    { -2, "-2.0" },
	    { 0, "0.0" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_0_0p1_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 0.1, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.00, "0.00" },
	    { 0.02, "0.02" },
	    { 0.04, "0.04" },
	    { 0.06, "0.06" },
	    { 0.08, "0.08" },
	    { 0.10, "0.10" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_0_0p01_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 0.01, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.000, "0.000" },
	    { 0.002, "0.002" },
	    { 0.004, "0.004" },
	    { 0.006, "0.006" },
	    { 0.008, "0.008" },
	    { 0.010, "0.010" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_0_0p005_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 0.005, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.000, "0.000" },
	    { 0.001, "0.001" },
	    { 0.002, "0.002" },
	    { 0.003, "0.003" },
	    { 0.004, "0.004" },
	    { 0.005, "0.005" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_0_0p001_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 0.001, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.0000, "0.0e+00" },
	    { 0.0002, "2.0e-04" },
	    { 0.0004, "4.0e-04" },
	    { 0.0006, "6.0e-04" },
	    { 0.0008, "8.0e-04" },
	    { 0.0010, "1.0e-03" }
	};
	compareTicks(ticks.ticks, expected);
    }
    
    void linear_1_1p001_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 1, 1.001, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 1.0000, "1.0000" },
	    { 1.0002, "1.0002" },
	    { 1.0004, "1.0004" },
	    { 1.0006, "1.0006" },
	    { 1.0008, "1.0008" },
	    { 1.0010, "1.0010" }
	};
	compareTicks(ticks.ticks, expected);
    }
    
    void linear_0p001_1_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 0.001, 1, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.1, "0.1" },
	    { 0.3, "0.3" },
	    { 0.5, "0.5" },
	    { 0.7, "0.7" },
	    { 0.9, "0.9" },
	};
	compareTicks(ticks.ticks, expected);
    }
        
    void linear_10000_10010_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 10000, 10010, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 10000, "10000.0" },
	    { 10002, "10002.0" },
	    { 10004, "10004.0" },
	    { 10006, "10006.0" },
	    { 10008, "10008.0" },
	    { 10010, "10010.0" },
	};
	compareTicks(ticks.ticks, expected);
    }
    
    void linear_10000_20000_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 10000, 20000, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 10000, "10000" },
	    { 12000, "12000" },
	    { 14000, "14000" },
	    { 16000, "16000" },
	    { 18000, "18000" },
	    { 20000, "20000" },
	};
	compareTicks(ticks.ticks, expected);
    }
    
    void linear_m1_1_10()
    {
	auto ticks = ScaleTickIntervals::linear({ -1, 1, 10 });
	vector<ScaleTickIntervals::Tick> expected {
	    { -1.0, "-1.0" },
	    { -0.8, "-0.8" },
	    { -0.6, "-0.6" },
	    { -0.4, "-0.4" },
	    { -0.2, "-0.2" },
	    { 0.0, "0.0" },
	    { 0.2, "0.2" },
	    { 0.4, "0.4" },
	    { 0.6, "0.6" },
	    { 0.8, "0.8" },
	    { 1.0, "1.0" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_221p23_623p7_57p4()
    {
	auto ticks = ScaleTickIntervals::linear({ 221.23, 623.7, 4 });
        // only 4 ticks, not 5, because none of the rounded tick
        // values lies on an end value
	vector<ScaleTickIntervals::Tick> expected {
            { 230, "230" },
            { 330, "330" },
            { 430, "430" },
            { 530, "530" },
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_sqrt2_pi_7()
    {
	auto ticks = ScaleTickIntervals::linear({ sqrt(2.0), M_PI, 7 });
        // This would be better in steps of 0.25, but we only round to
        // integral powers of ten
	vector<ScaleTickIntervals::Tick> expected {
            { 1.5, "1.5" },
            { 1.7, "1.7" },
            { 1.9, "1.9" },
            { 2.1, "2.1" },
            { 2.3, "2.3" },
            { 2.5, "2.5" },
            { 2.7, "2.7" },
            { 2.9, "2.9" },
            { 3.1, "3.1" },
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_pi_avogadro_7()
    {
	auto ticks = ScaleTickIntervals::linear({ M_PI, 6.022140857e23, 7 });
	vector<ScaleTickIntervals::Tick> expected {
            { 1e+21, "1.000e+21" },
            { 8.7e+22, "8.700e+22" },
            { 1.73e+23, "1.730e+23" },
            { 2.59e+23, "2.590e+23" },
            { 3.45e+23, "3.450e+23" },
            { 4.31e+23, "4.310e+23" },
            { 5.17e+23, "5.170e+23" },
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_2_3_1()
    {
	auto ticks = ScaleTickIntervals::linear({ 2, 3, 1 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 2.0, "2.0" },
	    { 3.0, "3.0" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_2_3_2()
    {
	auto ticks = ScaleTickIntervals::linear({ 2, 3, 2 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 2.0, "2.0" },
	    { 2.5, "2.5" },
	    { 3.0, "3.0" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_2_3_3()
    {
	auto ticks = ScaleTickIntervals::linear({ 2, 3, 3 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 2.0, "2.0" },
	    { 2.3, "2.3" },
	    { 2.6, "2.6" },
	    { 2.9, "2.9" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_2_3_4()
    {
	auto ticks = ScaleTickIntervals::linear({ 2, 3, 4 });
        // This would be better in steps of 0.25, but we only round to
        // integral powers of ten
	vector<ScaleTickIntervals::Tick> expected {
	    { 2.0, "2.0" },
	    { 2.3, "2.3" },
	    { 2.6, "2.6" },
	    { 2.9, "2.9" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_2_3_5()
    {
	auto ticks = ScaleTickIntervals::linear({ 2, 3, 5 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 2.0, "2.0" },
	    { 2.2, "2.2" },
	    { 2.4, "2.4" },
	    { 2.6, "2.6" },
	    { 2.8, "2.8" },
	    { 3.0, "3.0" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_2_3_6()
    {
	auto ticks = ScaleTickIntervals::linear({ 2, 3, 6 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 2.0, "2.0" },
	    { 2.2, "2.2" },
	    { 2.4, "2.4" },
	    { 2.6, "2.6" },
	    { 2.8, "2.8" },
	    { 3.0, "3.0" }
	};
	compareTicks(ticks.ticks, expected);
    }

    void linear_1_1_10()
    {
	auto ticks = ScaleTickIntervals::linear({ 1, 1, 10 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 1.0, "1.0" }
	};
	compareTicks(ticks.ticks, expected);
    }
    
    void linear_0_0_10()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 0, 10 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.0, "0.0" }
	};
	compareTicks(ticks.ticks, expected);
    }
    
    void linear_0_1_1()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 1, 1 });
	vector<ScaleTickIntervals::Tick> expected {
	    { 0.0, "0.0" },
	    { 1.0, "1.0" }
	};
	compareTicks(ticks.ticks, expected);
    }
    
    void linear_0_1_0()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 1, 0 });
	vector<ScaleTickIntervals::Tick> expected {
	};
	compareTicks(ticks.ticks, expected);
    }
    
    void linear_0_1_m1()
    {
	auto ticks = ScaleTickIntervals::linear({ 0, 1, -1 });
	vector<ScaleTickIntervals::Tick> expected {
	};
	compareTicks(ticks.ticks, expected);
    }
};

#endif


