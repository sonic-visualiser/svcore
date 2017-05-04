/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2017 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_SCALE_TICK_INTERVALS_H
#define SV_SCALE_TICK_INTERVALS_H

#include <string>
#include <vector>
#include <cmath>

//#define DEBUG_SCALE_TICK_INTERVALS 1

#ifdef DEBUG_SCALE_TICK_INTERVALS
#include <iostream>
#endif

class ScaleTickIntervals
{
public:
    struct Range {
	double min;        // start of value range
	double max;        // end of value range
	int n;             // number of divisions (approximate only)
    };

    struct Tick {
	double value;      // value this tick represents
	std::string label; // value as written 
    };

    struct Ticks {
	double initial;    // value of first tick
	double spacing;    // increment between ticks
	double roundTo;    // what all displayed values should be rounded to
	bool fixed;        // whether to use fixed precision (%f rather than %e)
	int precision;     // number of dp (%f) or sf (%e)
	std::vector<Tick> ticks;  // computed tick values and labels
    };
    
    static Ticks linear(Range r) {

	if (r.n < 1) {
	    return {};
	}
	if (r.max < r.min) {
	    return linear({ r.max, r.min, r.n });
	}
	
	double inc = (r.max - r.min) / r.n;
	if (inc == 0) {
#ifdef DEBUG_SCALE_TICK_INTERVALS
            std::cerr << "inc == 0, using trivial range" << std::endl;
#endif
            double roundTo = r.min;
            if (roundTo <= 0.0) {
                roundTo = 1.0;
            }
	    Ticks t { r.min, 1.0, roundTo, true, 1, {} };
	    explode(r, t);
	    return t;
	}

        double digInc = log10(inc);
        double digMax = log10(fabs(r.max));
        double digMin = log10(fabs(r.min));

        int precInc = int(trunc(digInc));
        if (double(precInc) != digInc) {
            precInc -= 1;
        }

        bool fixed = false;
        if (precInc > -4 && precInc < 4) {
            fixed = true;
        } else if ((digMax >= -3.0 && digMax <= 2.0) &&
                   (digMin >= -3.0 && digMin <= 3.0)) {
            fixed = true;
        }
        
        int precRange = int(ceil(digMax - digInc));

        int prec = 1;
        
        if (fixed) {
            if (precInc < 0) {
                prec = -precInc;
            } else if (precInc > 0) {
                prec = 0;
            }
        } else {
            prec = precRange;
        }

	double roundTo = pow(10.0, precInc);

#ifdef DEBUG_SCALE_TICK_INTERVALS
        std::cerr << "\nmin = " << r.min << ", max = " << r.max << ", n = " << r.n
                  << ", inc = " << inc << std::endl;
        std::cerr << "digMax = " << digMax << ", digInc = " << digInc
                  << std::endl;
        std::cerr << "fixed = " << fixed << ", inc = " << inc
                  << ", precInc = " << precInc << ", precRange = " << precRange
                  << ", prec = " << prec << std::endl;
        std::cerr << "roundTo = " << roundTo << std::endl;
#endif
        
	inc = round(inc / roundTo) * roundTo;
        if (inc < roundTo) inc = roundTo;
        
	double min = ceil(r.min / roundTo) * roundTo;
	if (min > r.max) min = r.max;

        if (!fixed && min != 0.0) {
            double digNewMin = log10(fabs(min));
            if (digNewMin < digInc) {
                prec = int(ceil(digMax - digNewMin));
#ifdef DEBUG_SCALE_TICK_INTERVALS
                std::cerr << "min is smaller than increment, adjusting prec to "
                          << prec << std::endl;
#endif
            }
        }
        
	Ticks t { min, inc, roundTo, fixed, prec, {} };
	explode(r, t);
	return t;
    }

private:
    static void explode(const Range &r, Ticks &t) {
#ifdef DEBUG_SCALE_TICK_INTERVALS
	std::cerr << "initial = " << t.initial << ", spacing = " << t.spacing
		  << ", roundTo = " << t.roundTo << ", fixed = " << t.fixed
		  << ", precision = " << t.precision << std::endl;
#endif
	auto makeTick = [&](double value) {
	    const int buflen = 40;
	    char buffer[buflen];
	    snprintf(buffer, buflen,
		     t.fixed ? "%.*f" : "%.*e",
		     t.precision, value);
	    return Tick({ value, std::string(buffer) });
	};
        double eps = 1e-7;
        if (t.spacing < eps * 10.0) {
            eps = t.spacing / 10.0;
        }
        int n = 0;
        while (true) {
            double value = t.initial + n * t.spacing;
	    value = t.roundTo * round(value / t.roundTo);
            if (value >= r.max + eps) {
                break;
            }
	    t.ticks.push_back(makeTick(value));
            ++n;
	}
    }
};

#endif
