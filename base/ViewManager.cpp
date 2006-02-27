/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "ViewManager.h"
#include "AudioPlaySource.h"
#include "Model.h"

#include <iostream>

//#define DEBUG_VIEW_MANAGER 1

ViewManager::ViewManager() :
    m_playSource(0),
    m_globalCentreFrame(0),
    m_globalZoom(1024),
    m_playbackFrame(0),
    m_lastLeft(0), 
    m_lastRight(0),
    m_inProgressExclusive(true),
    m_toolMode(NavigateMode),
    m_playLoopMode(false),
    m_playSelectionMode(false)
{
    connect(this, 
	    SIGNAL(centreFrameChanged(void *, unsigned long, bool)),
	    SLOT(considerSeek(void *, unsigned long, bool)));

    connect(this, 
	    SIGNAL(zoomLevelChanged(void *, unsigned long, bool)),
	    SLOT(considerZoomChange(void *, unsigned long, bool)));
}

unsigned long
ViewManager::getGlobalCentreFrame() const
{
#ifdef DEBUG_VIEW_MANAGER
    std::cout << "ViewManager::getGlobalCentreFrame: returning " << m_globalCentreFrame << std::endl;
#endif
    return m_globalCentreFrame;
}

unsigned long
ViewManager::getGlobalZoom() const
{
#ifdef DEBUG_VIEW_MANAGER
    std::cout << "ViewManager::getGlobalZoom: returning " << m_globalZoom << std::endl;
#endif
    return m_globalZoom;
}

unsigned long
ViewManager::getPlaybackFrame() const
{
    if (m_playSource && m_playSource->isPlaying()) {
	m_playbackFrame = m_playSource->getCurrentPlayingFrame();
    }
    return m_playbackFrame;
}

void
ViewManager::setPlaybackFrame(unsigned long f)
{
    if (m_playbackFrame != f) {
	m_playbackFrame = f;
	if (m_playSource && m_playSource->isPlaying()) {
	    m_playSource->play(f);
	}
	emit playbackFrameChanged(f);
    }
}

bool
ViewManager::haveInProgressSelection() const
{
    return !m_inProgressSelection.isEmpty();
}

const Selection &
ViewManager::getInProgressSelection(bool &exclusive) const
{
    exclusive = m_inProgressExclusive;
    return m_inProgressSelection;
}

void
ViewManager::setInProgressSelection(const Selection &selection, bool exclusive)
{
    m_inProgressExclusive = exclusive;
    m_inProgressSelection = selection;
    if (exclusive) clearSelections();
    emit inProgressSelectionChanged();
}

void
ViewManager::clearInProgressSelection()
{
    m_inProgressSelection = Selection();
    emit inProgressSelectionChanged();
}

const MultiSelection &
ViewManager::getSelection() const
{
    return m_selections;
}

const MultiSelection::SelectionList &
ViewManager::getSelections() const
{
    return m_selections.getSelections();
}

void
ViewManager::setSelection(const Selection &selection)
{
    m_selections.setSelection(selection);
    emit selectionChanged();
}

void
ViewManager::addSelection(const Selection &selection)
{
    m_selections.addSelection(selection);
    emit selectionChanged();
}

void
ViewManager::removeSelection(const Selection &selection)
{
    m_selections.removeSelection(selection);
    emit selectionChanged();
}

void
ViewManager::clearSelections()
{
    m_selections.clearSelections();
    emit selectionChanged();
}

Selection
ViewManager::getContainingSelection(size_t frame, bool defaultToFollowing)
{
    return m_selections.getContainingSelection(frame, defaultToFollowing);
}

void
ViewManager::setToolMode(ToolMode mode)
{
    m_toolMode = mode;

    emit toolModeChanged();
}

void
ViewManager::setPlayLoopMode(bool mode)
{
    m_playLoopMode = mode;

    emit playLoopModeChanged();
}

void
ViewManager::setPlaySelectionMode(bool mode)
{
    m_playSelectionMode = mode;

    emit playSelectionModeChanged();
}

void
ViewManager::setAudioPlaySource(AudioPlaySource *source)
{
    if (!m_playSource) {
	QTimer::singleShot(100, this, SLOT(checkPlayStatus()));
    }
    m_playSource = source;
}

void
ViewManager::playStatusChanged(bool playing)
{
    checkPlayStatus();
}

void
ViewManager::checkPlayStatus()
{
    if (m_playSource && m_playSource->isPlaying()) {

	float left = 0, right = 0;
	if (m_playSource->getOutputLevels(left, right)) {
	    if (left != m_lastLeft || right != m_lastRight) {
		emit outputLevelsChanged(left, right);
		m_lastLeft = left;
		m_lastRight = right;
	    }
	}

	m_playbackFrame = m_playSource->getCurrentPlayingFrame();

#ifdef DEBUG_VIEW_MANAGER
	std::cout << "ViewManager::checkPlayStatus: Playing, frame " << m_playbackFrame << ", levels " << m_lastLeft << "," << m_lastRight << std::endl;
#endif

	emit playbackFrameChanged(m_playbackFrame);

	QTimer::singleShot(20, this, SLOT(checkPlayStatus()));

    } else {

	QTimer::singleShot(100, this, SLOT(checkPlayStatus()));
	
	if (m_lastLeft != 0.0 || m_lastRight != 0.0) {
	    emit outputLevelsChanged(0.0, 0.0);
	    m_lastLeft = 0.0;
	    m_lastRight = 0.0;
	}

#ifdef DEBUG_VIEW_MANAGER
//	std::cout << "ViewManager::checkPlayStatus: Not playing" << std::endl;
#endif
    }
}

bool
ViewManager::isPlaying() const
{
    return m_playSource && m_playSource->isPlaying();
}

void
ViewManager::considerSeek(void *p, unsigned long f, bool locked)
{
    if (locked) {
	m_globalCentreFrame = f;
    }

#ifdef DEBUG_VIEW_MANAGER 
    std::cout << "ViewManager::considerSeek(" << p << ", " << f << ", " << locked << ")" << std::endl;
#endif

    if (p == this || !locked) return;

    if (m_playSource && m_playSource->isPlaying()) {
	unsigned long playFrame = m_playSource->getCurrentPlayingFrame();
	unsigned long diff = std::max(f, playFrame) - std::min(f, playFrame);
	if (diff > 20000) {
	    m_playbackFrame = f;
	    m_playSource->play(f);
#ifdef DEBUG_VIEW_MANAGER 
	    std::cout << "ViewManager::considerSeek: reseeking from " << playFrame << " to " << f << std::endl;
#endif
	}
    } else {
	m_playbackFrame = f; //!!! only if that view is in scroll mode?
    }
}

void
ViewManager::considerZoomChange(void *p, unsigned long z, bool locked)
{
    if (locked) {
	m_globalZoom = z;
    }

#ifdef DEBUG_VIEW_MANAGER 
    std::cout << "ViewManager::considerZoomChange(" << p << ", " << z << ", " << locked << ")" << std::endl;
#endif
}

#ifdef INCLUDE_MOCFILES
#include "ViewManager.moc.cpp"
#endif

