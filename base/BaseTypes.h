/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_BASE_TYPES_H
#define SV_BASE_TYPES_H

#include <cstdint>

/** Frame index, the unit of our time axis. This is signed because the
    axis conceptually extends below zero: zero represents the start of
    the main loaded audio model, not the start of time; a windowed
    transform could legitimately produce results before then. We also
    use this for frame counts, simply to avoid error-prone arithmetic
    between signed and unsigned types.
*/
typedef int64_t sv_frame_t;

/** Check whether an integer index is in range for a container,
    avoiding overflows and signed/unsigned comparison warnings.
*/
template<typename T, typename C>
bool in_range_for(const C &container, T i)
{
    if (i < 0) return false;
    if (sizeof(T) > sizeof(typename C::size_type)) {
	return i < static_cast<T>(container.size());
    } else {
	return static_cast<typename C::size_type>(i) < container.size();
    }
}

/** Sample rate. We have to deal with sample rates provided as float
    or (unsigned) int types, so we might as well have a type that can
    represent both. Storage size isn't an issue anyway.
*/
typedef double sv_samplerate_t;


/** Display zoom level. Can be an integer number of samples per pixel,
 *  or an integer number of pixels per sample.
 */
struct ZoomLevel {

    enum Zone {
        FramesPerPixel, // zoomed out (as in classic SV)
        PixelsPerFrame  // zoomed in beyond 1-1 (interpolating the waveform)
    };
    Zone zone;
    int level;

    bool operator<(const ZoomLevel &other) const {
        if (zone == FramesPerPixel) {
            if (other.zone == zone) {
                return level < other.level;
            } else {
                return false;
            }
        } else {
            if (other.zone == zone) {
                return level > other.level;
            } else {
                return false;
            }
        }
    }

    ZoomLevel incremented() const {
        if (zone == FramesPerPixel) {
            return { zone, level + 1 };
        } else if (level == 1) {
            return { FramesPerPixel, 2 };
        } else if (level == 2) {
            return { FramesPerPixel, 1 };
        } else {
            return { zone, level - 1 };
        }
    }

    ZoomLevel decremented() const {
        if (zone == PixelsPerFrame) {
            return { zone, level + 1 };
        } else if (level == 1) {
            return { PixelsPerFrame, 2 };
        } else {
            return { zone, level - 1 };
        }
    }
};

#endif

