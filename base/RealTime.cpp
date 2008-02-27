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
   This file copyright 2000-2006 Chris Cannam.
*/

#include <iostream>

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

using std::cerr;
using std::endl;

#include "RealTime.h"
#include "sys/time.h"

// A RealTime consists of two ints that must be at least 32 bits each.
// A signed 32-bit int can store values exceeding +/- 2 billion.  This
// means we can safely use our lower int for nanoseconds, as there are
// 1 billion nanoseconds in a second and we need to handle double that
// because of the implementations of addition etc that we use.
//
// The maximum valid RealTime on a 32-bit system is somewhere around
// 68 years: 999999999 nanoseconds longer than the classic Unix epoch.

#define ONE_BILLION 1000000000

RealTime::RealTime(int s, int n) :
    sec(s), nsec(n)
{
    if (sec == 0) {
	while (nsec <= -ONE_BILLION) { nsec += ONE_BILLION; --sec; }
	while (nsec >=  ONE_BILLION) { nsec -= ONE_BILLION; ++sec; }
    } else if (sec < 0) {
	while (nsec <= -ONE_BILLION) { nsec += ONE_BILLION; --sec; }
	while (nsec > 0)             { nsec -= ONE_BILLION; ++sec; }
    } else { 
	while (nsec >=  ONE_BILLION) { nsec -= ONE_BILLION; ++sec; }
	while (nsec < 0)             { nsec += ONE_BILLION; --sec; }
    }
}

RealTime
RealTime::fromSeconds(double sec)
{
    return RealTime(int(sec), int((sec - int(sec)) * ONE_BILLION + 0.5));
}

RealTime
RealTime::fromMilliseconds(int msec)
{
    return RealTime(msec / 1000, (msec % 1000) * 1000000);
}

RealTime
RealTime::fromTimeval(const struct timeval &tv)
{
    return RealTime(tv.tv_sec, tv.tv_usec * 1000);
}

std::ostream &operator<<(std::ostream &out, const RealTime &rt)
{
    if (rt < RealTime::zeroTime) {
	out << "-";
    } else {
	out << " ";
    }

    int s = (rt.sec < 0 ? -rt.sec : rt.sec);
    int n = (rt.nsec < 0 ? -rt.nsec : rt.nsec);

    out << s << ".";

    int nn(n);
    if (nn == 0) out << "00000000";
    else while (nn < (ONE_BILLION / 10)) {
	out << "0";
	nn *= 10;
    }
    
    out << n << "R";
    return out;
}

std::string
RealTime::toString(bool align) const
{
    std::stringstream out;
    out << *this;
    
#if (__GNUC__ < 3)
    out << std::ends;
#endif

    std::string s = out.str();

    if (!align && *this >= RealTime::zeroTime) {
        // remove leading " "
        s = s.substr(1, s.length() - 1);
    }

    // remove trailing R
    return s.substr(0, s.length() - 1);
}

RealTime
RealTime::fromString(std::string s)
{
    bool negative = false;
    bool faulty = false;
    bool section = 0;
    std::string ssec, snsec;

    for (size_t i = 0; i < s.length(); ++i) {

        char c = s[i];
        if (isspace(c)) continue;

        if (section == 0) {

            if (c == '-') negative = true;
            else if (isdigit(c)) { section = 1; ssec += c; }
            else if (c == '.') section = 2;
            else break;

        } else if (section == 1) {

            if (c == '.') section = 2;
            else if (isdigit(c)) ssec += c;
            else break;

        } else if (section == 2) {

            if (isdigit(c)) snsec += c;
            else break;
        }
    }

    while (snsec.length() < 8) snsec += '0';

    int sec = atoi(ssec.c_str());
    int nsec = atoi(snsec.c_str());
    if (negative) sec = -sec;

    std::cerr << "RealTime::fromString: string " << s << " -> "
              << sec << " sec, " << nsec << " nsec" << std::endl;

    return RealTime(sec, nsec);
}

std::string
RealTime::toText(bool fixedDp) const
{
    if (*this < RealTime::zeroTime) return "-" + (-*this).toText(fixedDp);

    std::stringstream out;

    if (sec >= 3600) {
	out << (sec / 3600) << ":";
    }

    if (sec >= 60) {
	out << (sec % 3600) / 60 << ":";
    }

    if (sec >= 10) {
	out << ((sec % 60) / 10);
    }

    out << (sec % 10);
    
    int ms = msec();

    if (ms != 0) {
	out << ".";
	out << (ms / 100);
	ms = ms % 100;
	if (ms != 0) {
	    out << (ms / 10);
	    ms = ms % 10;
	} else if (fixedDp) {
	    out << "0";
	}
	if (ms != 0) {
	    out << ms;
	} else if (fixedDp) {
	    out << "0";
	}
    } else if (fixedDp) {
	out << ".000";
    }
	
#if (__GNUC__ < 3)
    out << std::ends;
#endif

    std::string s = out.str();

    return s;
}

std::string
RealTime::toSecText() const
{
    if (*this < RealTime::zeroTime) return "-" + (-*this).toSecText();

    std::stringstream out;

    if (sec >= 3600) {
	out << (sec / 3600) << ":";
    }

    if (sec >= 60) {
	out << (sec % 3600) / 60 << ":";
    }

    if (sec >= 10) {
	out << ((sec % 60) / 10);
    }

    out << (sec % 10);
    
    if (sec < 60) {
        out << "s";
    }

	
#if (__GNUC__ < 3)
    out << std::ends;
#endif

    std::string s = out.str();

    return s;
}

RealTime
RealTime::operator*(int m) const
{
    double t = (double(nsec) / ONE_BILLION) * m;
    t += sec * m;
    return fromSeconds(t);
}

RealTime
RealTime::operator/(int d) const
{
    int secdiv = sec / d;
    int secrem = sec % d;

    double nsecdiv = (double(nsec) + ONE_BILLION * double(secrem)) / d;
    
    return RealTime(secdiv, int(nsecdiv + 0.5));
}

RealTime
RealTime::operator*(double m) const
{
    double t = (double(nsec) / ONE_BILLION) * m;
    t += sec * m;
    return fromSeconds(t);
}

RealTime
RealTime::operator/(double d) const
{
    double t = (double(nsec) / ONE_BILLION) / d;
    t += sec / d;
    return fromSeconds(t);
}

double 
RealTime::operator/(const RealTime &r) const
{
    double lTotal = double(sec) * ONE_BILLION + double(nsec);
    double rTotal = double(r.sec) * ONE_BILLION + double(r.nsec);
    
    if (rTotal == 0) return 0.0;
    else return lTotal/rTotal;
}

long
RealTime::realTime2Frame(const RealTime &time, unsigned int sampleRate)
{
    if (time < zeroTime) return -realTime2Frame(-time, sampleRate);

    // We like integers.  The last term is always zero unless the
    // sample rate is greater than 1MHz, but hell, you never know...

    long frame =
	time.sec * sampleRate +
	(time.msec() * sampleRate) / 1000 +
	((time.usec() - 1000 * time.msec()) * sampleRate) / 1000000 +
	((time.nsec - 1000 * time.usec()) * sampleRate) / 1000000000;

    return frame;
}

RealTime
RealTime::frame2RealTime(long frame, unsigned int sampleRate)
{
    if (frame < 0) return -frame2RealTime(-frame, sampleRate);

    RealTime rt;
    rt.sec = frame / long(sampleRate);
    frame -= rt.sec * long(sampleRate);
    rt.nsec = (int)(((float(frame) * 1000000) / long(sampleRate)) * 1000);
    return rt;
}

const RealTime RealTime::zeroTime(0,0);

