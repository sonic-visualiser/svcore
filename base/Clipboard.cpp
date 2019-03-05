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

Clipboard::Point::Point(sv_frame_t frame, QString label) :
    m_haveValue(false),
    m_haveLevel(false),
    m_haveFrame(true),
    m_haveDuration(false),
    m_haveReferenceFrame(false),
    m_haveLabel(true),
    m_value(0),
    m_level(0.f),
    m_frame(frame),
    m_duration(0),
    m_referenceFrame(frame),
    m_label(label)
{
}

Clipboard::Point::Point(sv_frame_t frame, float value, QString label) :
    m_haveValue(true),
    m_haveLevel(false),
    m_haveFrame(true),
    m_haveDuration(false),
    m_haveReferenceFrame(false),
    m_haveLabel(true),
    m_value(value),
    m_level(0.f),
    m_frame(frame),
    m_duration(0),
    m_referenceFrame(frame),
    m_label(label)
{
}

Clipboard::Point::Point(sv_frame_t frame, float value, sv_frame_t duration, QString label) :
    m_haveValue(true),
    m_haveLevel(false),
    m_haveFrame(true),
    m_haveDuration(true),
    m_haveReferenceFrame(false),
    m_haveLabel(true),
    m_value(value),
    m_level(0.f),
    m_frame(frame),
    m_duration(duration),
    m_referenceFrame(frame),
    m_label(label)
{
}

Clipboard::Point::Point(sv_frame_t frame, float value, sv_frame_t duration, float level, QString label) :
    m_haveValue(true),
    m_haveLevel(true),
    m_haveFrame(true),
    m_haveDuration(true),
    m_haveReferenceFrame(false),
    m_haveLabel(true),
    m_value(value),
    m_level(level),
    m_frame(frame),
    m_duration(duration),
    m_referenceFrame(frame),
    m_label(label)
{
}

bool
Clipboard::Point::haveFrame() const
{
    return m_haveFrame;
}

sv_frame_t
Clipboard::Point::getFrame() const
{
    return m_frame;
}

Clipboard::Point
Clipboard::Point::withFrame(sv_frame_t frame) const
{
    Point p(*this);
    p.m_haveFrame = true;
    p.m_frame = frame;
    return p;
}

bool
Clipboard::Point::haveValue() const
{
    return m_haveValue;
}

float
Clipboard::Point::getValue() const
{
    return m_value;
}

Clipboard::Point
Clipboard::Point::withValue(float value) const
{
    Point p(*this);
    p.m_haveValue = true;
    p.m_value = value;
    return p;
}

bool
Clipboard::Point::haveDuration() const
{
    return m_haveDuration;
}

sv_frame_t
Clipboard::Point::getDuration() const
{
    return m_duration;
}

Clipboard::Point
Clipboard::Point::withDuration(sv_frame_t duration) const
{
    Point p(*this);
    p.m_haveDuration = true;
    p.m_duration = duration;
    return p;
}

bool
Clipboard::Point::haveLabel() const
{
    return m_haveLabel;
}

QString
Clipboard::Point::getLabel() const
{
    return m_label;
}

Clipboard::Point
Clipboard::Point::withLabel(QString label) const
{
    Point p(*this);
    p.m_haveLabel = true;
    p.m_label = label;
    return p;
}

bool
Clipboard::Point::haveLevel() const
{
    return m_haveLevel;
}

float
Clipboard::Point::getLevel() const
{
    return m_level;
}

Clipboard::Point
Clipboard::Point::withLevel(float level) const
{
    Point p(*this);
    p.m_haveLevel = true;
    p.m_level = level;
    return p;
}

bool
Clipboard::Point::haveReferenceFrame() const
{
    return m_haveReferenceFrame;
}

bool
Clipboard::Point::referenceFrameDiffers() const
{
    return m_haveReferenceFrame && (m_referenceFrame != m_frame);
}

sv_frame_t
Clipboard::Point::getReferenceFrame() const
{
    return m_referenceFrame;
}

void
Clipboard::Point::setReferenceFrame(sv_frame_t f) 
{
    m_haveReferenceFrame = true;
    m_referenceFrame = f;
}

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

