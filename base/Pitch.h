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
