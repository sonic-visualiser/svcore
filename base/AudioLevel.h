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

#ifndef SV_AUDIO_LEVEL_H
#define SV_AUDIO_LEVEL_H

namespace sv {

/**
 * AudioLevel converts audio sample levels between various scales:
 *
 *   - dB values (-inf -> 0dB)
 * 
 *   - floating-point values (-1.0 -> 1.0) such as used for nominal
 *     voltage in floating-point WAV files
 *
 *   - integer values intended to correspond to pixels on a fader
 *     or level scale.
 */
class AudioLevel
{
public:
    static const double DB_FLOOR;

    enum class Scale {
        Sigmoid,                // -80 -> +12 dB (sqrt) - play gain controls
        IEC268Meter,            // -70 ->   0 dB (piecewise)
        IEC268MeterPlus,        // -70 -> +10 dB (piecewise)
        Preview                 // -80 ->   0 dB (sqrt) - meter-scale waveforms
    };

    enum class Quantity {
        Power,
        RootPower
    };

    /** Convert a voltage or voltage-like value (a RootPower quantity)
     *  to a dB value relative to reference +/-1.0.
     * 
     *  This is 20 * log10(abs(v)).
     */
    static double voltage_to_dB(double v);

    /** Convert a dB value relative to reference +1.0V to a voltage.
     *  This is pow(10, dB / 20).
     */
    static double dB_to_voltage(double dB);

    /** Convert a power-like value (a Power quantity) relative to full
     *  scale to a dB value.
     *  This is 10 * log10(abs(v)).
     */
    static double power_to_dB(double power);

    /** Convert a dB value relative to reference +1.0V to a power-like
     *  value.
     *  This is pow(10, dB / 10).
     */
    static double dB_to_power(double dB);

    /** Convert a quantity to a dB value relative to reference 1.0.
     *  If sort is Quantity::RootPower, use voltage_to_dB; if sort is
     *  Quantity::Power, use power_to_dB.
     */
    static double quantity_to_dB(double value, Quantity sort);

    /** Convert a dB value to a quantity relative to reference 1.0.
     *  If sort is Quantity::RootPower, use dB_to_voltage; if sort is
     *  Quantity::Power, use dB_to_power.
     */
    static double dB_to_quantity(double dB, Quantity sort);
    
    /** Convert a fader level on one of the preset scales, in the
     *  range 0-maxLevel, to a dB value.
     */
    static double fader_to_dB(int level, int maxLevel, Scale type);

    /** Convert a dB value to a fader level on one of the preset
        scales, rounding to the nearest discrete fader level within
        the range 0-maxFaderLevel.
     */
    static int dB_to_fader(double dB, int maxFaderLevel, Scale type);

    /** Convert a fader level on one of the preset scales, in the
     *  range 0-maxLevel, to a voltage with reference +1.0.
     */
    static double fader_to_voltage(int level, int maxLevel, Scale type);

    /** Convert a voltage or voltage-like value to a fader level on
        one of the preset scales, with reference +/-1.0V, rounding to
        the nearest discrete fader level within the range
        0-maxFaderLevel.
     */
    static int voltage_to_fader(double dB, int maxFaderLevel, Scale type);
};


} // end namespace sv

#endif

