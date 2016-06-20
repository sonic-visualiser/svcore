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

#ifndef COLUMN_OP_H
#define COLUMN_OP_H

#include "BaseTypes.h"

#include <cmath>

class ColumnOp
{
public:
    typedef std::vector<float> Column;

    enum Normalization {
        NoNormalization,
        NormalizeColumns,
        NormalizeVisibleArea,
        NormalizeHybrid
    };

    static Column fftScale(const Column &in, int fftSize) {
	
	Column out;
	out.reserve(in.size());
	float scale = 2.f / float(fftSize);
	for (auto v: in) {
	    out.push_back(v * scale);
	}
	
	return out;
    }

    static bool isPeak(const Column &in, int ix) {
	
	if (!in_range_for(in, ix-1)) return false;
	if (!in_range_for(in, ix+1)) return false;
	if (in[ix] < in[ix+1]) return false;
	if (in[ix] < in[ix-1]) return false;
	
	return true;
    }

    static Column peakPick(const Column &in) {
	
	std::vector<float> out(in.size(), 0.f);
	for (int i = 0; in_range_for(in, i); ++i) {
	    if (isPeak(in, i)) {
		out[i] = in[i];
	    }
	}
	
	return out;
    }

    static Column normalize(const Column &in, Normalization n) {

	if (n == NoNormalization || n == NormalizeVisibleArea) {
	    return in;
	}
	
	float max = *max_element(in.begin(), in.end());

	if (n == NormalizeColumns && max == 0.f) {
	    return in;
	}

	if (n == NormalizeHybrid && max <= 0.f) {
	    return in;
	}
    
	std::vector<float> out;
	out.reserve(in.size());

	float scale;
	if (n == NormalizeHybrid) {
	    scale = log10f(max + 1.f) / max;
	} else {
	    scale = 1.f / max;
	}
    
	for (auto v: in) {
	    out.push_back(v * scale);
	}
	return out;
    }

    static Column applyGain(const Column &in, float gain) {
	
	if (gain == 1.f) {
	    return in;
	}
	Column out;
	out.reserve(in.size());
	for (auto v: in) {
	    out.push_back(v * gain);
	}
	return out;
    }

    static Column distribute(const Column &in,
			     int h,
			     const std::vector<double> &binfory,
			     int minbin,
			     bool interpolate) {
	
	std::vector<float> out(h, 0.f);
	int bins = int(in.size());

	for (int y = 0; y < h; ++y) {
        
	    double sy0 = binfory[y] - minbin;
	    double sy1 = sy0 + 1;
	    if (y+1 < h) {
		sy1 = binfory[y+1] - minbin;
	    }
        
	    if (interpolate && fabs(sy1 - sy0) < 1.0) {
            
		double centre = (sy0 + sy1) / 2;
		double dist = (centre - 0.5) - rint(centre - 0.5);
		int bin = int(centre);

		int other = (dist < 0 ? (bin-1) : (bin+1));

		if (bin < 0) bin = 0;
		if (bin >= bins) bin = bins-1;

		if (other < 0 || other >= bins) {
		    other = bin;
		}

		double prop = 1.0 - fabs(dist);

		double v0 = in[bin];
		double v1 = in[other];
                
		out[y] = float(prop * v0 + (1.0 - prop) * v1);

	    } else { // not interpolating this one

		int by0 = int(sy0 + 0.0001);
		int by1 = int(sy1 + 0.0001);
		if (by1 < by0 + 1) by1 = by0 + 1;

		for (int bin = by0; bin < by1; ++bin) {

		    float value = in[bin];

		    if (value > out[y]) {
			out[y] = value;
		    }
		}
	    }
	}

	return out;
    }

};

#endif

