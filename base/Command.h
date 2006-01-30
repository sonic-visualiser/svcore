/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _COMMAND_H_
#define _COMMAND_H_

class Command
{
public:
    virtual void execute() = 0;
    virtual void unexecute() = 0;
    virtual QString name() const = 0;
};

#endif

