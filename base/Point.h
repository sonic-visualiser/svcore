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

#ifndef SV_POINT_H
#define SV_POINT_H

#include <QString>

#include "BaseTypes.h"

class Point
{
public:
    Point(sv_frame_t frame, QString label) :
        m_haveValue(false), m_haveLevel(false), m_haveFrame(true),
        m_haveDuration(false), m_haveReferenceFrame(false), m_haveLabel(true),
        m_value(0.f), m_level(0.f), m_frame(frame),
        m_duration(0), m_referenceFrame(0), m_label(label) { }
        
    Point(sv_frame_t frame, float value, QString label) :
        m_haveValue(true), m_haveLevel(false), m_haveFrame(true),
        m_haveDuration(false), m_haveReferenceFrame(false), m_haveLabel(true),
        m_value(value), m_level(0.f), m_frame(frame),
        m_duration(0), m_referenceFrame(0), m_label(label) { }
        
    Point(sv_frame_t frame, float value, sv_frame_t duration, QString label) :
        m_haveValue(true), m_haveLevel(false), m_haveFrame(true),
        m_haveDuration(true), m_haveReferenceFrame(false), m_haveLabel(true),
        m_value(value), m_level(0.f), m_frame(frame),
        m_duration(duration), m_referenceFrame(0), m_label(label) { }
        
    Point(sv_frame_t frame, float value, sv_frame_t duration, float level, QString label) :
        m_haveValue(true), m_haveLevel(true), m_haveFrame(true),
        m_haveDuration(true), m_haveReferenceFrame(false), m_haveLabel(true),
        m_value(value), m_level(level), m_frame(frame),
        m_duration(duration), m_referenceFrame(0), m_label(label) { }

    Point(const Point &point) =default;
    Point &operator=(const Point &point) =default;
    Point &operator=(Point &&point) =default;
    
    bool haveFrame() const { return m_haveFrame; }
    sv_frame_t getFrame() const { return m_frame; }

    Point withFrame(sv_frame_t frame) const {
        Point p(*this);
        p.m_haveFrame = true;
        p.m_frame = frame;
        return p;
    }
    
    bool haveValue() const { return m_haveValue; }
    float getValue() const { return m_value; }
    
    Point withValue(float value) const {
        Point p(*this);
        p.m_haveValue = true;
        p.m_value = value;
        return p;
    }
    
    bool haveDuration() const { return m_haveDuration; }
    sv_frame_t getDuration() const { return m_duration; }

    Point withDuration(sv_frame_t duration) const {
        Point p(*this);
        p.m_haveDuration = true;
        p.m_duration = duration;
        return p;
    }
    
    bool haveLabel() const { return m_haveLabel; }
    QString getLabel() const { return m_label; }

    Point withLabel(QString label) const {
        Point p(*this);
        p.m_haveLabel = true;
        p.m_label = label;
        return p;
    }
    
    bool haveLevel() const { return m_haveLevel; }
    float getLevel() const { return m_level; }
    Point withLevel(float level) const {
        Point p(*this);
        p.m_haveLevel = true;
        p.m_level = level;
        return p;
    }
    
    bool haveReferenceFrame() const { return m_haveReferenceFrame; }
    sv_frame_t getReferenceFrame() const { return m_referenceFrame; }
        
    bool referenceFrameDiffers() const { // from point frame
        return m_haveReferenceFrame && (m_referenceFrame != m_frame);
    }
    
    Point withReferenceFrame(sv_frame_t frame) const {
        Point p(*this);
        p.m_haveReferenceFrame = true;
        p.m_referenceFrame = frame;
        return p;
    }
    
private:
    // The order of fields here is chosen to minimise overall size of struct.
    // If you change something, check what difference it makes to packing.
    bool m_haveValue : 1;
    bool m_haveLevel : 1;
    bool m_haveFrame : 1;
    bool m_haveDuration : 1;
    bool m_haveReferenceFrame : 1;
    bool m_haveLabel : 1;
    float m_value;
    float m_level;
    sv_frame_t m_frame;
    sv_frame_t m_duration;
    sv_frame_t m_referenceFrame;
    QString m_label;
};

#endif
