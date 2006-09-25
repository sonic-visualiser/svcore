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

#ifndef _STORAGE_ADVISER_H_
#define _STORAGE_ADVISER_H_

/**
 * A utility class designed to help decide whether to store cache data
 * (for example FFT outputs) in memory or on disk in the TempDirectory.
 */

class StorageAdviser
{
public:
    // pass to recommend() zero or more of these OR'd together
    enum Criteria {
        SpeedCritical       = 1,
        PrecisionCritical   = 2,
        RepeatabilityUseful = 4
    };

    // recommend() returns one or two of these OR'd together
    enum Recommendation {
        UseMemory          = 1,
        UseDisc            = 2,
        ConserveSpace      = 4,
        UseAsMuchAsYouLike = 8
    };

    // May throw InsufficientDiscSpace exception if it looks like
    // minimumSize won't fit on the disc.  

    /**
     * Recommend where to store some data, given certain storage and
     * recall criteria.  The minimum size is the approximate amount of
     * data in bytes that will be stored if the recommendation is to
     * ConserveSpace; the maximum size is approximately the amount
     * that will be used if UseAsMuchAsYouLike is returned.
     **!!! sizes should be longer types
     */
    static Recommendation recommend(Criteria criteria,
                                    int minimumSize,
                                    int maximumSize);
};

#endif

