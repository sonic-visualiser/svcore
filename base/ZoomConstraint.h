/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005
    
    This is experimental software.  Not for distribution.
*/

#ifndef _ZOOM_CONSTRAINT_H_
#define _ZOOM_CONSTRAINT_H_

#include <stdlib.h>

/**
 * ZoomConstraint is a simple interface that describes a limitation on
 * the available zoom sizes for a view, for example based on cache
 * strategy or a (processing) window-size limitation.
 *
 * The default ZoomConstraint imposes no actual constraint.
 */

class ZoomConstraint
{
public:
    enum RoundingDirection {
	RoundDown,
	RoundUp,
	RoundNearest
    };

    /**
     * Given the "ideal" block size (frames per pixel) for a given
     * zoom level, return the nearest viable block size for this
     * constraint.
     *
     * For example, if a block size of 1523 frames per pixel is
     * requested but the underlying model only supports value
     * summaries at powers-of-two block sizes, return 1024 or 2048
     * depending on the rounding direction supplied.
     */
    virtual size_t getNearestBlockSize(size_t requestedBlockSize,
				       RoundingDirection = RoundNearest)
	const
    {
	return requestedBlockSize;
    }
};

#endif

