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

#ifndef NOTE_DATA_H
#define NOTE_DATA_H

#include <vector>

#include "base/Pitch.h"

struct NoteData
{
    NoteData(int _start, int _dur, int _mp, int _vel) :
	start(_start), duration(_dur), midiPitch(_mp), frequency(0),
	isMidiPitchQuantized(true), velocity(_vel), channel(0) { };
            
    int start;       // audio sample frame
    int duration;    // in audio sample frames
    int midiPitch;   // 0-127
    float frequency; // Hz, to be used if isMidiPitchQuantized false
    bool isMidiPitchQuantized;
    int velocity;    // MIDI-style 0-127
    int channel;     // MIDI 0-15

    float getFrequency() const {
        if (isMidiPitchQuantized) {
            return Pitch::getFrequencyForPitch(midiPitch);
        } else {
            return frequency;
        }
    }
};

typedef std::vector<NoteData> NoteList;

class NoteExportable
{
public:
    virtual NoteList getNotes() const = 0;
    virtual NoteList getNotesWithin(int startFrame, int endFrame) const = 0;
};

#endif
