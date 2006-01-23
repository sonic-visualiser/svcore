/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _SELECTION_H_
#define _SELECTION_H_

#include <cstddef>

class Selection
{
public:
    Selection();
    Selection(size_t startFrame, size_t endFrame);
    Selection(const Selection &);
    Selection &operator=(const Selection &);
    virtual ~Selection();

    bool isEmpty() const;
    size_t getStartFrame() const;
    size_t getEndFrame() const;

    bool operator<(const Selection &) const;
    bool operator==(const Selection &) const;
    
protected:
    size_t m_startFrame;
    size_t m_endFrame;
};

#endif
