/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _PITCH_H_
#define _PITCH_H_

#include <QString>

class Pitch
{
public:
    static float getFrequencyForPitch(int midiPitch,
				      float centsOffset = 0,
				      float concertA = 440.0);

    static int getPitchForFrequency(float frequency,
				    float *centsOffsetReturn = 0,
				    float concertA = 440.0);

    static QString getPitchLabel(int midiPitch,
				 float centsOffset = 0,
				 bool useFlats = false);

    static QString getPitchLabelForFrequency(float frequency,
					     float concertA = 440.0,
					     bool useFlats = false);
};


#endif
