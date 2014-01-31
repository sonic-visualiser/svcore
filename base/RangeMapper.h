/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RANGE_MAPPER_H_
#define _RANGE_MAPPER_H_

#include <QString>

#include "Debug.h"
#include <map>

class RangeMapper 
{
public:
    virtual ~RangeMapper() { }
    virtual int getPositionForValue(float value) const = 0;
    virtual float getValueForPosition(int position) const = 0;
    virtual QString getUnit() const { return ""; }
};


class LinearRangeMapper : public RangeMapper
{
public:
    /**
     * Map values in range minval->maxval linearly into integer range
     * minpos->maxpos. minval and minpos must be less than maxval and
     * maxpos respectively. If inverted is true, the range will be
     * mapped "backwards" (minval to maxpos and maxval to minpos).
     */
    LinearRangeMapper(int minpos, int maxpos,
                      float minval, float maxval,
                      QString unit = "", bool inverted = false);
    
    virtual int getPositionForValue(float value) const;
    virtual float getValueForPosition(int position) const;

    virtual QString getUnit() const { return m_unit; }

protected:
    int m_minpos;
    int m_maxpos;
    float m_minval;
    float m_maxval;
    QString m_unit;
    bool m_inverted;
};

class LogRangeMapper : public RangeMapper
{
public:
    /**
     * Map values in range minval->maxval into integer range
     * minpos->maxpos such that logs of the values are mapped
     * linearly. minval and minpos must be less than maxval and maxpos
     * respectively. If inverted is true, the range will be mapped
     * "backwards" (minval to maxpos and maxval to minpos).
     */
    LogRangeMapper(int minpos, int maxpos,
                   float minval, float maxval,
                   QString m_unit = "", bool inverted = false);

    static void convertRatioMinLog(float ratio, float minlog,
                                   int minpos, int maxpos,
                                   float &minval, float &maxval);

    static void convertMinMax(int minpos, int maxpos,
                              float minval, float maxval,
                              float &ratio, float &minlog);

    virtual int getPositionForValue(float value) const;
    virtual float getValueForPosition(int position) const;

    virtual QString getUnit() const { return m_unit; }

protected:
    int m_minpos;
    int m_maxpos;
    float m_ratio;
    float m_minlog;
    float m_maxlog;
    QString m_unit;
    bool m_inverted;
};

class InterpolatingRangeMapper : public RangeMapper
{
public:
    typedef std::map<float, int> CoordMap;

    /**
     * Given a series of (value, position) coordinate mappings,
     * construct a range mapper that maps arbitrary values, in the
     * range between minimum and maximum of the provided values, onto
     * coordinates using linear interpolation between the supplied
     * points.
     *
     *!!! todo: Cubic -- more generally useful than linear interpolation
     *!!! todo: inverted flag
     *
     * The set of provided mappings must contain at least two
     * coordinates.
     *
     * It is expected that the values and positions in the coordinate
     * mappings will both be monotonically increasing (i.e. no
     * inflections in the mapping curve). Behaviour is undefined if
     * this is not the case.
     */
    InterpolatingRangeMapper(CoordMap pointMappings,
                             QString unit);

    virtual int getPositionForValue(float value) const;
    virtual float getValueForPosition(int position) const;

    virtual QString getUnit() const { return m_unit; }

protected:
    CoordMap m_mappings;
    std::map<int, float> m_reverse;
    QString m_unit;
};

class AutoRangeMapper : public RangeMapper
{
public:
    enum MappingType {
        Interpolating,
        StraightLine,
        Logarithmic,
    };

    typedef std::map<float, int> CoordMap;

    /**
     * Given a series of (value, position) coordinate mappings,
     * construct a range mapper that maps arbitrary values, in the
     * range between minimum and maximum of the provided values, onto
     * coordinates. 
     *
     * The mapping used may be
     * 
     *    Interpolating -- an InterpolatingRangeMapper will be used
     * 
     *    StraightLine -- a LinearRangeMapper from the minimum to
     *    maximum value coordinates will be used, ignoring all other
     *    supplied coordinate mappings
     * 
     *    Logarithmic -- a LogRangeMapper from the minimum to
     *    maximum value coordinates will be used, ignoring all other
     *    supplied coordinate mappings
     *
     * The mapping will be chosen automatically by looking at the
     * supplied coordinates. If the supplied coordinates fall on a
     * straight line, a StraightLine mapping will be used; if they
     * fall on a log curve, a Logarithmic mapping will be used;
     * otherwise an Interpolating mapping will be used.
     *
     *!!! todo: inverted flag
     *
     * The set of provided mappings must contain at least two
     * coordinates, or at least three if the points are not supposed
     * to be in a straight line.
     *
     * It is expected that the values and positions in the coordinate
     * mappings will both be monotonically increasing (i.e. no
     * inflections in the mapping curve). Behaviour is undefined if
     * this is not the case.
     */
    AutoRangeMapper(CoordMap pointMappings,
                    QString unit);

    ~AutoRangeMapper();

    /**
     * Return the mapping type in use.
     */
    MappingType getType() const { return m_type; }

    virtual int getPositionForValue(float value) const;
    virtual float getValueForPosition(int position) const;

    virtual QString getUnit() const { return m_unit; }

protected:
    MappingType m_type;
    CoordMap m_mappings;
    QString m_unit;
    RangeMapper *m_mapper;

    MappingType chooseMappingTypeFor(const CoordMap &);
};

#endif
