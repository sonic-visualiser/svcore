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

#include "ModelTransformer.h"

#include "TransformFactory.h"

namespace sv {

ModelTransformer::ModelTransformer(Input input,
                                   const Transform &transform,
                                   CompletionReporter *reporter) :
    m_input(input),
    m_reporter(reporter),
    m_abandoned(false)
{
    m_transforms.push_back(transform);
    checkTransformsExist();
}

ModelTransformer::ModelTransformer(Input input,
                                   const Transforms &transforms,
                                   CompletionReporter *reporter) :
    m_transforms(transforms),
    m_input(input),
    m_reporter(reporter),
    m_abandoned(false)
{
    checkTransformsExist();
}

ModelTransformer::~ModelTransformer()
{
    m_abandoned = true;
    wait();
}

void
ModelTransformer::checkTransformsExist()
{
    // This is partly for diagnostic purposes, but also to cause the
    // TransformFactory to resolve any pending scan/load process
    // before we continue into running a transform
    TransformFactory *tf = TransformFactory::getInstance();
    for (auto t: m_transforms) {
        if (!tf->haveTransform(t.getIdentifier())) {
            SVCERR << "WARNING: ModelTransformer::checkTransformsExist: Unknown transform \"" << t.getIdentifier() << "\"" << endl;
        }
    }
}
} // end namespace sv

