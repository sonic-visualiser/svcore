/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2006 Chris Cannam and Guillaume Laurent.
*/


#ifndef _PROFILER_H_
#define _PROFILER_H_

#include "System.h"

#include <ctime>
#include <sys/time.h>
#include <map>

#include "RealTime.h"


/**
 * Profiling classes
 */

/**
 * The class holding all profiling data
 *
 * This class is a singleton
 */
class Profiles
{
public:
    static Profiles* getInstance();
    ~Profiles();

    void accumulate(const char* id, clock_t time, RealTime rt);
    void dump();

protected:
    Profiles();

    typedef std::pair<clock_t, RealTime> TimePair;
    typedef std::pair<int, TimePair> ProfilePair;
    typedef std::map<const char *, ProfilePair> ProfileMap;
    typedef std::map<const char *, TimePair> LastCallMap;
    ProfileMap m_profiles;
    LastCallMap m_lastCalls;

    static Profiles* m_instance;
};

class Profiler
{
public:
    Profiler(const char*, bool showOnDestruct = false);
    ~Profiler();

    void update();

protected:
    const char* m_c;
    clock_t m_startCPU;
    RealTime m_startTime;
    bool m_showOnDestruct;
};
 

#endif
