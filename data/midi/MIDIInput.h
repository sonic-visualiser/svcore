/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2009 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MIDI_INPUT_H_
#define _MIDI_INPUT_H_

#include <QObject>
#include "MIDIEvent.h"

#include <vector>
#include "base/RingBuffer.h"

class RtMidi;

class MIDIInput : public QObject
{
    Q_OBJECT

public:
    MIDIInput();
    virtual ~MIDIInput();

    bool isOK() const;

    bool isEmpty() const { return getEventsAvailable() == 0; }
    size_t getEventsAvailable() const;
    MIDIEvent readEvent();

signals:
    void eventsAvailable();

protected:
    RtMidi *m_rtmidi;

    static void callback(double, std::vector<unsigned char> *, void *);
    void callback(double, std::vector<unsigned char> *);

    void postEvent(MIDIEvent);
    RingBuffer<MIDIEvent *> m_buffer;
};

#endif

