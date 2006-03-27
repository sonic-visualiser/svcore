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

#include "FeatureExtractionPluginAdapter.h"

FeatureExtractionPluginAdapterBase::FeatureExtractionPluginAdapterBase() :
    m_populated(false)
{
}

const SVPPluginDescriptor *
FeatureExtractionPluginAdapterBase::getDescriptor()
{
    if (m_populated) return &m_descriptor;

    FeatureExtractionPlugin *plugin = createPlugin(48000);

    m_parameters = plugin->getParameterDescriptors();
    m_programs = plugin->getPrograms();
    
    m_descriptor.name = strdup(plugin->getName().c_str());
    m_descriptor.description = strdup(plugin->getDescription().c_str());
    m_descriptor.maker = strdup(plugin->getMaker().c_str());
    m_descriptor.pluginVersion = plugin->getPluginVersion();
    m_descriptor.copyright = strdup(plugin->getCopyright().c_str());
    
    m_descriptor.parameterCount = m_parameters.size();
    m_descriptor.parameters = (const SVPParameterDescriptor **)
        malloc(m_parameters.size() * sizeof(SVPParameterDescriptor));
    
    for (unsigned int i = 0; i < m_parameters.size(); ++i) {
        SVPParameterDescriptor *desc = (SVPParameterDescriptor *)
            malloc(sizeof(SVPParameterDescriptor));
        desc->name = strdup(m_parameters[i].name.c_str());
        desc->description = strdup(m_parameters[i].description.c_str());
        desc->unit = strdup(m_parameters[i].unit.c_str());
        desc->minValue = m_parameters[i].minValue;
        desc->maxValue = m_parameters[i].maxValue;
        desc->defaultValue = m_parameters[i].defaultValue;
        desc->isQuantized = m_parameters[i].isQuantized;
        desc->quantizeStep = m_parameters[i].quantizeStep;
        m_descriptor.parameters[i] = desc;
    }
    
    m_descriptor.programCount = m_programs.size();
    m_descriptor.programs = (const char **)
        malloc(m_programs.size() * sizeof(const char *));
    
    for (unsigned int i = 0; i < m_programs.size(); ++i) {
        m_descriptor.programs[i] = strdup(m_programs[i].c_str());
    }
    
    m_descriptor.instantiate = svpInstantiate;
    m_descriptor.cleanup = svpCleanup;
    m_descriptor.initialise = svpInitialise;
    m_descriptor.reset = svpReset;
    m_descriptor.getParameter = svpGetParameter;
    m_descriptor.setParameter = svpSetParameter;
    m_descriptor.getCurrentProgram = svpGetCurrentProgram;
    m_descriptor.selectProgram = svpSelectProgram;
    m_descriptor.getPreferredStepSize = svpGetPreferredStepSize;
    m_descriptor.getPreferredBlockSize = svpGetPreferredBlockSize;
    m_descriptor.getMinChannelCount = svpGetMinChannelCount;
    m_descriptor.getMaxChannelCount = svpGetMaxChannelCount;
    m_descriptor.getOutputCount = svpGetOutputCount;
    m_descriptor.getOutputDescriptor = svpGetOutputDescriptor;
    m_descriptor.releaseOutputDescriptor = svpReleaseOutputDescriptor;
    m_descriptor.process = svpProcess;
    m_descriptor.getRemainingFeatures = svpGetRemainingFeatures;
    m_descriptor.releaseFeatureSet = svpReleaseFeatureSet;
    
    m_adapterMap[&m_descriptor] = this;

    delete plugin;

    m_populated = true;
    return &m_descriptor;
}

FeatureExtractionPluginAdapterBase::~FeatureExtractionPluginAdapterBase()
{
    if (!m_populated) return;

    free((void *)m_descriptor.name);
    free((void *)m_descriptor.description);
    free((void *)m_descriptor.maker);
    free((void *)m_descriptor.copyright);
        
    for (unsigned int i = 0; i < m_descriptor.parameterCount; ++i) {
        const SVPParameterDescriptor *desc = m_descriptor.parameters[i];
        free((void *)desc->name);
        free((void *)desc->description);
        free((void *)desc->unit);
    }
    free((void *)m_descriptor.parameters);

    for (unsigned int i = 0; i < m_descriptor.programCount; ++i) {
        free((void *)m_descriptor.programs[i]);
    }
    free((void *)m_descriptor.programs);

    m_adapterMap.erase(&m_descriptor);
}

FeatureExtractionPluginAdapterBase *
FeatureExtractionPluginAdapterBase::lookupAdapter(SVPPluginHandle handle)
{
    AdapterMap::const_iterator i = m_adapterMap.find(handle);
    if (i == m_adapterMap.end()) return 0;
    return i->second;
}

SVPPluginHandle
FeatureExtractionPluginAdapterBase::svpInstantiate(const SVPPluginDescriptor *desc,
                                                   float inputSampleRate)
{
    if (m_adapterMap.find(desc) == m_adapterMap.end()) return 0;
    FeatureExtractionPluginAdapterBase *adapter = m_adapterMap[desc];
    if (desc != &adapter->m_descriptor) return 0;

    FeatureExtractionPlugin *plugin = adapter->createPlugin(inputSampleRate);
    if (plugin) {
        m_adapterMap[plugin] = adapter;
    }

    return plugin;
}

void
FeatureExtractionPluginAdapterBase::svpCleanup(SVPPluginHandle handle)
{
    FeatureExtractionPluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) {
        delete ((FeatureExtractionPlugin *)handle);
        return;
    }
    adapter->cleanup(((FeatureExtractionPlugin *)handle));
}

int
FeatureExtractionPluginAdapterBase::svpInitialise(SVPPluginHandle handle,
                                                  unsigned int channels,
                                                  unsigned int stepSize,
                                                  unsigned int blockSize)
{
    bool result = ((FeatureExtractionPlugin *)handle)->initialise
        (channels, stepSize, blockSize);
    return result ? 1 : 0;
}

void
FeatureExtractionPluginAdapterBase::svpReset(SVPPluginHandle handle) 
{
    ((FeatureExtractionPlugin *)handle)->reset();
}

float
FeatureExtractionPluginAdapterBase::svpGetParameter(SVPPluginHandle handle,
                                                    int param) 
{
    FeatureExtractionPluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0.0;
    FeatureExtractionPlugin::ParameterList &list = adapter->m_parameters;
    return ((FeatureExtractionPlugin *)handle)->getParameter(list[param].name);
}

void
FeatureExtractionPluginAdapterBase::svpSetParameter(SVPPluginHandle handle,
                                                    int param, float value)
{
    FeatureExtractionPluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return;
    FeatureExtractionPlugin::ParameterList &list = adapter->m_parameters;
    ((FeatureExtractionPlugin *)handle)->setParameter(list[param].name, value);
}

unsigned int
FeatureExtractionPluginAdapterBase::svpGetCurrentProgram(SVPPluginHandle handle)
{
    FeatureExtractionPluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    FeatureExtractionPlugin::ProgramList &list = adapter->m_programs;
    std::string program = ((FeatureExtractionPlugin *)handle)->getCurrentProgram();
    for (unsigned int i = 0; i < list.size(); ++i) {
        if (list[i] == program) return i;
    }
    return 0;
}

void
FeatureExtractionPluginAdapterBase::svpSelectProgram(SVPPluginHandle handle,
                                                     unsigned int program)
{
    FeatureExtractionPluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return;
    FeatureExtractionPlugin::ProgramList &list = adapter->m_programs;
    ((FeatureExtractionPlugin *)handle)->selectProgram(list[program]);
}

unsigned int
FeatureExtractionPluginAdapterBase::svpGetPreferredStepSize(SVPPluginHandle handle)
{
    return ((FeatureExtractionPlugin *)handle)->getPreferredStepSize();
}

unsigned int
FeatureExtractionPluginAdapterBase::svpGetPreferredBlockSize(SVPPluginHandle handle) 
{
    return ((FeatureExtractionPlugin *)handle)->getPreferredBlockSize();
}

unsigned int
FeatureExtractionPluginAdapterBase::svpGetMinChannelCount(SVPPluginHandle handle)
{
    return ((FeatureExtractionPlugin *)handle)->getMinChannelCount();
}

unsigned int
FeatureExtractionPluginAdapterBase::svpGetMaxChannelCount(SVPPluginHandle handle)
{
    return ((FeatureExtractionPlugin *)handle)->getMaxChannelCount();
}

unsigned int
FeatureExtractionPluginAdapterBase::svpGetOutputCount(SVPPluginHandle handle)
{
    FeatureExtractionPluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    return adapter->getOutputCount((FeatureExtractionPlugin *)handle);
}

SVPOutputDescriptor *
FeatureExtractionPluginAdapterBase::svpGetOutputDescriptor(SVPPluginHandle handle,
                                                           unsigned int i)
{
    FeatureExtractionPluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    return adapter->getOutputDescriptor((FeatureExtractionPlugin *)handle, i);
}

void
FeatureExtractionPluginAdapterBase::svpReleaseOutputDescriptor(SVPOutputDescriptor *desc)
{
    if (desc->name) free((void *)desc->name);
    if (desc->description) free((void *)desc->description);
    if (desc->unit) free((void *)desc->unit);
    for (unsigned int i = 0; i < desc->valueCount; ++i) {
        free((void *)desc->valueNames[i]);
    }
    if (desc->valueNames) free((void *)desc->valueNames);
    free((void *)desc);
}

SVPFeatureList **
FeatureExtractionPluginAdapterBase::svpProcess(SVPPluginHandle handle,
                                               float **inputBuffers,
                                               int sec,
                                               int nsec)
{
    FeatureExtractionPluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    return adapter->process((FeatureExtractionPlugin *)handle,
                            inputBuffers, sec, nsec);
}

SVPFeatureList **
FeatureExtractionPluginAdapterBase::svpGetRemainingFeatures(SVPPluginHandle handle)
{
    FeatureExtractionPluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    return adapter->getRemainingFeatures((FeatureExtractionPlugin *)handle);
}

void
FeatureExtractionPluginAdapterBase::svpReleaseFeatureSet(SVPFeatureList **fs)
{
    if (!fs) return;
    for (unsigned int i = 0; fs[i]; ++i) {
        for (unsigned int j = 0; j < fs[i]->featureCount; ++j) {
            SVPFeature *feature = &fs[i]->features[j];
            if (feature->values) free((void *)feature->values);
            if (feature->label) free((void *)feature->label);
            free((void *)feature);
        }
        if (fs[i]->features) free((void *)fs[i]->features);
        free((void *)fs[i]);
    }
    free((void *)fs);
}

void 
FeatureExtractionPluginAdapterBase::cleanup(FeatureExtractionPlugin *plugin)
{
    if (m_pluginOutputs.find(plugin) != m_pluginOutputs.end()) {
        delete m_pluginOutputs[plugin];
        m_pluginOutputs.erase(plugin);
    }
    m_adapterMap.erase(plugin);
    delete ((FeatureExtractionPlugin *)plugin);
}

void 
FeatureExtractionPluginAdapterBase::checkOutputMap(FeatureExtractionPlugin *plugin)
{
    if (!m_pluginOutputs[plugin]) {
        m_pluginOutputs[plugin] = new FeatureExtractionPlugin::OutputList
            (plugin->getOutputDescriptors());
    }
}

unsigned int 
FeatureExtractionPluginAdapterBase::getOutputCount(FeatureExtractionPlugin *plugin)
{
    checkOutputMap(plugin);
    return m_pluginOutputs[plugin]->size();
}

SVPOutputDescriptor *
FeatureExtractionPluginAdapterBase::getOutputDescriptor(FeatureExtractionPlugin *plugin,
                                                        unsigned int i)
{
    checkOutputMap(plugin);
    FeatureExtractionPlugin::OutputDescriptor &od =
        (*m_pluginOutputs[plugin])[i];

    SVPOutputDescriptor *desc = (SVPOutputDescriptor *)
        malloc(sizeof(SVPOutputDescriptor));

    desc->name = strdup(od.name.c_str());
    desc->description = strdup(od.description.c_str());
    desc->unit = strdup(od.unit.c_str());
    desc->hasFixedValueCount = od.hasFixedValueCount;
    desc->valueCount = od.valueCount;

    desc->valueNames = (const char **)
        malloc(od.valueCount * sizeof(const char *));
        
    for (unsigned int i = 0; i < od.valueCount; ++i) {
        desc->valueNames[i] = strdup(od.valueNames[i].c_str());
    }

    desc->hasKnownExtents = od.hasKnownExtents;
    desc->minValue = od.minValue;
    desc->maxValue = od.maxValue;
    desc->isQuantized = od.isQuantized;
    desc->quantizeStep = od.quantizeStep;

    switch (od.sampleType) {
    case FeatureExtractionPlugin::OutputDescriptor::OneSamplePerStep:
        desc->sampleType = svpOneSamplePerStep; break;
    case FeatureExtractionPlugin::OutputDescriptor::FixedSampleRate:
        desc->sampleType = svpFixedSampleRate; break;
    case FeatureExtractionPlugin::OutputDescriptor::VariableSampleRate:
        desc->sampleType = svpVariableSampleRate; break;
    }

    desc->sampleRate = od.sampleRate;

    return desc;
}
    
SVPFeatureList **
FeatureExtractionPluginAdapterBase::process(FeatureExtractionPlugin *plugin,
                                            float **inputBuffers,
                                            int sec, int nsec)
{
    RealTime rt(sec, nsec);
    return convertFeatures(plugin->process(inputBuffers, rt));
}
    
SVPFeatureList **
FeatureExtractionPluginAdapterBase::getRemainingFeatures(FeatureExtractionPlugin *plugin)
{
    return convertFeatures(plugin->getRemainingFeatures());
}

SVPFeatureList **
FeatureExtractionPluginAdapterBase::convertFeatures(const FeatureExtractionPlugin::FeatureSet &features)
{
    unsigned int n = 0;
    if (features.begin() != features.end()) {
        FeatureExtractionPlugin::FeatureSet::const_iterator i = features.end();
        --i;
        n = i->first + 1;
    }

    if (!n) return 0;

    SVPFeatureList **fs = (SVPFeatureList **)
        malloc((n + 1) * sizeof(SVPFeatureList *));

    for (unsigned int i = 0; i < n; ++i) {
        fs[i] = (SVPFeatureList *)malloc(sizeof(SVPFeatureList));
        if (features.find(i) == features.end()) {
            fs[i]->featureCount = 0;
            fs[i]->features = 0;
        } else {
            FeatureExtractionPlugin::FeatureSet::const_iterator fi =
                features.find(i);
            const FeatureExtractionPlugin::FeatureList &fl = fi->second;
            fs[i]->featureCount = fl.size();
            fs[i]->features = (SVPFeature *)malloc(fl.size() *
                                                   sizeof(SVPFeature));
            for (unsigned int j = 0; j < fl.size(); ++j) {
                fs[i]->features[j].hasTimestamp = fl[j].hasTimestamp;
                fs[i]->features[j].sec = fl[j].timestamp.sec;
                fs[i]->features[j].nsec = fl[j].timestamp.nsec;
                fs[i]->features[j].valueCount = fl[j].values.size();
                fs[i]->features[j].values = (float *)malloc
                    (fs[i]->features[j].valueCount * sizeof(float));
                for (unsigned int k = 0; k < fs[i]->features[j].valueCount; ++k) {
                    fs[i]->features[j].values[k] = fl[j].values[k];
                }
                fs[i]->features[j].label = strdup(fl[j].label.c_str());
            }
        }
    }

    fs[n] = 0;

    return fs;
}

FeatureExtractionPluginAdapterBase::AdapterMap 
FeatureExtractionPluginAdapterBase::m_adapterMap;


