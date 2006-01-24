/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "Selection.h"

Selection::Selection() :
    m_startFrame(0),
    m_endFrame(0)
{
}

Selection::Selection(size_t startFrame, size_t endFrame) :
    m_startFrame(startFrame),
    m_endFrame(endFrame)
{
    if (m_startFrame > m_endFrame) {
	size_t tmp = m_endFrame;
	m_endFrame = m_startFrame;
	m_startFrame = tmp;
    }
}

Selection::Selection(const Selection &s) :
    m_startFrame(s.m_startFrame),
    m_endFrame(s.m_endFrame)
{
}

Selection &
Selection::operator=(const Selection &s)
{
    if (this != &s) {
	m_startFrame = s.m_startFrame;
	m_endFrame = s.m_endFrame;
    } 
    return *this;
}

Selection::~Selection()
{
}

bool
Selection::isEmpty() const
{
    return m_startFrame == m_endFrame;
}

size_t
Selection::getStartFrame() const
{
    return m_startFrame;
}

size_t
Selection::getEndFrame() const
{
    return m_endFrame;
}

bool
Selection::contains(size_t frame) const
{
    return (frame >= m_startFrame) && (frame < m_endFrame);
}

bool
Selection::operator<(const Selection &s) const
{
    if (isEmpty()) {
	if (s.isEmpty()) return false;
	else return true;
    } else {
	if (s.isEmpty()) return false;
	else return (m_startFrame < s.m_startFrame);
    }
}

bool
Selection::operator==(const Selection &s) const
{
    if (isEmpty()) return s.isEmpty();

    return (m_startFrame == s.m_startFrame &&
	    m_endFrame == s.m_endFrame);
}

