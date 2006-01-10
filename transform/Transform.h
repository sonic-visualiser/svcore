/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005
   
    This is experimental software.  Not for distribution.
*/

#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include <QThread>

#include "base/Model.h"

typedef QString TransformName;

/**
 * A Transform turns one data model into another.
 *
 * Typically in this application, a Transform might have a
 * DenseTimeValueModel as its input (e.g. an audio waveform) and a
 * SparseOneDimensionalModel (e.g. detected beats) as its output.
 *
 * The Transform typically runs in the background, as a separate
 * thread populating the output model.  The model is available to the
 * user of the Transform immediately, but may be initially empty until
 * the background thread has populated it.
 */

class Transform : public QThread
{
public:
    virtual ~Transform();

    Model *getInputModel()  { return m_input; }
    Model *getOutputModel() { return m_output; }
    Model *detachOutputModel() { m_detached = true; return m_output; }

protected:
    Transform(Model *m);

    Model *m_input; // I don't own this
    Model *m_output; // I own this, unless...
    bool m_detached; // ... this is true.
    bool m_deleting;
};

#endif
