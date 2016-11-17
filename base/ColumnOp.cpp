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

#include "ColumnOp.h"

#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;

ColumnOp::Column
ColumnOp::fftScale(const Column &in, int fftSize)
{
    return applyGain(in, 2.0 / fftSize);
}

ColumnOp::Column
ColumnOp::peakPick(const Column &in)
{
    vector<float> out(in.size(), 0.f);

    for (int i = 0; in_range_for(in, i); ++i) {
        if (isPeak(in, i)) {
            out[i] = in[i];
        }
    }
    
    return out;
}

ColumnOp::Column
ColumnOp::normalize(const Column &in, ColumnNormalization n) {

    if (n == ColumnNormalization::None || in.empty()) {
        return in;
    }

    float scale = 1.f;
        
    if (n == ColumnNormalization::Sum1) {

        float sum = 0.f;

        for (auto v: in) {
            sum += fabsf(v);
        }

        if (sum != 0.f) {
            scale = 1.f / sum;
        }

    } else {

        float max = 0.f;

        for (auto v: in) {
            v = fabsf(v);
            if (v > max) {
                max = v;
            }
        }

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

ColumnOp::Column
ColumnOp::distribute(const Column &in,
                     int h,
                     const vector<double> &binfory,
                     int minbin,
                     bool interpolate)
{
    vector<float> out(h, 0.f);
    int bins = int(in.size());

    for (int y = 0; y < h; ++y) {

        if (interpolate && h > bins) {

            double sy = binfory[y] - minbin - 0.5;
            double syf = floor(sy);

            int mainbin = int(syf);
            int other = mainbin;
            if (sy > syf) {
                other = mainbin + 1;
            } else if (sy < syf) {
                other = mainbin - 1;
            }

            if (mainbin < 0) {
                mainbin = 0;
            }
            if (mainbin >= bins) {
                mainbin = bins - 1;
            }

            if (other < 0) {
                other = 0;
            }
            if (other >= bins) {
                other = bins - 1;
            }

            double prop = 1.0 - fabs(sy - syf);
            
            double v0 = in[mainbin];
            double v1 = in[other];
                
            out[y] = float(prop * v0 + (1.0 - prop) * v1);

        } else {
            
            double sy0 = binfory[y] - minbin;

            double sy1;
            if (y+1 < h) {
                sy1 = binfory[y+1] - minbin;
            } else {
                sy1 = bins;
            }

            int by0 = int(sy0 + 0.0001);
            int by1 = int(sy1 + 0.0001);
                
            for (int bin = by0; bin == by0 || bin < by1; ++bin) {

                float value = in[bin];

                if (bin == by0 || value > out[y]) {
                    out[y] = value;
                }
            }
        }
    }

    return out;
}
