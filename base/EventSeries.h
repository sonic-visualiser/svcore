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

#ifndef SV_EVENT_SERIES_H
#define SV_EVENT_SERIES_H

#include "Event.h"

#include <set>

//#define DEBUG_EVENT_SERIES 1

/**
 * Container storing a series of events, with or without durations,
 * and supporting the ability to query which events span a given frame
 * time. To that end, in addition to the series of events, it stores a
 * series of "seam points", which are the frame positions at which the
 * set of simultaneous events changes (i.e. an event of non-zero
 * duration starts or ends). These are updated when event is added or
 * removed.
 */
class EventSeries
{
public:
    EventSeries() : m_count(0) { }
    
    void add(const Event &p) {

        m_events.insert(p);
        ++m_count;

        if (p.hasDuration()) {
            sv_frame_t frame = p.getFrame();
            sv_frame_t endFrame = p.getFrame() + p.getDuration();

            createSeam(frame);
            createSeam(endFrame);
            
            auto i0 = m_seams.find(frame); // must succeed after createSeam
            auto i1 = m_seams.find(endFrame); // likewise

            for (auto i = i0; i != i1; ++i) {
                if (i == m_seams.end()) {
                    SVCERR << "ERROR: EventSeries::add: "
                           << "reached end of seam map"
                           << endl;
                    break;
                }
                i->second.insert(p);
            }
        }

#ifdef DEBUG_EVENT_SERIES
        std::cerr << "after add:" << std::endl;
        dumpEvents();
        dumpSeams();
#endif
    }

    void remove(const Event &p) {

        // erase first itr that matches p; if there is more than one
        // p, erase(p) would remove all of them, but we only want to
        // remove (any) one
        auto pitr = m_events.find(p);
        if (pitr == m_events.end()) {
            return; // we don't know this event
        } else {
            m_events.erase(pitr);
            --m_count;
        }

        if (p.hasDuration()) {
            sv_frame_t frame = p.getFrame();
            sv_frame_t endFrame = p.getFrame() + p.getDuration();

            auto i0 = m_seams.find(frame);
            auto i1 = m_seams.find(endFrame);

#ifdef DEBUG_EVENT_SERIES
            // This should be impossible if we found p in m_events above
            if (i0 == m_seams.end() || i1 == m_seams.end()) {
                SVCERR << "ERROR: EventSeries::remove: either frame " << frame
                       << " or endFrame " << endFrame
                       << " for event not found in seam map: event is "
                       << p.toXmlString() << endl;
            }
#endif

            for (auto i = i0; i != i1; ++i) {
                if (i == m_seams.end()) {
                    // This can happen only if we have a negative
                    // duration, which Event forbids, but we don't
                    // protect against it in this class, so we'll
                    // leave this check in
                    SVCERR << "ERROR: EventSeries::remove: "
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
            // lots of empty event-sets, or consecutive identical
            // ones, which are a pure irrelevance that take space and
            // slow us down. But a lot depends on whether callers ever
            // really delete anything much.
        }

#ifdef DEBUG_EVENT_SERIES
        std::cerr << "after remove:" << std::endl;
        dumpEvents();
        dumpSeams();
#endif
    }

    bool contains(const Event &p) {
        return m_events.find(p) != m_events.end();
    }

    int count() const {
        return m_count;
    }

    bool isEmpty() const {
        return m_count == 0;
    }
    
    void clear() {
        m_events.clear();
        m_seams.clear();
        m_count = 0;
    }

    /**
     * Retrieve all events that span the given frame. A event without
     * duration spans a frame if its own frame is equal to it. A event
     * with duration spans a frame if its start frame is less than or
     * equal to it and its end frame (start + duration) is greater
     * than it.
     */
    EventVector getEventsSpanning(sv_frame_t frame) const {
        EventVector span;

        // first find any zero-duration events
        auto pitr = m_events.lower_bound(Event(frame, QString()));
        if (pitr != m_events.end()) {
            while (pitr->getFrame() == frame) {
                if (!pitr->hasDuration()) {
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

    typedef std::multiset<Event> EventMultiset;
    EventMultiset m_events;

    typedef std::map<sv_frame_t, std::multiset<Event>> FrameEventsMap;
    FrameEventsMap m_seams;

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

#ifdef DEBUG_EVENT_SERIES
    void dumpEvents() const {
        std::cerr << "EVENTS [" << std::endl;
        for (const auto &p: m_events) {
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
