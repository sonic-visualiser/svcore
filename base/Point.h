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
#include <vector>

#include "BaseTypes.h"
#include "XmlExportable.h"

//!!! given that these can have size (i.e. duration), maybe Point
//!!! isn't really an ideal name... perhaps I should go back to dull
//!!! old Event

class Point
{
public:
    Point(sv_frame_t frame, QString label) :
        m_haveValue(false), m_haveLevel(false), m_haveReferenceFrame(false),
        m_value(0.f), m_level(0.f), m_frame(frame),
        m_duration(0), m_referenceFrame(0), m_label(label) { }
        
    Point(sv_frame_t frame, float value, QString label) :
        m_haveValue(true), m_haveLevel(false), m_haveReferenceFrame(false),
        m_value(value), m_level(0.f), m_frame(frame),
        m_duration(0), m_referenceFrame(0), m_label(label) { }
        
    Point(sv_frame_t frame, float value, sv_frame_t duration, QString label) :
        m_haveValue(true), m_haveLevel(false), m_haveReferenceFrame(false),
        m_value(value), m_level(0.f), m_frame(frame),
        m_duration(duration), m_referenceFrame(0), m_label(label) { }
        
    Point(sv_frame_t frame, float value, sv_frame_t duration,
          float level, QString label) :
        m_haveValue(true), m_haveLevel(true), m_haveReferenceFrame(false),
        m_value(value), m_level(level), m_frame(frame),
        m_duration(duration), m_referenceFrame(0), m_label(label) { }

    Point(const Point &point) =default;
    Point &operator=(const Point &point) =default;
    Point &operator=(Point &&point) =default;
    
    sv_frame_t getFrame() const { return m_frame; }

    Point withFrame(sv_frame_t frame) const {
        Point p(*this);
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
    
    bool haveDuration() const { return m_duration != 0; }
    sv_frame_t getDuration() const { return m_duration; }

    Point withDuration(sv_frame_t duration) const {
        Point p(*this);
        p.m_duration = duration;
        return p;
    }
    
    QString getLabel() const { return m_label; }

    Point withLabel(QString label) const {
        Point p(*this);
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

    bool operator==(const Point &p) const {

        if (m_frame != p.m_frame) return false;

        if (m_haveValue != p.m_haveValue) return false;
        if (m_haveValue && (m_value != p.m_value)) return false;

        if (m_duration != p.m_duration) return false;

        if (m_haveLevel != p.m_haveLevel) return false;
        if (m_haveLevel && (m_level != p.m_level)) return false;

        if (m_haveReferenceFrame != p.m_haveReferenceFrame) return false;
        if (m_haveReferenceFrame &&
            (m_referenceFrame != p.m_referenceFrame)) return false;
        
        if (m_label != p.m_label) return false;
        
        return true;
    }

    bool operator<(const Point &p) const {

        if (m_frame != p.m_frame) return m_frame < p.m_frame;

        // points without a property sort before points with that property

        if (m_haveValue != p.m_haveValue) return !m_haveValue;
        if (m_haveValue && (m_value != p.m_value)) return m_value < p.m_value;
        
        if (m_duration != p.m_duration) return m_duration < p.m_duration;
        
        if (m_haveLevel != p.m_haveLevel) return !m_haveLevel;
        if (m_haveLevel && (m_level != p.m_level)) return m_level < p.m_level;

        if (m_haveReferenceFrame != p.m_haveReferenceFrame) {
            return !m_haveReferenceFrame;
        }
        if (m_haveReferenceFrame && (m_referenceFrame != p.m_referenceFrame)) {
            return m_referenceFrame < p.m_referenceFrame;
        }
        
        return m_label < p.m_label;
    }

    void toXml(QTextStream &stream,
               QString indent = "",
               QString extraAttributes = "") const {

        stream << indent << QString("<point frame=\"%1\" ").arg(m_frame);
        if (m_haveValue) stream << QString("value=\"%1\" ").arg(m_value);
        if (m_duration) stream << QString("duration=\"%1\" ").arg(m_duration);
        if (m_haveLevel) stream << QString("level=\"%1\" ").arg(m_level);
        if (m_haveReferenceFrame) stream << QString("referenceFrame=\"%1\" ")
                                      .arg(m_referenceFrame);
        stream << QString("label=\"%1\" ")
            .arg(XmlExportable::encodeEntities(m_label));
        stream << extraAttributes << ">\n";
    }

    QString toXmlString(QString indent = "",
                        QString extraAttributes = "") const {
        QString s;
        QTextStream out(&s);
        toXml(out, indent, extraAttributes);
        out.flush();
        return s;
    }
    
private:
    // The order of fields here is chosen to minimise overall size of struct.
    // We potentially store very many of these objects.
    // If you change something, check what difference it makes to packing.
    bool m_haveValue : 1;
    bool m_haveLevel : 1;
    bool m_haveReferenceFrame : 1;
    float m_value;
    float m_level;
    sv_frame_t m_frame;
    sv_frame_t m_duration;
    sv_frame_t m_referenceFrame;
    QString m_label;
};

typedef std::vector<Point> PointVector;

#endif
