/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "AudioLevel.h"
#include <cmath>
#include <iostream>
#include <map>
#include <vector>
#include <cassert>
#include "system/System.h"

const double AudioLevel::DB_FLOOR = -1000.;

struct ScaleDescription
{
    ScaleDescription(double _minDb, double _maxDb, double _zeroPoint) :
        minDb(_minDb), maxDb(_maxDb), zeroPoint(_zeroPoint) { }

    double minDb;
    double maxDb;
    double zeroPoint; // as fraction of total throw
};

static ScaleDescription scaleDescription(AudioLevel::Scale type)
{
    switch (type) {
    case AudioLevel::Scale::Sigmoid:
        return ScaleDescription(-80., +12., 0.75);
    case AudioLevel::Scale::IEC268Meter:
        return ScaleDescription(-70.,   0., 1.00);
    case AudioLevel::Scale::IEC268MeterPlus:
        return ScaleDescription(-70., +10., 0.80);
    case AudioLevel::Scale::Preview:
        return ScaleDescription(-80.,   0., 1.00);
    }
};

double
AudioLevel::voltage_to_dB(double v)
{
    if (v == 0.) return DB_FLOOR;
    else if (v < 0.) return voltage_to_dB(-v);
    double dB = 20. * log10(v);
    return dB;
}

double
AudioLevel::dB_to_voltage(double dB)
{
    if (dB == DB_FLOOR) return 0.;
    double m = pow(10., dB / 20.);
    return m;
}

double
AudioLevel::power_to_dB(double power)
{
    if (power == 0.) return DB_FLOOR;
    else if (power < 0.) return power_to_dB(-power);
    double dB = 10. * log10(power);
    return dB;
}

double
AudioLevel::dB_to_power(double dB)
{
    if (dB == DB_FLOOR) return 0.;
    double m = pow(10., dB / 10.);
    return m;
}

double
AudioLevel::quantity_to_dB(double v, Quantity sort)
{
    if (sort == Quantity::Power) {
        return power_to_dB(v);
    } else {
        return voltage_to_dB(v);
    }
}

double
AudioLevel::dB_to_quantity(double v, Quantity sort)
{
    if (sort == Quantity::Power) {
        return dB_to_power(v);
    } else {
        return dB_to_voltage(v);
    }
}

/* IEC 60-268-18 fader levels.  Thanks to Steve Harris. */

static double iec_dB_to_fader(double db)
{
    double def = 0.0f; // Meter deflection %age

    if (db < -70.0f) {
        def = 0.0f;
    } else if (db < -60.0f) {
        def = (db + 70.0f) * 0.25f;
    } else if (db < -50.0f) {
        def = (db + 60.0f) * 0.5f + 2.5f; // corrected from 5.0f base, thanks Robin Gareus
    } else if (db < -40.0f) {
        def = (db + 50.0f) * 0.75f + 7.5f;
    } else if (db < -30.0f) {
        def = (db + 40.0f) * 1.5f + 15.0f;
    } else if (db < -20.0f) {
        def = (db + 30.0f) * 2.0f + 30.0f;
    } else {
        def = (db + 20.0f) * 2.5f + 50.0f;
    }

    return def;
}

static double iec_fader_to_dB(double def)  // Meter deflection %age
{
    double db = 0.0f;

    if (def >= 50.0f) {
        db = (def - 50.0f) / 2.5f - 20.0f;
    } else if (def >= 30.0f) {
        db = (def - 30.0f) / 2.0f - 30.0f;
    } else if (def >= 15.0f) {
        db = (def - 15.0f) / 1.5f - 40.0f;
    } else if (def >= 7.5f) {
        db = (def - 7.5f) / 0.75f - 50.0f;
    } else if (def >= 2.5f) {
        db = (def - 2.5f) / 0.5f - 60.0f;
    } else {
        db = (def / 0.25f) - 70.0f;
    }

    return db;
}

double
AudioLevel::fader_to_dB(int level, int maxLevel, Scale type)
{
    if (level == 0) return DB_FLOOR;

    ScaleDescription desc = scaleDescription(type);
    
    if (type == Scale::IEC268Meter || type == Scale::IEC268MeterPlus) {

        double maxPercent = iec_dB_to_fader(desc.maxDb);
        double percent = double(level) * maxPercent / double(maxLevel);
        double dB = iec_fader_to_dB(percent);
        return dB;

    } else { // scale proportional to sqrt(fabs(dB))

        int zeroLevel = int(round(maxLevel * desc.zeroPoint));
    
        if (level >= zeroLevel) {
            
            double value = level - zeroLevel;
            double scale = (maxLevel - zeroLevel) / sqrt(desc.maxDb);
            value /= scale;
            double dB = pow(value, 2.);
            return dB;
            
        } else {
            
            double value = zeroLevel - level;
            double scale = zeroLevel / sqrt(0. - desc.minDb);
            value /= scale;
            double dB = pow(value, 2.);
            return 0. - dB;
        }
    }
}


int
AudioLevel::dB_to_fader(double dB, int maxLevel, Scale type)
{
    if (dB == DB_FLOOR) return 0;

    ScaleDescription desc = scaleDescription(type);

    if (type == Scale::IEC268Meter || type == Scale::IEC268MeterPlus) {

        // The IEC scale gives a "percentage travel" for a given dB
        // level, but it reaches 100% at 0dB.  So we want to treat the
        // result not as a percentage, but as a scale between 0 and
        // whatever the "percentage" for our (possibly >0dB) max dB is.
        
        double maxPercent = iec_dB_to_fader(desc.maxDb);
        double percent = iec_dB_to_fader(dB);
        int faderLevel = int((maxLevel * percent) / maxPercent + 0.01f);
        
        if (faderLevel < 0) faderLevel = 0;
        if (faderLevel > maxLevel) faderLevel = maxLevel;
        return faderLevel;

    } else {

        int zeroLevel = int(round(maxLevel * desc.zeroPoint));

        if (dB >= 0.) {
            
            if (desc.maxDb <= 0.) {
                
                return maxLevel;

            } else {

                double value = sqrt(dB);
                double scale = (maxLevel - zeroLevel) / sqrt(desc.maxDb);
                value *= scale;
                int level = int(value + 0.01f) + zeroLevel;
                if (level > maxLevel) level = maxLevel;
                return level;
            }
            
        } else {

            dB = 0. - dB;
            double value = sqrt(dB);
            double scale = zeroLevel / sqrt(0. - desc.minDb);
            value *= scale;
            int level = zeroLevel - int(value + 0.01f);
            if (level < 0) level = 0;
            return level;
        }
    }
}

double
AudioLevel::fader_to_voltage(int level, int maxLevel, Scale type)
{
    if (level == 0) return 0.;
    return dB_to_voltage(fader_to_dB(level, maxLevel, type));
}

int
AudioLevel::voltage_to_fader(double v, int maxLevel, Scale type)
{
    if (v == 0.) return 0;
    double dB = voltage_to_dB(v);
    int fader = dB_to_fader(dB, maxLevel, type);
    return fader;
}
        

