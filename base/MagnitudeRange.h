/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef MAGNITUDE_RANGE_H
#define MAGNITUDE_RANGE_H

#include <vector>

/**
 * Maintain a min and max value, and update them when supplied a new
 * data point.
 */
class MagnitudeRange
{
public:
    MagnitudeRange() : m_min(0), m_max(0) { }
    MagnitudeRange(float min, float max) : m_min(min), m_max(max) { }
    
    bool operator==(const MagnitudeRange &r) {
	return r.m_min == m_min && r.m_max == m_max;
    }
    bool operator!=(const MagnitudeRange &r) {
        return !(*this == r);
    }
    
    bool isSet() const { return (m_min != 0.f || m_max != 0.f); }
    void set(float min, float max) {
	m_min = min;
	m_max = max;
	if (m_max < m_min) m_max = m_min;
    }
    bool sample(float f) {
	bool changed = false;
	if (isSet()) {
	    if (f < m_min) { m_min = f; changed = true; }
	    if (f > m_max) { m_max = f; changed = true; }
	} else {
	    m_max = m_min = f;
	    changed = true;
	}
	return changed;
    }
    bool sample(const std::vector<float> &ff) {
        bool changed = false;
        for (auto f: ff) {
            if (sample(f)) {
                changed = true;
            }
        }
        return changed;
    }
    bool sample(const MagnitudeRange &r) {
	bool changed = false;
	if (isSet()) {
	    if (r.m_min < m_min) { m_min = r.m_min; changed = true; }
	    if (r.m_max > m_max) { m_max = r.m_max; changed = true; }
	} else {
	    m_min = r.m_min;
	    m_max = r.m_max;
	    changed = true;
	}
	return changed;
    }            
    float getMin() const { return m_min; }
    float getMax() const { return m_max; }
private:
    float m_min;
    float m_max;
};

#endif
