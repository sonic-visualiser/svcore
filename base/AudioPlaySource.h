/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _AUDIO_PLAY_SOURCE_H_
#define _AUDIO_PLAY_SOURCE_H_

/**
 * Simple interface for audio playback.  This should be all that the
 * ViewManager needs to know about to synchronise with playback by
 * sample frame, but it doesn't provide enough to determine what is
 * actually being played or how.  See the audioio directory for a
 * concrete subclass.
 */

class AudioPlaySource
{
public:
    virtual ~AudioPlaySource() { }

    /**
     * Start playing from the given frame.  If playback is already
     * under way, reseek to the given frame and continue.
     */
    virtual void play(size_t startFrame) = 0;

    /**
     * Stop playback.
     */
    virtual void stop() = 0;

    /**
     * Return whether playback is currently supposed to be happening.
     */
    virtual bool isPlaying() const = 0;

    /**
     * Return the frame number that is currently expected to be coming
     * out of the speakers.  (i.e. compensating for playback latency.)
     */
    virtual size_t getCurrentPlayingFrame() = 0;

    /**
     * Return the current (or thereabouts) output levels in the range
     * 0.0 -> 1.0, for metering purposes.
     */
    virtual bool getOutputLevels(float &left, float &right) = 0;

    /**
     * Return the sample rate set by the target audio device (or the
     * source sample rate if the target hasn't set one).
     */
    virtual size_t getTargetSampleRate() const = 0;
};

#endif
