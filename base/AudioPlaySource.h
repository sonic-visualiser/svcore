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

#ifndef SV_AUDIO_PLAY_SOURCE_H
#define SV_AUDIO_PLAY_SOURCE_H

#include "BaseTypes.h"

#include <memory>

namespace sv {

struct Auditionable {
    virtual ~Auditionable() { }
};

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
    virtual void play(sv_frame_t startFrame) = 0;

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
    virtual sv_frame_t getCurrentPlayingFrame() = 0;

    /**
     * Return the current (or thereabouts) output levels in the range
     * 0.0 -> 1.0, for metering purposes.  The values returned are
     * peak values since the last call to this function was made
     * (i.e. calling this function also resets them).
     */
    virtual bool getOutputLevels(float &left, float &right) = 0;

    /**
     * Return the sample rate of the source material -- any material
     * that wants to play at a different rate will sound wrong.
     */
    virtual sv_samplerate_t getSourceSampleRate() const = 0;

    /**
     * Return the sample rate set by the target audio device (or 0 if
     * the target hasn't told us yet).  If the source and target
     * sample rates differ, resampling will occur.
     *
     * Note that we don't actually do any processing at the device
     * sample rate. All processing happens at the source sample rate,
     * and then a resampler is applied if necessary at the interface
     * between application and driver layer.
     */
    virtual sv_samplerate_t getDeviceSampleRate() const = 0;

    /**
     * Get the block size of the target audio device.  This may be an
     * estimate or upper bound, if the target has a variable block
     * size; the source should behave itself even if this value turns
     * out to be inaccurate.
     */
    virtual int getTargetBlockSize() const = 0;

    /**
     * Get the number of channels of audio that will be provided
     * to the play target.  This may be more than the source channel
     * count: for example, a mono source will provide 2 channels
     * after pan.
     */
    virtual int getTargetChannelCount() const = 0;

    /**
     * Set a plugin or other subclass of Auditionable as an
     * auditioning effect. The Auditionable is shared with the caller:
     * the expectation is that the caller may continue to modify its
     * parameters etc during auditioning.
     */
    virtual void setAuditioningEffect(std::shared_ptr<Auditionable>) = 0;

};

} // end namespace sv

#endif
