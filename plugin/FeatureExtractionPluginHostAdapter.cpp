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

#include "FeatureExtractionPluginHostAdapter.h"

FeatureExtractionPluginHostAdapter::FeatureExtractionPluginHostAdapter(const SVPPluginDescriptor *descriptor,
                                                                       float inputSampleRate) :
    FeatureExtractionPlugin(inputSampleRate),
    m_descriptor(descriptor)
{
    m_handle = m_descriptor->instantiate(m_descriptor, inputSampleRate);
}

FeatureExtractionPluginHostAdapter::~FeatureExtractionPluginHostAdapter()
{
    if (m_handle) m_descriptor->cleanup(m_handle);
}

bool
FeatureExtractionPluginHostAdapter::initialise(size_t channels,
                                               size_t stepSize,
                                               size_t blockSize)
{
    if (!m_handle) return false;
    return m_descriptor->initialise(m_handle, channels, stepSize, blockSize) ?
        true : false;
}

void
FeatureExtractionPluginHostAdapter::reset()
{
    if (!m_handle) return;
    m_descriptor->reset(m_handle);
}

std::string
FeatureExtractionPluginHostAdapter::getName() const
{
    return m_descriptor->name;
}

std::string
FeatureExtractionPluginHostAdapter::getDescription() const
{
    return m_descriptor->description;
}

std::string
FeatureExtractionPluginHostAdapter::getMaker() const
{
    return m_descriptor->maker;
}

int
FeatureExtractionPluginHostAdapter::getPluginVersion() const
{
    return m_descriptor->pluginVersion;
}

std::string
FeatureExtractionPluginHostAdapter::getCopyright() const
{
    return m_descriptor->copyright;
}

FeatureExtractionPluginHostAdapter::ParameterList
FeatureExtractionPluginHostAdapter::getParameterDescriptors() const
{
    //!!!
    return ParameterList();
}

float
FeatureExtractionPluginHostAdapter::getParameter(std::string param) const
{
    //!!!
    return 0.0;
}

void
FeatureExtractionPluginHostAdapter::setParameter(std::string param, 
                                                 float value)
{
    //!!!
}

FeatureExtractionPluginHostAdapter::ProgramList
FeatureExtractionPluginHostAdapter::getPrograms() const
{
    //!!!
    return ProgramList();
}

std::string
FeatureExtractionPluginHostAdapter::getCurrentProgram() const
{
    //!!!
    return "";
}

void
FeatureExtractionPluginHostAdapter::selectProgram(std::string program)
{
    //!!!
}

size_t
FeatureExtractionPluginHostAdapter::getPreferredStepSize() const
{
    if (!m_handle) return 0;
    return m_descriptor->getPreferredStepSize(m_handle);
}

size_t
FeatureExtractionPluginHostAdapter::getPreferredBlockSize() const
{
    if (!m_handle) return 0;
    return m_descriptor->getPreferredBlockSize(m_handle);
}

FeatureExtractionPluginHostAdapter::OutputList
FeatureExtractionPluginHostAdapter::getOutputDescriptors() const
{
    //!!!
    return OutputList();
}

FeatureExtractionPluginHostAdapter::FeatureSet
FeatureExtractionPluginHostAdapter::process(float **inputBuffers,
                                            RealTime timestamp)
{
    //!!!
    return FeatureSet();
}

FeatureExtractionPluginHostAdapter::FeatureSet
FeatureExtractionPluginHostAdapter::getRemainingFeatures()
{
    //!!!
    return FeatureSet();
}

