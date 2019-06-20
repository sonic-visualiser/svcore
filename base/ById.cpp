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

#include "ById.h"

#include <QMutex>

#include <climits>

int
WithId::getNextId()
{
    static int nextId = 0;
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    int i = nextId;
    if (nextId == INT_MAX) {
        nextId = INT_MIN;
    }
    ++nextId;
    return i;
}
