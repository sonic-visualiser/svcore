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

//#define DEBUG_POINT_SERIES 1

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

            createSeam(frame);
            createSeam(endFrame);
            
            auto i0 = m_seams.find(frame); // must succeed after createSeam
            auto i1 = m_seams.find(endFrame); // likewise

            for (auto i = i0; i != i1; ++i) {
                if (i == m_seams.end()) {
                    SVCERR << "ERROR: PointSeries::add: "
                           << "reached end of seam map"
                           << endl;
                    break;
                }
                i->second.insert(p);
            }
        }

#ifdef DEBUG_POINT_SERIES
        std::cerr << "after add:" << std::endl;
        dumpPoints();
        dumpSeams();
#endif
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

            auto i0 = m_seams.find(frame);
            auto i1 = m_seams.find(endFrame);

#ifdef DEBUG_POINT_SERIES
            // This should be impossible if we found p in m_points above
            if (i0 == m_seams.end() || i1 == m_seams.end()) {
                SVCERR << "ERROR: PointSeries::remove: either frame " << frame
                       << " or endFrame " << endFrame
                       << " for point not found in seam map: point is "
                       << p.toXmlString() << endl;
            }
#endif

            for (auto i = i0; i != i1; ++i) {
                if (i == m_seams.end()) {
                    SVCERR << "ERROR: PointSeries::remove: "
                           << "reached end of seam map"
                           << endl;
                    break;
                }
                // Can't just erase(p) as that would erase all of
                // them, if there are several identical ones
                auto si = i->second.find(p);
                if (si != i->second.end()) {
                    i->second.erase(si);
                }
            }

            // Shall we "garbage-collect" here? We could be leaving
            // lots of empty point-sets, or consecutive identical
            // ones, which are a pure irrelevance that take space and
            // slow us down. But a lot depends on whether callers ever
            // really delete anything much.
        }

#ifdef DEBUG_POINT_SERIES
        std::cerr << "after remove:" << std::endl;
        dumpPoints();
        dumpSeams();
#endif
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
    PointVector getPointsSpanning(sv_frame_t frame) const {
        PointVector span;

        // first find any zero-duration points
        auto pitr = m_points.lower_bound(Point(frame, QString()));
        if (pitr != m_points.end()) {
            while (pitr->getFrame() == frame) {
                if (!pitr->haveDuration()) {
                    span.push_back(*pitr);
                }
                ++pitr;
            }
        }

        // now any non-zero-duration ones from the seam map
        auto sitr = m_seams.lower_bound(frame);
        if (sitr == m_seams.end() || sitr->first > frame) {
            if (sitr != m_seams.begin()) {
                --sitr;
            }                
        }
        if (sitr != m_seams.end() && sitr->first <= frame) {
            for (auto p: sitr->second) {
                span.push_back(p);
            }
        }
        
        return span;
    }

private:
    int m_count;

    typedef std::multiset<Point> PointMultiset;
    PointMultiset m_points;

    typedef std::map<sv_frame_t, std::multiset<Point>> FramePointsMap;
    FramePointsMap m_seams;

    /** Create a seam at the given frame, copying from the prior seam
     *  if there is one. If a seam already exists at the given frame,
     *  leave it untouched.
     */
    void createSeam(sv_frame_t frame) {
        auto itr = m_seams.lower_bound(frame);
        if (itr == m_seams.end() || itr->first > frame) {
            if (itr != m_seams.begin()) {
                --itr;
            }
        }
        if (itr == m_seams.end()) {
            m_seams[frame] = {};
        } else if (itr->first < frame) {
            m_seams[frame] = itr->second;
        } else if (itr->first > frame) { // itr must be begin()
            m_seams[frame] = {};
        }
    }

#ifdef DEBUG_POINT_SERIES
    void dumpPoints() const {
        std::cerr << "POINTS [" << std::endl;
        for (const auto &p: m_points) {
            std::cerr << p.toXmlString("  ");
        }
        std::cerr << "]" << std::endl;
    }
    
    void dumpSeams() const {
        std::cerr << "SEAMS [" << std::endl;
        for (const auto &s: m_seams) {
            std::cerr << "  " << s.first << " -> {" << std::endl;
            for (const auto &p: s.second) {
                std::cerr << p.toXmlString("    ");
            }
            std::cerr << "  }" << std::endl;
        }
        std::cerr << "]" << std::endl;
    }
#endif
};

#endif
