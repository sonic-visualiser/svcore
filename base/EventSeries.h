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
 * and supporting the ability to query which events are active at a
 * given frame or within a span of frames.
 *
 * To that end, in addition to the series of events, it stores a
 * series of "seams", which are frame positions at which the set of
 * simultaneous events changes (i.e. an event of non-zero duration
 * starts or ends) associated with a set of the events that are active
 * at or from that position. These are updated when an event is added
 * or removed.
 */
class EventSeries
{
public:
    EventSeries() : m_count(0) { }
    ~EventSeries() =default;

    EventSeries(const EventSeries &) =default;
    EventSeries &operator=(const EventSeries &) =default;
    EventSeries &operator=(EventSeries &&) =default;
    
    bool operator==(const EventSeries &other) const {
        return m_events == other.m_events;
    }
    
    void add(const Event &p) {

        ++m_events[p];
        ++m_count;

        if (p.hasDuration()) {

            const sv_frame_t frame = p.getFrame();
            const sv_frame_t endFrame = p.getFrame() + p.getDuration();

            createSeam(frame);
            createSeam(endFrame);

            // These calls must both succeed after calling createSeam above
            const auto i0 = m_seams.find(frame);
            const auto i1 = m_seams.find(endFrame);

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

        // If we are removing the last (unique) example of an event,
        // then we also need to remove it from the seam map. If this
        // is only one of multiple identical events, then we don't.
        bool isUnique = false;
        
        auto pitr = m_events.find(p);
        if (pitr == m_events.end()) {
            return; // we don't know this event
        } else {
            if (--(pitr->second) == 0) {
                isUnique = true;
                m_events.erase(pitr);
            }
            --m_count;
        }

        if (p.hasDuration() && isUnique) {
            
            const sv_frame_t frame = p.getFrame();
            const sv_frame_t endFrame = p.getFrame() + p.getDuration();

            const auto i0 = m_seams.find(frame);
            const auto i1 = m_seams.find(endFrame);

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
                    // duration, which Event forbids
                    throw std::logic_error("unexpectedly reached end of map");
                }
                
                i->second.erase(p);
            }

            // Tidy up by removing any entries that are now identical
            // to their predecessors

            std::vector<sv_frame_t> redundant;

            auto pitr = m_seams.end();
            if (i0 != m_seams.begin()) {
                pitr = i0;
                --pitr;
            }

            for (auto i = i0; i != m_seams.end(); ++i) {
                if (pitr != m_seams.end() && i->second == pitr->second) {
                    redundant.push_back(i->first);
                }
                pitr = i;
                if (i == i1) {
                    break;
                }
            }

            for (sv_frame_t f: redundant) {
                m_seams.erase(f);
            }
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
     * Retrieve all events any part of which falls within the span in
     * frames defined by the given frame f and duration d.
     *
     * - An event without duration is within the span if its own frame
     * is greater than or equal to f and less than f + d.
     * 
     * - An event with duration is within the span if its start frame
     * is less than f + d and its start frame plus its duration is
     * greater than f.
     * 
     * Note that getEventsSpanning(f, 0) is not equivalent to
     * getEventsCovering(f). The latter includes durationless events
     * at f and events starting at f, both of which are excluded from
     * the former.
     */
    EventVector getEventsSpanning(sv_frame_t frame,
                                  sv_frame_t duration) const {
        EventVector span;

        const sv_frame_t start = frame;
        const sv_frame_t end = frame + duration;
        
        // first find any zero-duration events
        
        auto pitr = m_events.lower_bound(Event(start, QString()));
        while (pitr != m_events.end() && pitr->first.getFrame() < end) {
            if (!pitr->first.hasDuration()) {
                for (int i = 0; i < pitr->second; ++i) {
                    span.push_back(pitr->first);
                }
            }
            ++pitr;
        }

        // now any non-zero-duration ones from the seam map

        std::set<Event> found;
        auto sitr = m_seams.lower_bound(start);
        if (sitr == m_seams.end() || sitr->first > start) {
            if (sitr != m_seams.begin()) {
                --sitr;
            }                
        }
        while (sitr != m_seams.end() && sitr->first < end) {
            for (const auto &p: sitr->second) {
                found.insert(p);
            }
            ++sitr;
        }
        for (const auto &p: found) {
            int n = m_events.at(p);
            if (n < 1) {
                throw std::logic_error("event is in seams but not events");
            }
            for (int i = 0; i < n; ++i) {
                span.push_back(p);
            }
        }
            
        return span;
    }

    /**
     * Retrieve all events that cover the given frame. An event without
     * duration covers a frame if its own frame is equal to it. An event
     * with duration covers a frame if its start frame is less than or
     * equal to it and its end frame (start + duration) is greater
     * than it.
     */
    EventVector getEventsCovering(sv_frame_t frame) const {
        EventVector cover;

        // first find any zero-duration events
        
        auto pitr = m_events.lower_bound(Event(frame, QString()));
        while (pitr != m_events.end() && pitr->first.getFrame() == frame) {
            if (!pitr->first.hasDuration()) {
                for (int i = 0; i < pitr->second; ++i) {
                    cover.push_back(pitr->first);
                }
            }
            ++pitr;
        }

        // now any non-zero-duration ones from the seam map
        
        auto sitr = m_seams.lower_bound(frame);
        if (sitr == m_seams.end() || sitr->first > frame) {
            if (sitr != m_seams.begin()) {
                --sitr;
            }                
        }
        if (sitr != m_seams.end() && sitr->first <= frame) {
            for (const auto &p: sitr->second) {
                int n = m_events.at(p);
                if (n < 1) {
                    throw std::logic_error("event is in seams but not events");
                }
                for (int i = 0; i < n; ++i) {
                    cover.push_back(p);
                }
            }
        }
        
        return cover;
    }

private:
    /**
     * Total number of events in the series. Will exceed
     * m_events.size() if the series contains duplicate events.
     */
    int m_count;

    /**
     * The (ordered) Events map acts as a list of all the events in
     * the series. For reasons of backward compatibility, we have to
     * handle series containing multiple instances of identical
     * events; the int indexed against each event records the number
     * of instances of that event. We do this in preference to using a
     * multiset, in order to support the case in which we have
     * obtained a set of distinct events (from the seam container
     * below) and then need to return the correct number of each.
     *
     * Because events are immutable, we never have to move one to a
     * different "key" in this map - we only add or delete them or
     * change their counts.
     */
    typedef std::map<Event, int> Events;
    Events m_events;

    /**
     * The FrameEventMap maps from frame number to a set of events. In
     * the seam map this is used to represent the events that are
     * active at that frame, either because they begin at that frame
     * or because they are continuing from an earlier frame. There is
     * an entry here for each frame at which an event starts or ends,
     * with the event appearing in all entries from its start time
     * onward and disappearing again at its end frame.
     *
     * Only events with duration appear in this map; point events
     * appear only in m_events.
     */
    typedef std::map<sv_frame_t, std::set<Event>> FrameEventMap;
    FrameEventMap m_seams;

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
        for (const auto &i: m_events) {
            std::cerr << i.first.toXmlString("  ");
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
