/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2005 Chris Cannam.
*/

#ifndef _AUDIO_LEVEL_H_
#define _AUDIO_LEVEL_H_

/**
 * AudioLevel converts audio sample levels between various scales:
 *
 *   - dB values (-inf -> 0dB)
 *   - floating-point values (-1.0 -> 1.0) such as used for a
 *     multiplier for gain or in floating-point WAV files
 *   - integer values intended to correspond to pixels on a fader
 *     or vu level scale.
 */

class AudioLevel
{
public:

    static const float DB_FLOOR;

    enum FaderType {
	     ShortFader = 0, // -40 -> +6  dB
              LongFader = 1, // -70 -> +10 dB
            IEC268Meter = 2, // -70 ->  0  dB
        IEC268LongMeter = 3, // -70 -> +10 dB (0dB aligns with LongFader)
	   PreviewLevel = 4
    };

    static float multiplier_to_dB(float multiplier);
    static float dB_to_multiplier(float dB);

    static float fader_to_dB(int level, int maxLevel, FaderType type);
    static int   dB_to_fader(float dB, int maxFaderLevel, FaderType type);

    static float fader_to_multiplier(int level, int maxLevel, FaderType type);
    static int   multiplier_to_fader(float multiplier, int maxFaderLevel,
				     FaderType type);

    // fast if "levels" doesn't change often -- for audio segment previews
    static int   multiplier_to_preview(float multiplier, int levels);
    static float preview_to_multiplier(int level, int levels);
};


#endif

