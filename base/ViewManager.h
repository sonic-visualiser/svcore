/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _VIEW_MANAGER_H_
#define _VIEW_MANAGER_H_

#include <QObject>
#include <QTimer>

#include <map>
#include <set>

#include "Selection.h"

class AudioPlaySource;
class PlayParameters;
class Model;

/**
 * The ViewManager manages properties that may need to be synchronised
 * between separate Views.  For example, it handles signals associated
 * with changes to the global pan and zoom.  It also handles playback
 * properties and play synchronisation.
 *
 * Views should be implemented in such a way as to work
 * correctly whether they are supplied with a ViewManager or not.
 */

class ViewManager : public QObject
{
    Q_OBJECT

public:
    ViewManager();

    void setAudioPlaySource(AudioPlaySource *source);

//!!! No way to remove a model!
    PlayParameters *getPlayParameters(const Model *model);
    void clearPlayParameters();

    bool isPlaying() const;

    unsigned long getGlobalCentreFrame() const;
    unsigned long getGlobalZoom() const;

    typedef std::set<Selection> SelectionList;

    bool haveInProgressSelection() const;
    const Selection &getInProgressSelection(bool &exclusive) const;
    void setInProgressSelection(const Selection &selection, bool exclusive);
    void clearInProgressSelection();

    const SelectionList &getSelections() const;
    void setSelection(const Selection &selection);
    void addSelection(const Selection &selection);
    void removeSelection(const Selection &selection);
    void clearSelections();

    enum ToolMode {
	NavigateMode,
	SelectMode,
        EditMode,
	DrawMode,
	TextMode
    };
    ToolMode getToolMode() const { return m_toolMode; }
    void setToolMode(ToolMode mode);

signals:
    /** Emitted when a widget pans.  The originator identifies the widget. */
    void centreFrameChanged(void *originator, unsigned long frame, bool locked);

    /** Emitted when a widget zooms.  The originator identifies the widget. */
    void zoomLevelChanged(void *originator, unsigned long zoom, bool locked);

    /** Emitted when the playback frame changes. */
    void playbackFrameChanged(unsigned long frame);

    /** Emitted when the output levels change. Values in range 0.0 -> 1.0. */
    void outputLevelsChanged(float left, float right);

    /** Emitted when the selection has changed. */
    void selectionChanged();

    /** Emitted when the tool mode has been changed. */
    void toolModeChanged();

protected slots:
    void checkPlayStatus();
    void considerSeek(void *, unsigned long, bool);
    void considerZoomChange(void *, unsigned long, bool);

protected:
    AudioPlaySource *m_playSource;
    unsigned long m_globalCentreFrame;
    unsigned long m_globalZoom;

    float m_lastLeft;
    float m_lastRight;

    SelectionList m_selections;
    Selection m_inProgressSelection;
    bool m_inProgressExclusive;

    ToolMode m_toolMode;

    std::map<const Model *, PlayParameters *> m_playParameters;
};

#endif

