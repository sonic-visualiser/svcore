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


MultiSelection::MultiSelection()
{
}

MultiSelection::~MultiSelection()
{
}

const MultiSelection::SelectionList &
MultiSelection::getSelections() const
{
    return m_selections;
}

void
MultiSelection::setSelection(const Selection &selection)
{
    clearSelections();
    addSelection(selection);
}

void
MultiSelection::addSelection(const Selection &selection)
{
    m_selections.insert(selection);

    // Cope with a sitation where the new selection overlaps one or
    // more existing ones.  This is a terribly inefficient way to do
    // this, but that probably isn't significant in real life.

    // It's essential for the correct operation of
    // getContainingSelection that the selections do not overlap, so
    // this is not just a frill.

    for (SelectionList::iterator i = m_selections.begin();
	 i != m_selections.end(); ) {
	
	SelectionList::iterator j = i;
	if (++j == m_selections.end()) break;

	if (i->getEndFrame() >= j->getStartFrame()) {
	    Selection merged(i->getStartFrame(),
			     std::max(i->getEndFrame(), j->getEndFrame()));
	    m_selections.erase(i);
	    m_selections.erase(j);
	    m_selections.insert(merged);
	    i = m_selections.begin();
	} else {
	    ++i;
	}
    }
}

void
MultiSelection::removeSelection(const Selection &selection)
{
    //!!! Likewise this needs to cope correctly with the situation
    //where selection is not one of the original selection set but
    //simply overlaps one of them (cutting down the original selection
    //appropriately)

    if (m_selections.find(selection) != m_selections.end()) {
	m_selections.erase(selection);
    }
}

void
MultiSelection::clearSelections()
{
    if (!m_selections.empty()) {
	m_selections.clear();
    }
}

Selection
MultiSelection::getContainingSelection(size_t frame, bool defaultToFollowing) const
{
    // This scales very badly with the number of selections, but it's
    // more efficient for very small numbers of selections than a more
    // scalable method, and I think that may be what we need

    for (SelectionList::const_iterator i = m_selections.begin();
	 i != m_selections.end(); ++i) {

	if (i->contains(frame)) return *i;

	if (i->getStartFrame() > frame) {
	    if (defaultToFollowing) return *i;
	    else return Selection();
	}
    }

    return Selection();
}
