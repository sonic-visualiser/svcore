/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _SELECTION_H_
#define _SELECTION_H_

#include <cstddef>
#include <set>

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
    bool contains(size_t frame) const;

    bool operator<(const Selection &) const;
    bool operator==(const Selection &) const;
    
protected:
    size_t m_startFrame;
    size_t m_endFrame;
};

class MultiSelection
{
public:
    MultiSelection();
    virtual ~MultiSelection();

    typedef std::set<Selection> SelectionList;

    const SelectionList &getSelections() const;
    void setSelection(const Selection &selection);
    void addSelection(const Selection &selection);
    void removeSelection(const Selection &selection);
    void clearSelections();

    /**
     * Return the selection that contains a given frame.
     * If defaultToFollowing is true, and if the frame is not in a
     * selected area, return the next selection after the given frame.
     * Return the empty selection if no appropriate selection is found.
     */
    Selection getContainingSelection(size_t frame, bool defaultToFollowing) const;

protected:
    SelectionList m_selections;
};
    

#endif
