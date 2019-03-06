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

#ifndef SV_POINT_SERIES_H
#define SV_POINT_SERIES_H

#include "Point.h"

#include <set>

class PointSeries
{
public:
    PointSeries() : m_count(0) { }
    
    void add(const Point &p) {

        m_points.insert(p);
        ++m_count;

        if (p.haveDuration()) {
            sv_frame_t frame = p.getFrame();
            sv_frame_t endFrame = p.getFrame() + p.getDuration();

            std::set<Point> active;
            auto itr = m_seams.lower_bound(frame);
            if (itr == m_seams.end() || itr->first > frame) {
                if (itr != m_seams.begin()) {
                    --itr;
                }
            }
            if (itr != m_seams.end()) {
                active = itr->second;
            }
            active.insert(p);
            m_seams[frame] = active;

            for (itr = m_seams.find(frame); itr->first < endFrame; ++itr) {
                active = itr->second;
                itr->second.insert(p);
            }

            m_seams[endFrame] = active;
        }
    }

    void remove(const Point &p) {

        // erase first itr that matches p; if there is more than one
        // p, erase(p) would remove all of them, but we only want to
        // remove (any) one
        auto pitr = m_points.find(p);
        if (pitr == m_points.end()) {
            return; // we don't know this point
        } else {
            m_points.erase(pitr);
            --m_count;
        }

        if (p.haveDuration()) {
            sv_frame_t frame = p.getFrame();
            sv_frame_t endFrame = p.getFrame() + p.getDuration();

            auto itr = m_seams.find(frame);
            if (itr == m_seams.end()) {
                SVCERR << "WARNING: PointSeries::remove: frame " << frame
                       << " for point not found in seam map: point is "
                       << p.toXmlString() << endl;
                return;
            }

            while (itr != m_seams.end() && itr->first <= endFrame) {
                itr->second.erase(p);
                ++itr;
            }

            // Shall we "garbage-collect" here? We could be leaving
            // lots of empty point-sets, or consecutive identical
            // ones, which are a pure irrelevance that take space and
            // slow us down. But a lot depends on whether callers ever
            // really delete anything much.
        }
    }

    bool contains(const Point &p) {
        return m_points.find(p) != m_points.end();
    }

    int count() const {
        return m_count;
    }

    bool isEmpty() const {
        return m_count == 0;
    }
    
    void clear() {
        m_points.clear();
        m_seams.clear();
        m_count = 0;
    }

    /**
     * Retrieve all points that span the given frame. A point without
     * duration spans a frame if its own frame is equal to it. A point
     * with duration spans a frame if its start frame is less than or
     * equal to it and its end frame (start + duration) is greater
     * than it.
     */
    PointVector getPointsSpanning(sv_frame_t frame) {
        return {};
    }

private:
    int m_count;
    std::multiset<Point> m_points;
    std::map<sv_frame_t, std::set<Point>> m_seams;
};

#endif
