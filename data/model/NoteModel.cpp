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

#include "NoteModel.h"

NoteModel::PointList
NoteModel::getPoints(long start, long end) const
{
    if (start > end) return PointList();
    QMutexLocker locker(&m_mutex);

    Note endPoint(end);
    
    PointListIterator endItr = m_points.upper_bound(endPoint);

    if (endItr != m_points.end()) ++endItr;
    if (endItr != m_points.end()) ++endItr;

    PointList rv;

    for (PointListIterator i = endItr; i != m_points.begin(); ) {
        --i;
        if (i->frame < start) {
            if (i->frame + i->duration >= start) {
                rv.insert(*i);
            }
        } else if (i->frame <= end) {
            rv.insert(*i);
        }
    }

    return rv;
}

NoteModel::PointList
NoteModel::getPoints(long frame) const
{
    QMutexLocker locker(&m_mutex);

    if (m_resolution == 0) return PointList();

    long start = (frame / m_resolution) * m_resolution;
    long end = start + m_resolution;

    Note endPoint(end);
    
    PointListIterator endItr = m_points.upper_bound(endPoint);

    PointList rv;

    for (PointListIterator i = endItr; i != m_points.begin(); ) {
        --i;
        if (i->frame < start) {
            if (i->frame + i->duration >= start) {
                rv.insert(*i);
            }
        } else if (i->frame <= end) {
            rv.insert(*i);
        }
    }

    return rv;
}
