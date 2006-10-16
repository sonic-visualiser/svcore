/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "RangeMapper.h"

#include <cassert>
#include <cmath>

#include <iostream>

LinearRangeMapper::LinearRangeMapper(int minpos, int maxpos,
				     float minval, float maxval,
                                     QString unit) :
    m_minpos(minpos),
    m_maxpos(maxpos),
    m_minval(minval),
    m_maxval(maxval),
    m_unit(unit)
{
    assert(m_maxval != m_minval);
    assert(m_maxpos != m_minpos);
}

int
LinearRangeMapper::getPositionForValue(float value) const
{
    int position = lrintf(((value - m_minval) / (m_maxval - m_minval))
                          * (m_maxpos - m_minpos));
    if (position < m_minpos) position = m_minpos;
    if (position > m_maxpos) position = m_maxpos;
    return position;
}

float
LinearRangeMapper::getValueForPosition(int position) const
{
    float value = ((float(position - m_minpos) / float(m_maxpos - m_minpos))
                   * (m_maxval - m_minval));
    if (value < m_minval) value = m_minval;
    if (value > m_maxval) value = m_maxval;
    return value;
}

LogRangeMapper::LogRangeMapper(int minpos, int maxpos,
                               float ratio, float minlog,
                               QString unit) :
    m_minpos(minpos),
    m_maxpos(maxpos),
    m_minlog(minlog),
    m_unit(unit)
{
    assert(m_maxpos != m_minpos);

    m_maxlog = (m_maxpos - m_minpos) / m_ratio + m_minlog;
}

int
LogRangeMapper::getPositionForValue(float value) const
{
    float mapped = m_ratio * log10(value);
    int position = lrintf(((mapped - m_minlog) / (m_maxlog - m_minlog))
                          * (m_maxpos - m_minpos));
    if (position < m_minpos) position = m_minpos;
    if (position > m_maxpos) position = m_maxpos;
    std::cerr << "LogRangeMapper::getPositionForValue: " << value << " -> "
              << position << std::endl;
    return position;
}

float
LogRangeMapper::getValueForPosition(int position) const
{
    float value = powf(10, (position - m_minpos) / m_ratio + m_minlog);
    std::cerr << "LogRangeMapper::getPositionForValue: " << position << " -> "
              << value << std::endl;
    return value;
}

