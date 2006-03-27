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

#ifndef _FEATURE_EXTRACTION_PLUGIN_HOST_ADAPTER_H_
#define _FEATURE_EXTRACTION_PLUGIN_HOST_ADAPTER_H_

#include "api/svp.h"

#include "FeatureExtractionPlugin.h"

class FeatureExtractionPluginHostAdapter : public FeatureExtractionPlugin
{
public:
    FeatureExtractionPluginHostAdapter(const SVPPluginDescriptor *descriptor,
                                       float inputSampleRate);
    virtual ~FeatureExtractionPluginHostAdapter();

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    std::string getName() const;
    std::string getDescription() const;
    std::string getMaker() const;
    int getPluginVersion() const;
    std::string getCopyright() const;

    ParameterList getParameterDescriptors() const;
    float getParameter(std::string) const;
    void setParameter(std::string, float);

    ProgramList getPrograms() const;
    std::string getCurrentProgram() const;
    void selectProgram(std::string);

    size_t getPreferredStepSize() const;
    size_t getPreferredBlockSize() const;

    OutputList getOutputDescriptors() const;

    FeatureSet process(float **inputBuffers, RealTime timestamp);

    FeatureSet getRemainingFeatures();

protected:
    void convertFeatures(SVPFeatureList **, FeatureSet &);

    const SVPPluginDescriptor *m_descriptor;
    SVPPluginHandle m_handle;
};

#endif


