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

#ifndef _BEAT_DETECTION_FUNCTION_TRANSFORM_H_
#define _BEAT_DETECTION_FUNCTION_TRANSFORM_H_

#include "Transform.h"

//!!! This should be replaced by a plugin, when we have a plugin
// transform.  But it's easier to start by testing concrete examples.

class DenseTimeValueModel;
class SparseTimeValueModel;

class BeatDetectionFunctionTransform : public Transform
{
public:
    BeatDetectionFunctionTransform(Model *inputModel);
    virtual ~BeatDetectionFunctionTransform();

    static TransformName getName();

protected:
    virtual void run();

    // just casts
    DenseTimeValueModel *getInput();
    SparseTimeValueModel *getOutput();
};

#endif

