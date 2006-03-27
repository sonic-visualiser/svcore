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
    ParameterList list;
    for (unsigned int i = 0; i < m_descriptor->parameterCount; ++i) {
        const SVPParameterDescriptor *spd = m_descriptor->parameters[i];
        ParameterDescriptor pd;
        pd.name = spd->name;
        pd.description = spd->description;
        pd.unit = spd->unit;
        pd.minValue = spd->minValue;
        pd.maxValue = spd->maxValue;
        pd.defaultValue = spd->defaultValue;
        pd.isQuantized = spd->isQuantized;
        pd.quantizeStep = spd->quantizeStep;
        list.push_back(pd);
    }
    return list;
}

float
FeatureExtractionPluginHostAdapter::getParameter(std::string param) const
{
    if (!m_handle) return 0.0;

    for (unsigned int i = 0; i < m_descriptor->parameterCount; ++i) {
        if (param == m_descriptor->parameters[i]->name) {
            return m_descriptor->getParameter(m_handle, i);
        }
    }

    return 0.0;
}

void
FeatureExtractionPluginHostAdapter::setParameter(std::string param, 
                                                 float value)
{
    if (!m_handle) return;

    for (unsigned int i = 0; i < m_descriptor->parameterCount; ++i) {
        if (param == m_descriptor->parameters[i]->name) {
            m_descriptor->setParameter(m_handle, i, value);
            return;
        }
    }
}

FeatureExtractionPluginHostAdapter::ProgramList
FeatureExtractionPluginHostAdapter::getPrograms() const
{
    ProgramList list;
    
    for (unsigned int i = 0; i < m_descriptor->programCount; ++i) {
        list.push_back(m_descriptor->programs[i]);
    }
    
    return list;
}

std::string
FeatureExtractionPluginHostAdapter::getCurrentProgram() const
{
    if (!m_handle) return "";

    int pn = m_descriptor->getCurrentProgram(m_handle);
    return m_descriptor->programs[pn];
}

void
FeatureExtractionPluginHostAdapter::selectProgram(std::string program)
{
    if (!m_handle) return;

    for (unsigned int i = 0; i < m_descriptor->programCount; ++i) {
        if (program == m_descriptor->programs[i]) {
            m_descriptor->selectProgram(m_handle, i);
            return;
        }
    }
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
    OutputList list;
    if (!m_handle) return list;

    unsigned int count = m_descriptor->getOutputCount(m_handle);

    for (unsigned int i = 0; i < count; ++i) {
        SVPOutputDescriptor *sd = m_descriptor->getOutputDescriptor(m_handle, i);
        OutputDescriptor d;
        d.name = sd->name;
        d.description = sd->description;
        d.unit = sd->unit;
        d.hasFixedValueCount = sd->hasFixedValueCount;
        d.valueCount = sd->valueCount;
        for (unsigned int j = 0; j < sd->valueCount; ++j) {
            d.valueNames.push_back(sd->valueNames[i]);
        }
        d.hasKnownExtents = sd->hasKnownExtents;
        d.minValue = sd->minValue;
        d.maxValue = sd->maxValue;
        d.isQuantized = sd->isQuantized;
        d.quantizeStep = sd->quantizeStep;

        switch (sd->sampleType) {
        case svpOneSamplePerStep:
            d.sampleType = OutputDescriptor::OneSamplePerStep; break;
        case svpFixedSampleRate:
            d.sampleType = OutputDescriptor::FixedSampleRate; break;
        case svpVariableSampleRate:
            d.sampleType = OutputDescriptor::VariableSampleRate; break;
        }

        d.sampleRate = sd->sampleRate;

        list.push_back(d);

        m_descriptor->releaseOutputDescriptor(sd);
    }

    return list;
}

FeatureExtractionPluginHostAdapter::FeatureSet
FeatureExtractionPluginHostAdapter::process(float **inputBuffers,
                                            RealTime timestamp)
{
    FeatureSet fs;
    if (!m_handle) return fs;

    int sec = timestamp.sec;
    int nsec = timestamp.nsec;
    
    SVPFeatureList **features = m_descriptor->process(m_handle,
                                                      inputBuffers,
                                                      sec, nsec);
    
    convertFeatures(features, fs);
    m_descriptor->releaseFeatureSet(features);
    return fs;
}

FeatureExtractionPluginHostAdapter::FeatureSet
FeatureExtractionPluginHostAdapter::getRemainingFeatures()
{
    FeatureSet fs;
    if (!m_handle) return fs;
    
    SVPFeatureList **features = m_descriptor->getRemainingFeatures(m_handle); 

    convertFeatures(features, fs);
    m_descriptor->releaseFeatureSet(features);
    return fs;
}

void
FeatureExtractionPluginHostAdapter::convertFeatures(SVPFeatureList **features,
                                                    FeatureSet &fs)
{
    for (unsigned int i = 0; features[i]; ++i) {
        
        SVPFeatureList &list = *features[i];

        if (list.featureCount > 0) {

            for (unsigned int j = 0; j < list.featureCount; ++j) {
                
                Feature feature;
                feature.hasTimestamp = list.features[j].hasTimestamp;
                feature.timestamp = RealTime(list.features[j].sec,
                                             list.features[j].nsec);

                for (unsigned int k = 0; k < list.features[j].valueCount; ++k) {
                    feature.values.push_back(list.features[j].values[k]);
                }
                
                feature.label = list.features[j].label;

                fs[i].push_back(feature);
            }
        }
    }
}

