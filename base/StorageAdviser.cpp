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
    QString path = TempDirectory::getInstance()->getPath();

    int discSpace = GetDiscSpaceMBAvailable(path.toLocal8Bit());
    int memory = GetRealMemoryMBAvailable();

    std::cerr << "Disc space: " << discSpace << ", memory: " << memory << std::endl;

    return Recommendation(0);
}


    
