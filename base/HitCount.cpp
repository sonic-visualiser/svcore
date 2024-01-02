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

#include "HitCount.h"

#ifndef NO_HIT_COUNTS

#include "Debug.h"

HitCount::~HitCount()
{
    int total = m_hit + m_partial + m_miss;
    SVDEBUG << "Hit count: " << m_name << ": ";
    if (m_partial > 0) {
        SVDEBUG << m_hit << " hits, " << m_partial << " partial, "
                << m_miss << " misses";
    } else {
        SVDEBUG << m_hit << " hits, " << m_miss << " misses";
    }
    if (total > 0) {
        if (m_partial > 0) {
            SVDEBUG << " (" << ((m_hit * 100.0) / total) << "%, "
                    << ((m_partial * 100.0) / total) << "%, "
                    << ((m_miss * 100.0) / total) << "%)";
        } else {
            SVDEBUG << " (" << ((m_hit * 100.0) / total) << "%, "
                    << ((m_miss * 100.0) / total) << "%)";
        }
    }
    SVDEBUG << endl;
}

#endif
