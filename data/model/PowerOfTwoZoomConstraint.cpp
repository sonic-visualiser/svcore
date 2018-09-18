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

#include "PowerOfTwoZoomConstraint.h"

int
PowerOfTwoZoomConstraint::getNearestBlockSize(int req,
                                              RoundingDirection dir) const
{
    int max = getMaxZoomLevel();

    if (req > max) {
        return max;
    }

    for (int bs = 1; bs <= max; bs *= 2) {
        if (bs < req) {
            continue;
        } else if (bs == req) {
            return bs;
        } else { // bs > req
            if (dir == RoundNearest) {
                if (bs - req < req - bs/2) {
                    return bs;
                } else {
                    return bs/2;
                }
            } else if (dir == RoundDown) {
                return bs/2;
            } else {
                return bs;
            }
        }
    }

    return max;
}

