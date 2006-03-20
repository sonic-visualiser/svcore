/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
   
    This is experimental software.  Not for distribution.
*/

#ifndef _BEAT_DETECT_TRANSFORM_H_
#define _BEAT_DETECT_TRANSFORM_H_

#include "Transform.h"

//!!! This should be replaced by a plugin, when we have a plugin
// transform.  But it's easier to start by testing concrete examples.

class DenseTimeValueModel;
class SparseOneDimensionalModel;

class BeatDetectTransform : public Transform
{
public:
    BeatDetectTransform(Model *inputModel);
    virtual ~BeatDetectTransform();

    static TransformName getName();

protected:
    virtual void run();

    // just casts
    DenseTimeValueModel *getInput();
    SparseOneDimensionalModel *getOutput();
};

#endif

