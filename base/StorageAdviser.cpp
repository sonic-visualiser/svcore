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

#include "StorageAdviser.h"

#include "Exceptions.h"
#include "TempDirectory.h"

#include "system/System.h"

#include <iostream>

StorageAdviser::Recommendation
StorageAdviser::recommend(Criteria criteria,
			  int minimumSize,
			  int maximumSize)
{
    std::cerr << "StorageAdviser::recommend: Criteria " << criteria 
              << ", minimumSize " << minimumSize
              << ", maximumSize " << maximumSize << std::endl;

    QString path = TempDirectory::getInstance()->getPath();
    int discFree = GetDiscSpaceMBAvailable(path.toLocal8Bit());
    int memoryFree, memoryTotal;
    GetRealMemoryMBAvailable(memoryFree, memoryTotal);

    std::cerr << "Disc space: " << discFree << ", memory free: " << memoryFree << ", memory total: " << memoryTotal << std::endl;

    enum StorageStatus {
        Unknown,
        Insufficient,
        Marginal,
        Sufficient
    };

    StorageStatus memoryStatus = Unknown;
    StorageStatus discStatus = Unknown;

    int minmb = minimumSize / 1024 + 1;
    int maxmb = maximumSize / 1024 + 1;

    if (memoryFree == -1) memoryStatus = Unknown;
    else if (minmb > (memoryFree * 3) / 4) memoryStatus = Insufficient;
    else if (maxmb > (memoryFree * 3) / 4) memoryStatus = Marginal;
    else if (minmb > (memoryFree / 3)) memoryStatus = Marginal;
    else if (memoryTotal == -1 ||
             minmb > (memoryTotal / 10)) memoryStatus = Marginal;
    else memoryStatus = Sufficient;

    if (discFree == -1) discStatus = Unknown;
    else if (minmb > (discFree * 3) / 4) discStatus = Insufficient;
    else if (maxmb > (discFree * 3) / 4) discStatus = Marginal;
    else if (minmb > (discFree / 3)) discStatus = Marginal;
    else discStatus = Sufficient;

    std::cerr << "Memory status: " << memoryStatus << ", disc status "
              << discStatus << std::endl;

    int recommendation = NoRecommendation;

    if (memoryStatus == Insufficient || memoryStatus == Unknown) {

        recommendation |= UseDisc;

        if (discStatus == Insufficient && minmb > discFree) {
            throw InsufficientDiscSpace(path, minmb, discFree);
        }

        if (discStatus == Insufficient || discStatus == Marginal) {
            recommendation |= ConserveSpace;
        } else if (discStatus == Unknown && !(criteria & PrecisionCritical)) {
            recommendation |= ConserveSpace;
        } else {
            recommendation |= UseAsMuchAsYouLike;
        }

    } else if (memoryStatus == Marginal) {

        if (((criteria & SpeedCritical) ||
             (criteria & FrequentLookupLikely)) &&
            !(criteria & PrecisionCritical) &&
            !(criteria & LongRetentionLikely)) {

            // requirements suggest a preference for memory

            if (discStatus != Insufficient) {
                recommendation |= PreferMemory;
            } else {
                recommendation |= UseMemory;
            }

            recommendation |= ConserveSpace;

        } else {

            if (discStatus == Insufficient) {
                recommendation |= (UseMemory | ConserveSpace);
            } else if (discStatus == Marginal) {
                recommendation |= (PreferMemory | ConserveSpace);
            } else if (discStatus == Unknown) {
                recommendation |= (PreferDisc | ConserveSpace);
            } else {
                recommendation |= (UseDisc | UseAsMuchAsYouLike);
            }
        }    

    } else {

        if (discStatus == Insufficient) {
            recommendation |= (UseMemory | ConserveSpace);
        } else if (discStatus != Sufficient) {
            recommendation |= (PreferMemory | ConserveSpace);
        } else {

            if ((criteria & SpeedCritical) ||
                (criteria & FrequentLookupLikely)) {
                recommendation |= PreferMemory;
                if (criteria & PrecisionCritical) {
                    recommendation |= UseAsMuchAsYouLike;
                } else {
                    recommendation |= ConserveSpace;
                }
            } else {
                recommendation |= PreferDisc;
                recommendation |= UseAsMuchAsYouLike;
            }
        }
    }

    return Recommendation(recommendation);
}

