/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2016 Chris Cannam and QMUL.
    
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

/**
 * Display normalization types for columns in e.g. grid plots.
 *
 * Max1 means to normalize to max value = 1.0.
 * Sum1 means to normalize to sum of values = 1.0.
 *
 * Hybrid means normalize to max = 1.0 and then multiply by
 * log10 of the max value, to retain some difference between
 * levels of neighbouring columns.
 *
 * Area normalization is handled separately.
 */
enum class ColumnNormalization {
    None,
    Max1,
    Sum1,
    Hybrid
};

/**
 * Class containing static functions for simple operations on data
 * columns, for use by display layers.
 */
class ColumnOp
{
public:
    /** 
     * Column type. 
     */
    typedef std::vector<float> Column;

    /**
     * Scale the given column using the given gain multiplier.
     */
    static Column applyGain(const Column &in, double gain) {
	
	if (gain == 1.0) {
	    return in;
	}
	Column out;
	out.reserve(in.size());
	for (auto v: in) {
	    out.push_back(float(v * gain));
	}
	return out;
    }

    /**
     * Scale an FFT output by half the FFT size.
     */
    static Column fftScale(const Column &in, int fftSize) {
        return applyGain(in, 2.0 / fftSize);
    }

    /**
     * Determine whether an index points to a local peak.
     */
    static bool isPeak(const Column &in, int ix) {
	
	if (!in_range_for(in, ix-1)) return false;
	if (!in_range_for(in, ix+1)) return false;
	if (in[ix] < in[ix+1]) return false;
	if (in[ix] < in[ix-1]) return false;
	
	return true;
    }

    /**
     * Return a column containing only the local peak values (all
     * others zero).
     */
    static Column peakPick(const Column &in) {
	
	std::vector<float> out(in.size(), 0.f);
	for (int i = 0; in_range_for(in, i); ++i) {
	    if (isPeak(in, i)) {
		out[i] = in[i];
	    }
	}
	
	return out;
    }

    /**
     * Return a column normalized from the input column according to
     * the given normalization scheme.
     */
    static Column normalize(const Column &in, ColumnNormalization n) {

	if (n == ColumnNormalization::None) {
	    return in;
	}

        float scale = 1.f;
        
        if (n == ColumnNormalization::Sum1) {

            float sum = 0.f;

            for (auto v: in) {
                sum += v;
            }

            if (sum != 0.f) {
                scale = 1.f / sum;
            }
        } else {
        
            float max = *max_element(in.begin(), in.end());

            if (n == ColumnNormalization::Max1) {
                if (max != 0.f) {
                    scale = 1.f / max;
                }
            } else if (n == ColumnNormalization::Hybrid) {
                if (max > 0.f) {
                    scale = log10f(max + 1.f) / max;
                }
            }
        }

        return applyGain(in, scale);
    }

    /**
     * Distribute the given column into a target vector of a different
     * size, optionally using linear interpolation. The binfory vector
     * contains a mapping from y coordinate (i.e. index into the
     * target vector) to bin (i.e. index into the source column).
     */
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
                if (by1 >= bins) by1 = bins - 1;
                
		for (int bin = by0; bin <= by1; ++bin) {

		    float value = in[bin];

		    if (bin == by0 || value > out[y]) {
			out[y] = value;
		    }
		}
	    }
	}

	return out;
    }

};

#endif

