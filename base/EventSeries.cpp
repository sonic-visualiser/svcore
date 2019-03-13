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

#include "EventSeries.h"

bool
EventSeries::isEmpty() const
{
    return m_events.empty();
}

int
EventSeries::count() const
{
    if (m_events.size() > INT_MAX) {
        throw std::logic_error("too many events");
    }
    return int(m_events.size());
}

void
EventSeries::add(const Event &p)
{
    bool isUnique = true;

    auto pitr = lower_bound(m_events.begin(), m_events.end(), p);
    if (pitr != m_events.end() && *pitr == p) {
        isUnique = false;
    }
    m_events.insert(pitr, p);

    if (p.hasDuration() && isUnique) {

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
            i->second.push_back(p);
        }
    }

#ifdef DEBUG_EVENT_SERIES
    std::cerr << "after add:" << std::endl;
    dumpEvents();
    dumpSeams();
#endif
}

void
EventSeries::remove(const Event &p)
{
    // If we are removing the last (unique) example of an event,
    // then we also need to remove it from the seam map. If this
    // is only one of multiple identical events, then we don't.
    bool isUnique = true;
        
    auto pitr = lower_bound(m_events.begin(), m_events.end(), p);
    if (pitr == m_events.end() || *pitr != p) {
        // we don't know this event
        return;
    } else {
        auto nitr = pitr;
        ++nitr;
        if (nitr != m_events.end() && *nitr == p) {
            isUnique = false;
        }
    }

    m_events.erase(pitr);

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

        // Remove any and all instances of p from the seam map; we
        // are only supposed to get here if we are removing the
        // last instance of p from the series anyway
            
        for (auto i = i0; i != i1; ++i) {
            if (i == m_seams.end()) {
                // This can happen only if we have a negative
                // duration, which Event forbids
                throw std::logic_error("unexpectedly reached end of map");
            }
            for (size_t j = 0; j < i->second.size(); ) {
                if (i->second[j] == p) {
                    i->second.erase(i->second.begin() + j);
                } else {
                    ++j;
                }
            }
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
            if (pitr != m_seams.end() &&
                seamsEqual(i->second, pitr->second)) {
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

        // And remove any empty seams from the start of the map
            
        while (m_seams.begin() != m_seams.end() &&
               m_seams.begin()->second.empty()) {
            m_seams.erase(m_seams.begin());
        }
    }

#ifdef DEBUG_EVENT_SERIES
    std::cerr << "after remove:" << std::endl;
    dumpEvents();
    dumpSeams();
#endif
}

bool
EventSeries::contains(const Event &p) const
{
    return binary_search(m_events.begin(), m_events.end(), p);
}

void
EventSeries::clear()
{
    m_events.clear();
    m_seams.clear();
}

EventVector
EventSeries::getEventsSpanning(sv_frame_t frame,
                               sv_frame_t duration) const
{
    EventVector span;
    
    const sv_frame_t start = frame;
    const sv_frame_t end = frame + duration;
        
    // first find any zero-duration events

    auto pitr = lower_bound(m_events.begin(), m_events.end(),
                            Event(start));
    while (pitr != m_events.end() && pitr->getFrame() < end) {
        if (!pitr->hasDuration()) {
            span.push_back(*pitr);
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
        auto pitr = lower_bound(m_events.begin(), m_events.end(), p);
        while (pitr != m_events.end() && *pitr == p) {
            span.push_back(p);
            ++pitr;
        }
    }
            
    return span;
}

EventVector
EventSeries::getEventsWithin(sv_frame_t frame,
                             sv_frame_t duration) const
{
    EventVector span;
    
    const sv_frame_t start = frame;
    const sv_frame_t end = frame + duration;

    // because we don't need to "look back" at events that started
    // earlier than the start of the given range, we can do this
    // entirely from m_events

    auto pitr = lower_bound(m_events.begin(), m_events.end(),
                            Event(start));
    while (pitr != m_events.end() && pitr->getFrame() < end) {
        if (!pitr->hasDuration()) {
            span.push_back(*pitr);
        } else if (pitr->getFrame() + pitr->getDuration() <= end) {
            span.push_back(*pitr);
        }
        ++pitr;
    }
            
    return span;
}

EventVector
EventSeries::getEventsCovering(sv_frame_t frame) const
{
    EventVector cover;

    // first find any zero-duration events

    auto pitr = lower_bound(m_events.begin(), m_events.end(),
                            Event(frame));
    while (pitr != m_events.end() && pitr->getFrame() == frame) {
        if (!pitr->hasDuration()) {
            cover.push_back(*pitr);
        }
        ++pitr;
    }
        
    // now any non-zero-duration ones from the seam map
        
    std::set<Event> found;
    auto sitr = m_seams.lower_bound(frame);
    if (sitr == m_seams.end() || sitr->first > frame) {
        if (sitr != m_seams.begin()) {
            --sitr;
        }                
    }
    if (sitr != m_seams.end() && sitr->first <= frame) {
        for (const auto &p: sitr->second) {
            found.insert(p);
        }
        ++sitr;
    }
    for (const auto &p: found) {
        auto pitr = lower_bound(m_events.begin(), m_events.end(), p);
        while (pitr != m_events.end() && *pitr == p) {
            cover.push_back(p);
            ++pitr;
        }
    }
        
    return cover;
}

bool
EventSeries::getEventPreceding(const Event &e, Event &preceding) const
{
    auto pitr = lower_bound(m_events.begin(), m_events.end(), e);
    if (pitr == m_events.end() || *pitr != e) {
        return false;
    }
    if (pitr == m_events.begin()) {
        return false;
    }
    --pitr;
    preceding = *pitr;
    return true;
}

bool
EventSeries::getEventFollowing(const Event &e, Event &following) const
{
    auto pitr = lower_bound(m_events.begin(), m_events.end(), e);
    if (pitr == m_events.end() || *pitr != e) {
        return false;
    }
    while (*pitr == e) {
        ++pitr;
        if (pitr == m_events.end()) {
            return false;
        }
    }
    following = *pitr;
    return true;
}

Event
EventSeries::getEventByIndex(int index) const
{
    if (index < 0 || index >= count()) {
        throw std::logic_error("index out of range");
    }
    return m_events[index];
}

void
EventSeries::toXml(QTextStream &out,
                   QString indent,
                   QString extraAttributes) const
{
    out << indent << QString("<dataset id=\"%1\" %2>\n")
        .arg(getObjectExportId(this))
        .arg(extraAttributes);
    
    for (const auto &p: m_events) {
        p.toXml(out, indent + "  ");
    }
    
    out << indent << "</dataset>\n";
}


