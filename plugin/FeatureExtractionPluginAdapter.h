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

#ifndef _FEATURE_EXTRACTION_PLUGIN_ADAPTER_H_
#define _FEATURE_EXTRACTION_PLUGIN_ADAPTER_H_

#include "api/svp.h"
#include "FeatureExtractionPlugin.h"

#include <map>

class FeatureExtractionPluginAdapterBase
{
public:
    virtual ~FeatureExtractionPluginAdapterBase();
    const SVPPluginDescriptor *getDescriptor();

protected:
    FeatureExtractionPluginAdapterBase();

    virtual FeatureExtractionPlugin *createPlugin(float inputSampleRate) = 0;

    static SVPPluginHandle svpInstantiate(const SVPPluginDescriptor *desc,
                                          float inputSampleRate);

    static void svpCleanup(SVPPluginHandle handle);

    static int svpInitialise(SVPPluginHandle handle, unsigned int channels,
                             unsigned int stepSize, unsigned int blockSize);

    static void svpReset(SVPPluginHandle handle);

    static float svpGetParameter(SVPPluginHandle handle, int param);
    static void svpSetParameter(SVPPluginHandle handle, int param, float value);

    static unsigned int svpGetCurrentProgram(SVPPluginHandle handle);
    static void svpSelectProgram(SVPPluginHandle handle, unsigned int program);

    static unsigned int svpGetPreferredStepSize(SVPPluginHandle handle);
    static unsigned int svpGetPreferredBlockSize(SVPPluginHandle handle);
    static unsigned int svpGetMinChannelCount(SVPPluginHandle handle);
    static unsigned int svpGetMaxChannelCount(SVPPluginHandle handle);

    static unsigned int svpGetOutputCount(SVPPluginHandle handle);

    static SVPOutputDescriptor *svpGetOutputDescriptor(SVPPluginHandle handle,
                                                       unsigned int i);

    static void svpReleaseOutputDescriptor(SVPOutputDescriptor *desc);

    static SVPFeatureList **svpProcess(SVPPluginHandle handle,
                                       float **inputBuffers,
                                       int sec,
                                       int nsec);

    static SVPFeatureList **svpGetRemainingFeatures(SVPPluginHandle handle);

    static void svpReleaseFeatureSet(SVPFeatureList **fs);

    void cleanup(FeatureExtractionPlugin *plugin);
    void checkOutputMap(FeatureExtractionPlugin *plugin);
    unsigned int getOutputCount(FeatureExtractionPlugin *plugin);
    SVPOutputDescriptor *getOutputDescriptor(FeatureExtractionPlugin *plugin,
                                             unsigned int i);
    SVPFeatureList **process(FeatureExtractionPlugin *plugin,
                             float **inputBuffers,
                             int sec, int nsec);
    SVPFeatureList **getRemainingFeatures(FeatureExtractionPlugin *plugin);
    SVPFeatureList **convertFeatures(const FeatureExtractionPlugin::FeatureSet &features);
    
    typedef std::map<const void *, FeatureExtractionPluginAdapterBase *> AdapterMap;
    static AdapterMap m_adapterMap;
    static FeatureExtractionPluginAdapterBase *lookupAdapter(SVPPluginHandle);

    bool m_populated;
    SVPPluginDescriptor m_descriptor;
    FeatureExtractionPlugin::ParameterList m_parameters;
    FeatureExtractionPlugin::ProgramList m_programs;
    
    typedef std::map<FeatureExtractionPlugin *,
                     FeatureExtractionPlugin::OutputList *> OutputMap;
    OutputMap m_pluginOutputs;

    typedef std::map<FeatureExtractionPlugin *,
                     SVPFeature ***> FeatureBufferMap;
    FeatureBufferMap m_pluginFeatures;
};

template <typename Plugin>
class FeatureExtractionPluginAdapter : public FeatureExtractionPluginAdapterBase
{
public:
    FeatureExtractionPluginAdapter() : FeatureExtractionPluginAdapterBase() { }
    ~FeatureExtractionPluginAdapter() { }

protected:
    FeatureExtractionPlugin *createPlugin(float inputSampleRate) {
        Plugin *plugin = new Plugin(inputSampleRate);
        FeatureExtractionPlugin *fep =
            dynamic_cast<FeatureExtractionPlugin *>(plugin);
        if (!fep) {
            std::cerr << "ERROR: FeatureExtractionPlugin::createPlugin: "
                      << "Plugin is not a feature extraction plugin"
                      << std::endl;
            delete plugin;
            return 0;
        }
        return fep;
    }
};
    

#endif

