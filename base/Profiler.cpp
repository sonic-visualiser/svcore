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

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2006 Chris Cannam, Guillaume Laurent,
   and QMUL.
*/

#include "Profiler.h"
#include "Debug.h"

#include <sstream>

#include <vector>
#include <algorithm>
#include <set>
#include <map>

namespace sv {

Profiles* Profiles::m_instance = nullptr;

Profiles* Profiles::getInstance()
{
    if (!m_instance) m_instance = new Profiles();
    
    return m_instance;
}

Profiles::Profiles()
{
}

Profiles::~Profiles()
{
}

#ifndef NO_TIMING
void Profiles::accumulate(const char* id, Duration duration)
{
    QMutexLocker locker(&m_mutex);

    ProfilePair &pair(m_profiles[id]);
    ++pair.first;
    pair.second += duration;

    m_lastCalls[id] = duration;

    Duration &worst(m_worstCalls[id]);
    if (duration > worst) {
        worst = duration;
    }
}
#endif

void Profiles::dump()
{
#ifndef NO_TIMING
    QMutexLocker locker(&m_mutex);

    std::ostringstream s;
    s << "\nProfiling points:\n";

    s << "\nBy name:\n\n";

    typedef std::set<const char *, std::less<std::string> > StringSet;

    StringSet profileNames;
    for (ProfileMap::const_iterator i = m_profiles.begin();
         i != m_profiles.end(); ++i) {
        profileNames.insert(i->first);
    }

    for (StringSet::const_iterator i = profileNames.begin();
         i != profileNames.end(); ++i) {

        ProfileMap::const_iterator j = m_profiles.find(*i);

        if (j == m_profiles.end()) continue;

        const ProfilePair &pp(j->second);

        s << *i << " (" << pp.first << " calls):\n";

        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(pp.second);
        
        s << "    Mean:  " << (ns / pp.first).count() << " ns/call\n";

        WorstCallMap::const_iterator k = m_worstCalls.find(*i);
        if (k == m_worstCalls.end()) continue;
        
        Duration worst(k->second);
        auto wns = std::chrono::duration_cast<std::chrono::nanoseconds>(worst);

        s << "    Worst: " << wns.count() << " ns/call\n";

        s << "    Total: " << ns.count() << " ns\n";
    }

    typedef std::multimap<Duration, const char *> TimeRMap;
    typedef std::multimap<int, const char *> IntRMap;
    
    TimeRMap totmap, avgmap, worstmap;
    IntRMap ncallmap;

    for (ProfileMap::const_iterator i = m_profiles.begin();
         i != m_profiles.end(); ++i) {
        totmap.insert(TimeRMap::value_type(i->second.second, i->first));
        avgmap.insert(TimeRMap::value_type(i->second.second /
                                           i->second.first, i->first));
        ncallmap.insert(IntRMap::value_type(i->second.first, i->first));
    }

    for (WorstCallMap::const_iterator i = m_worstCalls.begin();
         i != m_worstCalls.end(); ++i) {
        worstmap.insert(TimeRMap::value_type(i->second, i->first));
    }

    s << "\nBy number of calls:\n\n";

    for (IntRMap::const_iterator i = ncallmap.end(); i != ncallmap.begin(); ) {
        --i;
        s << i->first << ": " << i->second << "\n";
    }

    s << "\nBy average:\n\n";
    for (TimeRMap::const_iterator i = avgmap.end(); i != avgmap.begin(); ) {
        --i;
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(i->first);
        s << ns.count() << ": " << i->second << "\n";
    }

    s << "\nBy worst case:\n\n";
    for (TimeRMap::const_iterator i = worstmap.end(); i != worstmap.begin(); ) {
        --i;
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(i->first);
        s << ns.count() << ": " << i->second << "\n";
    }

    s << "\nBy total:\n\n";
    for (TimeRMap::const_iterator i = totmap.end(); i != totmap.begin(); ) {
        --i;
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(i->first);
        s << ns.count() << ": " << i->second << "\n";
    }

    SVCERR << s.str() << endl;
#endif
}

#ifndef NO_TIMING    

Profiler::Profiler(const char* c, bool showOnDestruct) :
    m_c(c),
    m_showOnDestruct(showOnDestruct),
    m_ended(false)
{
    m_start = std::chrono::steady_clock::now();
}

void
Profiler::update() const
{
    auto t = std::chrono::steady_clock::now();
    SVCERR << "Profiler : id = " << m_c << " - elapsed so far = "
           << std::chrono::duration_cast<std::chrono::nanoseconds>(t - m_start).count()
           << " ns" << endl;
}    

Profiler::~Profiler()
{
    if (!m_ended) end();
}

void
Profiler::end()
{
    auto t = std::chrono::steady_clock::now();
    Profiles::getInstance()->accumulate(m_c, t - m_start);

    if (m_showOnDestruct) {
        SVCERR << "Profiler : id = " << m_c << " - elapsed = "
               << std::chrono::duration_cast<std::chrono::nanoseconds>(t - m_start).count()
               << "ns" << endl;
    }

    m_ended = true;
}
 
#endif

} // end namespace sv

