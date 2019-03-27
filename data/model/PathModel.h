/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2007 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_PATH_MODEL_H
#define SV_PATH_MODEL_H

#include "Model.h"
#include "DeferredNotifier.h"
#include "base/RealTime.h"
#include "base/BaseTypes.h"

#include "base/XmlExportable.h"
#include "base/RealTime.h"

#include <QStringList>
#include <set>

struct PathPoint
{
    PathPoint(sv_frame_t _frame) :
        frame(_frame), mapframe(_frame) { }
    PathPoint(sv_frame_t _frame, sv_frame_t _mapframe) :
        frame(_frame), mapframe(_mapframe) { }

    sv_frame_t frame;
    sv_frame_t mapframe;

    void toXml(QTextStream &stream, QString indent = "",
               QString extraAttributes = "") const {
        stream << QString("%1<point frame=\"%2\" mapframe=\"%3\" %4/>\n")
            .arg(indent).arg(frame).arg(mapframe).arg(extraAttributes);
    }
        
    QString toDelimitedDataString(QString delimiter, DataExportOptions,
                                  sv_samplerate_t sampleRate) const {
        QStringList list;
        list << RealTime::frame2RealTime(frame, sampleRate).toString().c_str();
        list << QString("%1").arg(mapframe);
        return list.join(delimiter);
    }

    bool operator<(const PathPoint &p2) const {
        if (frame != p2.frame) return frame < p2.frame;
        return mapframe < p2.mapframe;
    }
};

class PathModel : public Model
{
public:
    typedef std::set<PathPoint> PointList;

    PathModel(sv_samplerate_t sampleRate,
              int resolution,
              bool notifyOnAdd = true) :
        m_sampleRate(sampleRate),
        m_resolution(resolution),
        m_notifier(this,
                   notifyOnAdd ?
                   DeferredNotifier::NOTIFY_ALWAYS :
                   DeferredNotifier::NOTIFY_DEFERRED),
        m_completion(100),
        m_start(0),
        m_end(0) {
    }

    QString getTypeName() const override { return tr("Path"); }
    bool isSparse() const { return true; }
    bool isOK() const override { return true; }

    sv_frame_t getStartFrame() const override {
        return m_start;
    }
    sv_frame_t getEndFrame() const override {
        return m_end;
    }
    
    sv_samplerate_t getSampleRate() const override { return m_sampleRate; }
    int getResolution() const { return m_resolution; }
    
    int getCompletion() const { return m_completion; }

    void setCompletion(int completion, bool update = true) {
        
        {   QMutexLocker locker(&m_mutex);
            if (m_completion == completion) return;
            m_completion = completion;
        }

        if (update) {
            m_notifier.makeDeferredNotifications();
        }
        
        emit completionChanged();

        if (completion == 100) {
            // henceforth:
            m_notifier.switchMode(DeferredNotifier::NOTIFY_ALWAYS);
            emit modelChanged();
        }
    }

    /**
     * Query methods.
     */
    int getPointCount() const {
        return int(m_points.size());
    }
    const PointList &getPoints() const {
        return m_points;
    }

    /**
     * Editing methods.
     */
    void add(PathPoint p) {

        {   QMutexLocker locker(&m_mutex);
            m_points.insert(p);

            if (m_start == m_end) {
                m_start = p.frame;
                m_end = m_start + m_resolution;
            } else {
                if (p.frame < m_start) {
                    m_start = p.frame;
                }
                if (p.frame + m_resolution > m_end) {
                    m_end = p.frame + m_resolution;
                }
            }
        }
        
        m_notifier.update(p.frame, m_resolution);
    }
    
    void remove(PathPoint p) {
        {   QMutexLocker locker(&m_mutex);
            m_points.erase(p);
        }

        emit modelChangedWithin(p.frame, p.frame + m_resolution);
    }

    void clear() {
        {   QMutexLocker locker(&m_mutex);
            m_start = m_end = 0;
            m_points.clear();
        }
    }

    /**
     * XmlExportable methods.
     */
    void toXml(QTextStream &out,
                       QString indent = "",
                       QString extraAttributes = "") const override {
        
        Model::toXml
            (out,
             indent,
             QString("type=\"sparse\" dimensions=\"2\" resolution=\"%1\" "
                     "notifyOnAdd=\"%2\" dataset=\"%3\" subtype=\"path\" %4")
             .arg(m_resolution)
             .arg("true") // always true after model reaches 100% -
                          // subsequent points are always notified
             .arg(getObjectExportId(&m_points))
             .arg(extraAttributes));

        
    }
    
protected:
    sv_samplerate_t m_sampleRate;
    int m_resolution;

    DeferredNotifier m_notifier;
    int m_completion;

    sv_frame_t m_start;
    sv_frame_t m_end;
    PointList m_points;

    mutable QMutex m_mutex;  
};


#endif
