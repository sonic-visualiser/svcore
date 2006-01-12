/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _FEATURE_EXTRACTION_PLUGIN_TRANSFORM_H_
#define _FEATURE_EXTRACTION_PLUGIN_TRANSFORM_H_

#include "Transform.h"
#include "FeatureExtractionPlugin.h"

class DenseTimeValueModel;

class FeatureExtractionPluginTransform : public Transform
{
public:
    FeatureExtractionPluginTransform(Model *inputModel,
				     QString plugin,
				     QString outputName = "");
    virtual ~FeatureExtractionPluginTransform();

protected:
    virtual void run();

    FeatureExtractionPlugin *m_plugin;
    FeatureExtractionPlugin::OutputDescriptor *m_descriptor;
    int m_outputFeatureNo;

    void addFeature(size_t blockFrame,
		    const FeatureExtractionPlugin::Feature &feature);

    void setCompletion(int);

    // just casts
    DenseTimeValueModel *getInput();
    template <typename ModelClass> ModelClass *getOutput() {
	ModelClass *mc = dynamic_cast<ModelClass *>(m_output);
	if (!mc) {
	    std::cerr << "FeatureExtractionPluginTransform::getOutput: Output model not conformable" << std::endl;
	}
	return mc;
    }
};

#endif

