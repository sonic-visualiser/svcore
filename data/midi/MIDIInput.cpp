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

#include "MIDIInput.h"

#include "rtmidi/RtMidi.h"

MIDIInput::MIDIInput(QString name) :
    m_rtmidi(),
    m_buffer(1023)
{
    try {
        m_rtmidi = new RtMidiIn(name.toStdString());
        m_rtmidi->setCallback(staticCallback, this);
        m_rtmidi->openPort(0, tr("Input").toStdString());
    } catch (RtError e) {
        e.printMessage();
        delete m_rtmidi;
        m_rtmidi = 0;
    }
}

MIDIInput::~MIDIInput()
{
    delete m_rtmidi;
}

void
MIDIInput::staticCallback(double timestamp, std::vector<unsigned char> *message,
                          void *userData)
{
    ((MIDIInput *)userData)->callback(timestamp, message);
}

void
MIDIInput::callback(double timestamp, std::vector<unsigned char> *message)
{
    std::cerr << "MIDIInput::callback(" << timestamp << ")" << std::endl;
    unsigned long deltaTime = 0;
    if (timestamp > 0) deltaTime = (unsigned long)(timestamp * 100000); //!!! for now!
    if (!message || message->empty()) return;
    MIDIEvent ev(deltaTime,
                 (*message)[0],
                 message->size() > 1 ? (*message)[1] : 0,
                 message->size() > 2 ? (*message)[2] : 0);
    postEvent(ev);
}

MIDIEvent
MIDIInput::readEvent()
{
    MIDIEvent *event = m_buffer.readOne();
    MIDIEvent revent = *event;
    delete event;
    return revent;
}

void
MIDIInput::postEvent(MIDIEvent e)
{
    int count = 0, max = 5;
    while (m_buffer.getWriteSpace() == 0) {
        if (count == max) {
            std::cerr << "ERROR: MIDIInput::postEvent: MIDI event queue is full and not clearing -- abandoning incoming event" << std::endl;
            return;
        }
        std::cerr << "WARNING: MIDIInput::postEvent: MIDI event queue (capacity " << m_buffer.getSize() << " is full!" << std::endl;
        std::cerr << "Waiting for something to be processed" << std::endl;
#ifdef _WIN32
        Sleep(1);
#else
        sleep(1);
#endif
        count++;
    }

    MIDIEvent *me = new MIDIEvent(e);
    m_buffer.write(&me, 1);
    emit eventsAvailable();
}

