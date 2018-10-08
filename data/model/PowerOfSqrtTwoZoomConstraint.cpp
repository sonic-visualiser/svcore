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

#include "PowerOfSqrtTwoZoomConstraint.h"

#include <iostream>
#include <cmath>

#include "base/Debug.h"


ZoomLevel
PowerOfSqrtTwoZoomConstraint::getNearestZoomLevel(ZoomLevel requested,
                                                  RoundingDirection dir) const
{
    int type, power;
    double blockSize;

    if (requested.zone == ZoomLevel::FramesPerPixel) {
        blockSize = getNearestBlockSize(requested.level, type, power, dir);
        return { requested.zone, blockSize };
    } else {
        RoundingDirection opposite = dir;
        if (dir == RoundUp) opposite = RoundDown;
        else if (dir == RoundDown) opposite = RoundUp;
        blockSize = getNearestBlockSize(requested.level, type, power, opposite);
        if (blockSize > getMinZoomLevel().level) {
            blockSize = getMinZoomLevel().level;
        }
        if (blockSize == 1) {
            return { ZoomLevel::FramesPerPixel, 1 };
        } else {
            return { requested.zone, blockSize };
        }
    }
}

double
PowerOfSqrtTwoZoomConstraint::getNearestBlockSize(double blockSize,
                                                  int &type, 
                                                  int &power,
                                                  RoundingDirection dir) const
{
//    SVCERR << "given " << blockSize << endl;

    int minCachePower = getMinCachePower();

    double eps = 1e-8;
    
    if (blockSize < (1 << minCachePower)) {
        type = -1;
        power = 0;
        double val = 1.0, prevVal = 1.0;
        while (val + eps < blockSize) {
            prevVal = val;
            val *= sqrtf(2.f);
        }
        double rval = val;
//        SVCERR << "got val = " << val << ", rval = " << rval << ", prevVal = " << prevVal << endl;
        if (rval != blockSize && dir != RoundUp) {
            if (dir == RoundDown) {
                rval = prevVal;
            } else if (val - blockSize < blockSize - prevVal) {
                rval = val;
            } else {
                rval = prevVal;
            }
        }
//        SVCERR << "returning " << rval << endl;
        return rval;
    }

    double prevBase = (1 << minCachePower);
    int prevPower = minCachePower;
    int prevType = 0;

    double result = 0;

    for (unsigned int i = 0; ; ++i) {

        power = minCachePower + i/2;
        type = i % 2;

        double base;
        if (type == 0) {
            base = pow(2.0, power);
        } else {
            base = sqrt(2.0) * pow(2.0, power);
        }

//        SVCERR << "Testing base " << base << " (i = " << i << ", power = " << power << ", type = " << type << ")" << endl;

        if (base == blockSize) {
            result = base;
            break;
        }

        if (base > blockSize) {
            if (dir == RoundNearest) {
                if (base - blockSize < blockSize - prevBase) {
                    dir = RoundUp;
                } else {
                    dir = RoundDown;
                }
            }
            if (dir == RoundUp) {
                result = base;
                break;
            } else {
                type = prevType;
                power = prevPower;
                result = prevBase;
                break;
            }
        }

        prevType = type;
        prevPower = power;
        prevBase = base;
    }

    if (result > getMaxZoomLevel().level) {
        result = getMaxZoomLevel().level;
    }

    return result;
}   
