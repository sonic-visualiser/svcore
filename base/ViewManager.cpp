/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005
    
    This is experimental software.  Not for distribution.
*/

#include "ViewManager.h"
#include "AudioPlaySource.h"
#include "PlayParameters.h"
#include "Model.h"

#include <iostream>

//#define DEBUG_VIEW_MANAGER 1

ViewManager::ViewManager() :
    m_playSource(0),
    m_globalCentreFrame(0),
    m_globalZoom(1024),
    m_lastLeft(0), 
    m_lastRight(0)
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

void
ViewManager::setAudioPlaySource(AudioPlaySource *source)
{
    if (!m_playSource) {
	QTimer::singleShot(100, this, SLOT(checkPlayStatus()));
    }
    m_playSource = source;
}

PlayParameters *
ViewManager::getPlayParameters(const Model *model)
{
    if (m_playParameters.find(model) == m_playParameters.end()) {
	// Give all models the same type of play parameters for the moment
	m_playParameters[model] = new PlayParameters;
    }

    return m_playParameters[model];
}

void
ViewManager::clearPlayParameters()
{
    while (!m_playParameters.empty()) {
	delete m_playParameters.begin()->second;
	m_playParameters.erase(m_playParameters.begin());
    }
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

	m_globalCentreFrame = m_playSource->getCurrentPlayingFrame();

#ifdef DEBUG_VIEW_MANAGER
	std::cout << "ViewManager::checkPlayStatus: Playing, frame " << m_globalCentreFrame << ", levels " << m_lastLeft << "," << m_lastRight << std::endl;
#endif

	emit playbackFrameChanged(m_globalCentreFrame);

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
	    m_playSource->play(f);
#ifdef DEBUG_VIEW_MANAGER 
	    std::cout << "ViewManager::considerSeek: reseeking from " << playFrame << " to " << f << std::endl;
#endif
	}
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

