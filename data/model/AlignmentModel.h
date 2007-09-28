/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2007 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _ALIGNMENT_MODEL_H_
#define _ALIGNMENT_MODEL_H_

#include "Model.h"

class SparseTimeValueModel;

class AlignmentModel : public Model
{
    Q_OBJECT

public:
    AlignmentModel(Model *reference,
                   Model *aligned,
                   Model *inputModel, // probably an AggregateWaveModel; I take ownership
                   SparseTimeValueModel *path); // I take ownership
    ~AlignmentModel();

    virtual bool isOK() const;
    virtual size_t getStartFrame() const;
    virtual size_t getEndFrame() const;
    virtual size_t getSampleRate() const;
    virtual Model *clone() const;
    virtual bool isReady(int *completion = 0) const;
    virtual const ZoomConstraint *getZoomConstraint() const;

    const Model *getReferenceModel() const;
    const Model *getAlignedModel() const;

    size_t toReference(size_t frame) const;
    size_t fromReference(size_t frame) const;

signals:
    void modelChanged();
    void modelChanged(size_t startFrame, size_t endFrame);
    void completionChanged();

protected slots:
    void pathChanged();
    void pathChanged(size_t startFrame, size_t endFrame);
    void pathCompletionChanged();

protected:
    Model *m_reference; // I don't own this
    Model *m_aligned; // I don't own this

    Model *m_inputModel; // I own this
    SparseTimeValueModel *m_path; // I own this
    mutable SparseTimeValueModel *m_reversePath; // I own this
    bool m_pathComplete;

    void constructReversePath() const;

    size_t align(SparseTimeValueModel *path, size_t frame) const;
};

#endif
