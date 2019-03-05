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

#include "Clipboard.h"

Clipboard::Clipboard() { }
Clipboard::~Clipboard() { }

void
Clipboard::clear()
{
    m_points.clear();
}

bool
Clipboard::empty() const
{
    return m_points.empty();
}

const Clipboard::PointList &
Clipboard::getPoints() const
{
    return m_points;
}

void
Clipboard::setPoints(const PointList &pl)
{
    m_points = pl;
}

void
Clipboard::addPoint(const Point &point)
{
    m_points.push_back(point);
}

bool
Clipboard::haveReferenceFrames() const
{
    for (PointList::const_iterator i = m_points.begin();
         i != m_points.end(); ++i) {
        if (i->haveReferenceFrame()) return true;
    } 
    return false;
}

bool
Clipboard::referenceFramesDiffer() const
{
    for (PointList::const_iterator i = m_points.begin();
         i != m_points.end(); ++i) {
        if (i->referenceFrameDiffers()) return true;
    } 
    return false;
}

