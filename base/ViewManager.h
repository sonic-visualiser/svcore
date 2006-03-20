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

#include "Selection.h"
#include "Command.h"

class AudioPlaySource;
class Model;

/**
 * The ViewManager manages properties that may need to be synchronised
 * between separate Views.  For example, it handles signals associated
 * with changes to the global pan and zoom, and it handles selections.
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

    bool isPlaying() const;

    unsigned long getGlobalCentreFrame() const;
    unsigned long getGlobalZoom() const;

    unsigned long getPlaybackFrame() const;
    void setPlaybackFrame(unsigned long frame);

    bool haveInProgressSelection() const;
    const Selection &getInProgressSelection(bool &exclusive) const;
    void setInProgressSelection(const Selection &selection, bool exclusive);
    void clearInProgressSelection();

    const MultiSelection &getSelection() const;

    const MultiSelection::SelectionList &getSelections() const;
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

    enum ToolMode {
	NavigateMode,
	SelectMode,
        EditMode,
	DrawMode
    };
    ToolMode getToolMode() const { return m_toolMode; }
    void setToolMode(ToolMode mode);

    bool getPlayLoopMode() const { return m_playLoopMode; }
    void setPlayLoopMode(bool on);

    bool getPlaySelectionMode() const { return m_playSelectionMode; }
    void setPlaySelectionMode(bool on);

    size_t getPlaybackSampleRate() const;
    size_t getMainModelSampleRate() const { return m_mainModelSampleRate; }
    void setMainModelSampleRate(size_t sr) { m_mainModelSampleRate = sr; }

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

    /** Emitted when the in-progress (rubberbanding) selection has changed. */
    void inProgressSelectionChanged();

    /** Emitted when the tool mode has been changed. */
    void toolModeChanged();

    /** Emitted when the play loop mode has been changed. */
    void playLoopModeChanged();

    /** Emitted when the play selection mode has been changed. */
    void playSelectionModeChanged();

protected slots:
    void checkPlayStatus();
    void playStatusChanged(bool playing);
    void considerSeek(void *, unsigned long, bool);
    void considerZoomChange(void *, unsigned long, bool);

protected:
    AudioPlaySource *m_playSource;
    unsigned long m_globalCentreFrame;
    unsigned long m_globalZoom;
    mutable unsigned long m_playbackFrame;
    size_t m_mainModelSampleRate;

    float m_lastLeft;
    float m_lastRight;

    MultiSelection m_selections;
    Selection m_inProgressSelection;
    bool m_inProgressExclusive;

    ToolMode m_toolMode;

    bool m_playLoopMode;
    bool m_playSelectionMode;

    void setSelections(const MultiSelection &ms);
    void signalSelectionChange();

    class SetSelectionCommand : public Command
    {
    public:
	SetSelectionCommand(ViewManager *vm, const MultiSelection &ms);
	virtual ~SetSelectionCommand();
	virtual void execute();
	virtual void unexecute();
	virtual QString getName() const;

    protected:
	ViewManager *m_vm;
	MultiSelection m_oldSelection;
	MultiSelection m_newSelection;
    };
};

#endif

