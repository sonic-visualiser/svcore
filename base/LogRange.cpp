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

#include "LogRange.h"

#include <algorithm>
#include <cmath>

void
LogRange::mapRange(float &min, float &max, float logthresh)
{
    if (min > max) std::swap(min, max);
    if (max == min) max = min + 1;

    if (min >= 0.f) {

        max = log10f(max); // we know max != 0

        if (min == 0.f) min = std::min(logthresh, max);
        else min = log10f(min);

    } else if (max <= 0.f) {
        
        min = log10f(-min); // we know min != 0
        
        if (max == 0.f) max = std::min(logthresh, min);
        else max = log10f(-max);
        
        std::swap(min, max);
        
    } else {
        
        // min < 0 and max > 0
        
        max = log10f(std::max(max, -min));
        min = std::min(logthresh, max);
    }

    if (min == max) min = max - 1;
}        

float
LogRange::map(float value, float thresh)
{
    if (value == 0.f) return thresh;
    return log10f(fabsf(value));
}

float
LogRange::unmap(float value)
{
    return powf(10.0, value);
}
