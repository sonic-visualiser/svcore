/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _FEATURE_EXTRACTION_PLUGIN_TRANSFORMER_H_
#define _FEATURE_EXTRACTION_PLUGIN_TRANSFORMER_H_

#include "PluginTransformer.h"

class DenseTimeValueModel;

class FeatureExtractionModelTransformer : public PluginTransformer
{
    Q_OBJECT

public:
    FeatureExtractionModelTransformer(Model *inputModel,
                                       QString plugin,
                                       ExecutionContext context,
                                       QString configurationXml = "",
                                       QString outputName = "");
    virtual ~FeatureExtractionModelTransformer();

protected:
    virtual void run();

    Vamp::Plugin *m_plugin;
    Vamp::Plugin::OutputDescriptor *m_descriptor;
    int m_outputFeatureNo;

    void addFeature(size_t blockFrame,
		    const Vamp::Plugin::Feature &feature);

    void setCompletion(int);

    void getFrames(int channel, int channelCount,
                   long startFrame, long size, float *buffer);

    // just casts
    DenseTimeValueModel *getInput();
    template <typename ModelClass> ModelClass *getOutput() {
	ModelClass *mc = dynamic_cast<ModelClass *>(m_output);
	if (!mc) {
	    std::cerr << "FeatureExtractionModelTransformer::getOutput: Output model not conformable" << std::endl;
	}
	return mc;
    }
};

#endif

