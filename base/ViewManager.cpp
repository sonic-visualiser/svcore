/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ViewManager.h"
#include "AudioPlaySource.h"
#include "Model.h"
#include "CommandHistory.h"

#include <iostream>

#include <QDir>
#include <QFile>

#include <cassert>

// #define DEBUG_VIEW_MANAGER 1

ViewManager::ViewManager() :
    m_playSource(0),
    m_globalCentreFrame(0),
    m_globalZoom(1024),
    m_playbackFrame(0),
    m_mainModelSampleRate(0),
    m_lastLeft(0), 
    m_lastRight(0),
    m_inProgressExclusive(true),
    m_toolMode(NavigateMode),
    m_playLoopMode(false),
    m_playSelectionMode(false),
    m_overlayMode(BasicOverlays)
{
    connect(this, 
	    SIGNAL(centreFrameChanged(void *, unsigned long, bool)),
	    SLOT(considerSeek(void *, unsigned long, bool)));

    connect(this, 
	    SIGNAL(zoomLevelChanged(void *, unsigned long, bool)),
	    SLOT(considerZoomChange(void *, unsigned long, bool)));
}

ViewManager::~ViewManager()
{
    if (m_tmpdir != "") deleteTemporaryDirectory(m_tmpdir);
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
	emit playbackFrameChanged(f);
	if (m_playSource && m_playSource->isPlaying()) {
	    m_playSource->play(f);
	}
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
    MultiSelection ms(m_selections);
    ms.setSelection(selection);
    setSelections(ms);
}

void
ViewManager::addSelection(const Selection &selection)
{
    MultiSelection ms(m_selections);
    ms.addSelection(selection);
    setSelections(ms);
}

void
ViewManager::removeSelection(const Selection &selection)
{
    MultiSelection ms(m_selections);
    ms.removeSelection(selection);
    setSelections(ms);
}

void
ViewManager::clearSelections()
{
    MultiSelection ms(m_selections);
    ms.clearSelections();
    setSelections(ms);
}

void
ViewManager::setSelections(const MultiSelection &ms)
{
    if (m_selections.getSelections() == ms.getSelections()) return;
    SetSelectionCommand *command = new SetSelectionCommand(this, ms);
    CommandHistory::getInstance()->addCommand(command);
}

void
ViewManager::signalSelectionChange()
{
    emit selectionChanged();
}

ViewManager::SetSelectionCommand::SetSelectionCommand(ViewManager *vm,
						      const MultiSelection &ms) :
    m_vm(vm),
    m_oldSelection(vm->m_selections),
    m_newSelection(ms)
{
}

ViewManager::SetSelectionCommand::~SetSelectionCommand() { }

void
ViewManager::SetSelectionCommand::execute()
{
    m_vm->m_selections = m_newSelection;
    m_vm->signalSelectionChange();
}

void
ViewManager::SetSelectionCommand::unexecute()
{
    m_vm->m_selections = m_oldSelection;
    m_vm->signalSelectionChange();
}

QString
ViewManager::SetSelectionCommand::getName() const
{
    if (m_newSelection.getSelections().empty()) return tr("Clear Selection");
    else return tr("Select");
}

Selection
ViewManager::getContainingSelection(size_t frame, bool defaultToFollowing) const
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

size_t
ViewManager::getPlaybackSampleRate() const
{
    if (m_playSource) {
	return m_playSource->getTargetSampleRate();
    }
    return 0;
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

void
ViewManager::setOverlayMode(OverlayMode mode)
{
    if (m_overlayMode != mode) {
        m_overlayMode = mode;
        emit overlayModeChanged();
    }
}

QString
ViewManager::getTemporaryDirectory()
{
    if (m_tmpdir != "") return m_tmpdir;

    // Generate a temporary directory.  Qt4.1 doesn't seem to be able
    // to do this for us, and mkdtemp is not standard.  This method is
    // based on the way glibc does mkdtemp.

    static QString chars =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    QString suffix;
    int padlen = 6, attempts = 100;
    unsigned int r = time(0) ^ getpid();

    for (int i = 0; i < padlen; ++i) {
        suffix += "X";
    }
    
    for (int j = 0; j < attempts; ++j) {

        unsigned int v = r;
        
        for (int i = 0; i < padlen; ++i) {
            suffix[i] = chars[v % 62];
            v /= 62;
        }

        QString candidate = QString("sv_%1").arg(suffix);

        if (QDir::temp().mkpath(candidate)) {
            m_tmpdir = QDir::temp().filePath(candidate);
            break;
        }

        r = r + 7777;
    }

    if (m_tmpdir == "") {
        std::cerr << "ERROR: ViewManager::getTemporaryDirectory: "
                  << "Unable to create a temporary directory!" << std::endl;
        assert(0);
    }

    return m_tmpdir;
}

void
ViewManager::deleteTemporaryDirectory(QString tmpdir)
{
    if (tmpdir == "") return;
    
    QDir dir(tmpdir);
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    for (unsigned int i = 0; i < dir.count(); ++i) {

        QFileInfo fi(dir.filePath(dir[i]));

        if (fi.isDir()) {
            deleteTemporaryDirectory(fi.absoluteFilePath());
        } else {
            if (!QFile(fi.absoluteFilePath()).remove()) {
                std::cerr << "WARNING: ViewManager::deleteTemporaryDirectory: "
                          << "Failed to unlink file \""
                          << fi.absoluteFilePath().toStdString() << "\""
                          << std::endl;
            }
        }
    }

    QString dirname = dir.dirName();
    if (dirname != "") {
        if (!dir.cdUp()) {
            std::cerr << "WARNING: ViewManager::deleteTemporaryDirectory: "
                      << "Failed to cd to parent directory of "
                      << tmpdir.toStdString() << std::endl;
            return;
        }
        if (!dir.rmdir(dirname)) {
            std::cerr << "WARNING: ViewManager::deleteTemporaryDirectory: "
                      << "Failed to remove directory "
                      << dirname.toStdString() << std::endl;
        } 
    }
}

#ifdef INCLUDE_MOCFILES
#include "ViewManager.moc.cpp"
#endif

