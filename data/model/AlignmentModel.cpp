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

#include "AlignmentModel.h"

#include "SparseTimeValueModel.h"

AlignmentModel::AlignmentModel(Model *reference,
                               Model *aligned,
                               Model *inputModel,
			       SparseTimeValueModel *path) :
    m_reference(reference),
    m_aligned(aligned),
    m_inputModel(inputModel),
    m_path(path),
    m_reversePath(0),
    m_pathComplete(false)
{
    connect(m_path, SIGNAL(modelChanged()),
            this, SLOT(pathChanged()));

    connect(m_path, SIGNAL(modelChanged(size_t, size_t)),
            this, SLOT(pathChanged(size_t, size_t)));

    connect(m_path, SIGNAL(completionChanged()),
            this, SLOT(pathCompletionChanged()));

    constructReversePath();
}

AlignmentModel::~AlignmentModel()
{
    delete m_inputModel;
    delete m_path;
    delete m_reversePath;
}

bool
AlignmentModel::isOK() const
{
    return m_path->isOK();
}

size_t
AlignmentModel::getStartFrame() const
{
    //!!! do we care about distinct rates?
    size_t a = m_reference->getStartFrame();
    size_t b = m_aligned->getStartFrame();
    return std::min(a, b);
}

size_t
AlignmentModel::getEndFrame() const
{
    //!!! do we care about distinct rates?
    size_t a = m_reference->getEndFrame();
    size_t b = m_aligned->getEndFrame();
    return std::max(a, b);
}

size_t
AlignmentModel::getSampleRate() const
{
    return m_reference->getSampleRate();
}

Model *
AlignmentModel::clone() const
{
    return new AlignmentModel
        (m_reference, m_aligned,
         m_inputModel ? m_inputModel->clone() : 0,
         m_path ? static_cast<SparseTimeValueModel *>(m_path->clone()) : 0);
}

bool
AlignmentModel::isReady(int *completion) const
{
    return m_path->isReady(completion);
}

const ZoomConstraint *
AlignmentModel::getZoomConstraint() const
{
    return m_path->getZoomConstraint();
}

const Model *
AlignmentModel::getReferenceModel() const
{
    return m_reference;
}

const Model *
AlignmentModel::getAlignedModel() const
{
    return m_aligned;
}

size_t
AlignmentModel::toReference(size_t frame) const
{
//    std::cerr << "AlignmentModel::toReference(" << frame << ")" << std::endl;
    if (!m_reversePath) constructReversePath();
    return align(m_reversePath, frame);
}

size_t
AlignmentModel::fromReference(size_t frame) const
{
//    std::cerr << "AlignmentModel::fromReference(" << frame << ")" << std::endl;
    return align(m_path, frame);
}

void
AlignmentModel::pathChanged()
{
}

void
AlignmentModel::pathChanged(size_t, size_t)
{
    if (!m_pathComplete) return;
    constructReversePath();
}    

void
AlignmentModel::pathCompletionChanged()
{
    if (!m_pathComplete) {
        int completion = 0;
        m_path->isReady(&completion);
        std::cerr << "AlignmentModel::pathCompletionChanged: completion = "
                  << completion << std::endl;
        m_pathComplete = (completion == 100); //!!! a bit of a hack
        if (m_pathComplete) {
            constructReversePath();
            delete m_inputModel;
            m_inputModel = 0;
        }
    }
    emit completionChanged();
}

void
AlignmentModel::constructReversePath() const
{
    if (!m_reversePath) {
        m_reversePath = new SparseTimeValueModel
            (m_path->getSampleRate(), m_path->getResolution(), false);
    }
        
    m_reversePath->clear();

    SparseTimeValueModel::PointList points = m_path->getPoints();
        
    for (SparseTimeValueModel::PointList::const_iterator i = points.begin();
         i != points.end(); ++i) {
        long frame = i->frame;
        float value = i->value;
        long rframe = lrintf(value * m_aligned->getSampleRate());
        float rvalue = (float)frame / (float)m_reference->getSampleRate();
        m_reversePath->addPoint
            (SparseTimeValueModel::Point(rframe, rvalue, ""));
    }

    std::cerr << "AlignmentModel::constructReversePath: " << m_reversePath->getPointCount() << " points, at least " << (2 * m_reversePath->getPointCount() * (3 * sizeof(void *) + sizeof(int) + sizeof(SparseTimeValueModel::Point))) << " bytes" << std::endl;
}

size_t
AlignmentModel::align(SparseTimeValueModel *path, size_t frame) const
{
    // The path consists of a series of points, each with x (time)
    // equal to the time on the source model and y (value) equal to
    // the time on the target model.  Times and values are both
    // monotonically increasing.

    const SparseTimeValueModel::PointList &points = path->getPoints();

    if (points.empty()) {
//        std::cerr << "AlignmentModel::align: No points" << std::endl;
        return frame;
    }        

    SparseTimeValueModel::Point point(frame);
    SparseTimeValueModel::PointList::const_iterator i = points.lower_bound(point);
    if (i == points.end()) --i;
    while (i != points.begin() && i->frame > frame) --i;

    long foundFrame = i->frame;
    float foundTime = i->value;

    long followingFrame = foundFrame;
    float followingTime = foundTime;

    if (++i != points.end()) {
        followingFrame = i->frame;
        followingTime = i->value;
    }

    float resultTime = foundTime;

    if (followingFrame != foundFrame && frame > foundFrame) {

        std::cerr << "AlignmentModel::align: foundFrame = " << foundFrame << ", frame = " << frame << ", followingFrame = " << followingFrame << std::endl;

        float interp = float(frame - foundFrame) / float(followingFrame - foundFrame);
        std::cerr << "AlignmentModel::align: interp = " << interp << ", result " << resultTime << " -> ";

        resultTime += (followingTime - foundTime) * interp;

        std::cerr << resultTime << std::endl;
    }

    size_t resultFrame = lrintf(resultTime * getSampleRate());

    std::cerr << "AlignmentModel::align: resultFrame = " << resultFrame << std::endl;

    return resultFrame;
}
    
