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

#include "RangeSummarisableTimeValueModel.h"

#include "AlignmentModel.h"

#include <iostream>

void
RangeSummarisableTimeValueModel::setAlignment(AlignmentModel *alignment)
{
    delete m_alignment;
    m_alignment = alignment;
    connect(m_alignment, SIGNAL(completionChanged()),
            this, SIGNAL(alignmentCompletionChanged()));
}

const Model *
RangeSummarisableTimeValueModel::getAlignmentReference() const
{
    if (!m_alignment) return 0;
    return m_alignment->getReferenceModel();
}

size_t
RangeSummarisableTimeValueModel::alignToReference(size_t frame) const
{
    if (!m_alignment) return frame;
    return m_alignment->toReference(frame);
}

size_t
RangeSummarisableTimeValueModel::alignFromReference(size_t refFrame) const
{
    if (!m_alignment) return refFrame;
    return m_alignment->fromReference(refFrame);
}

int
RangeSummarisableTimeValueModel::getAlignmentCompletion() const
{
    std::cerr << "RangeSummarisableTimeValueModel::getAlignmentCompletion" << std::endl;
    if (!m_alignment) return 100;
    int completion = 0;
    (void)m_alignment->isReady(&completion);
    std::cerr << " -> " << completion << std::endl;
    return completion;
}

