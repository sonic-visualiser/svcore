
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

#include "FeatureExtractionPluginTransform.h"

#include "plugin/FeatureExtractionPluginFactory.h"
#include "plugin/FeatureExtractionPlugin.h"

#include "base/Model.h"
#include "model/SparseOneDimensionalModel.h"
#include "model/SparseTimeValueModel.h"
#include "model/DenseThreeDimensionalModel.h"
#include "model/DenseTimeValueModel.h"

#include <iostream>

FeatureExtractionPluginTransform::FeatureExtractionPluginTransform(Model *inputModel,
								   QString pluginId,
                                                                   QString configurationXml,
								   QString outputName) :
    Transform(inputModel),
    m_plugin(0),
    m_descriptor(0),
    m_outputFeatureNo(0)
{
    std::cerr << "FeatureExtractionPluginTransform::FeatureExtractionPluginTransform: plugin " << pluginId.toStdString() << ", outputName " << outputName.toStdString() << std::endl;

    FeatureExtractionPluginFactory *factory =
	FeatureExtractionPluginFactory::instanceFor(pluginId);

    if (!factory) {
	std::cerr << "FeatureExtractionPluginTransform: No factory available for plugin id \""
		  << pluginId.toStdString() << "\"" << std::endl;
	return;
    }

    m_plugin = factory->instantiatePlugin(pluginId, m_input->getSampleRate());

    if (!m_plugin) {
	std::cerr << "FeatureExtractionPluginTransform: Failed to instantiate plugin \""
		  << pluginId.toStdString() << "\"" << std::endl;
	return;
    }

    if (configurationXml != "") {
        m_plugin->setParametersFromXml(configurationXml);
    }

    FeatureExtractionPlugin::OutputList outputs =
	m_plugin->getOutputDescriptors();

    if (outputs.empty()) {
	std::cerr << "FeatureExtractionPluginTransform: Plugin \""
		  << pluginId.toStdString() << "\" has no outputs" << std::endl;
	return;
    }
    
    for (size_t i = 0; i < outputs.size(); ++i) {
	if (outputName == "" || outputs[i].name == outputName.toStdString()) {
	    m_outputFeatureNo = i;
	    m_descriptor = new FeatureExtractionPlugin::OutputDescriptor
		(outputs[i]);
	    break;
	}
    }

    if (!m_descriptor) {
	std::cerr << "FeatureExtractionPluginTransform: Plugin \""
		  << pluginId.toStdString() << "\" has no output named \""
		  << outputName.toStdString() << "\"" << std::endl;
	return;
    }

    std::cerr << "FeatureExtractionPluginTransform: output sample type "
	      << m_descriptor->sampleType << std::endl;

    int valueCount = 1;
    float minValue = 0.0, maxValue = 0.0;
    
    if (m_descriptor->hasFixedValueCount) {
	valueCount = m_descriptor->valueCount;
    }

    if (valueCount > 0 && m_descriptor->hasKnownExtents) {
	minValue = m_descriptor->minValue;
	maxValue = m_descriptor->maxValue;
    }

    size_t modelRate = m_input->getSampleRate();
    size_t modelResolution = 1;
    
    switch (m_descriptor->sampleType) {

    case FeatureExtractionPlugin::OutputDescriptor::VariableSampleRate:
	if (m_descriptor->sampleRate != 0.0) {
	    modelResolution = size_t(modelRate / m_descriptor->sampleRate + 0.001);
	}
	break;

    case FeatureExtractionPlugin::OutputDescriptor::OneSamplePerStep:
	modelResolution = m_plugin->getPreferredStepSize();
	break;

    case FeatureExtractionPlugin::OutputDescriptor::FixedSampleRate:
	modelRate = m_descriptor->sampleRate;
	break;
    }

    if (valueCount == 0) {

	m_output = new SparseOneDimensionalModel(modelRate, modelResolution,
						 false);

    } else if (valueCount == 1 ||

	       // We don't have a sparse 3D model
	       m_descriptor->sampleType ==
	       FeatureExtractionPlugin::OutputDescriptor::VariableSampleRate) {
	
        SparseTimeValueModel *model = new SparseTimeValueModel
            (modelRate, modelResolution, minValue, maxValue, false);
        model->setScaleUnits(outputs[m_outputFeatureNo].unit.c_str());

        m_output = model;

    } else {
	
	m_output = new DenseThreeDimensionalModel(modelRate, modelResolution,
						  valueCount, false);

	if (!m_descriptor->valueNames.empty()) {
	    std::vector<QString> names;
	    for (size_t i = 0; i < m_descriptor->valueNames.size(); ++i) {
		names.push_back(m_descriptor->valueNames[i].c_str());
	    }
	    (dynamic_cast<DenseThreeDimensionalModel *>(m_output))
		->setBinNames(names);
	}
    }
}

FeatureExtractionPluginTransform::~FeatureExtractionPluginTransform()
{
    delete m_plugin;
    delete m_descriptor;
}

DenseTimeValueModel *
FeatureExtractionPluginTransform::getInput()
{
    DenseTimeValueModel *dtvm =
	dynamic_cast<DenseTimeValueModel *>(getInputModel());
    if (!dtvm) {
	std::cerr << "FeatureExtractionPluginTransform::getInput: WARNING: Input model is not conformable to DenseTimeValueModel" << std::endl;
    }
    return dtvm;
}

void
FeatureExtractionPluginTransform::run()
{
    DenseTimeValueModel *input = getInput();
    if (!input) return;

    if (!m_output) return;

    size_t channelCount = input->getChannelCount();
    if (m_plugin->getMaxChannelCount() < channelCount) {
	channelCount = 1;
    }
    if (m_plugin->getMinChannelCount() > channelCount) {
	std::cerr << "FeatureExtractionPluginTransform::run: "
		  << "Can't provide enough channels to plugin (plugin min "
		  << m_plugin->getMinChannelCount() << ", max "
		  << m_plugin->getMaxChannelCount() << ", input model has "
		  << input->getChannelCount() << ")" << std::endl;
	return;
    }

    size_t sampleRate = m_input->getSampleRate();

    size_t stepSize = m_plugin->getPreferredStepSize();
    size_t blockSize = m_plugin->getPreferredBlockSize();

    m_plugin->initialise(channelCount, stepSize, blockSize);

    float **buffers = new float*[channelCount];
    for (size_t ch = 0; ch < channelCount; ++ch) {
	buffers[ch] = new float[blockSize];
    }

    size_t startFrame = m_input->getStartFrame();
    size_t   endFrame = m_input->getEndFrame();
    size_t blockFrame = startFrame;

    size_t prevCompletion = 0;

    while (blockFrame < endFrame) {

//	std::cerr << "FeatureExtractionPluginTransform::run: blockFrame "
//		  << blockFrame << std::endl;

	size_t completion =
	    (((blockFrame - startFrame) / stepSize) * 99) /
	    (   (endFrame - startFrame) / stepSize);

	// channelCount is either m_input->channelCount or 1

	size_t got = 0;

	if (channelCount == 1) {
	    got = input->getValues
		(-1, blockFrame, blockFrame + blockSize, buffers[0]);
	    while (got < blockSize) {
		buffers[0][got++] = 0.0;
	    }
	} else {
	    for (size_t ch = 0; ch < channelCount; ++ch) {
		got = input->getValues
		    (ch, blockFrame, blockFrame + blockSize, buffers[ch]);
		while (got < blockSize) {
		    buffers[ch][got++] = 0.0;
		}
	    }
	}

	FeatureExtractionPlugin::FeatureSet features = m_plugin->process
	    (buffers, RealTime::frame2RealTime(blockFrame, sampleRate));

	for (size_t fi = 0; fi < features[m_outputFeatureNo].size(); ++fi) {
	    FeatureExtractionPlugin::Feature feature =
		features[m_outputFeatureNo][fi];
	    addFeature(blockFrame, feature);
	}

	if (blockFrame == startFrame || completion > prevCompletion) {
	    setCompletion(completion);
	    prevCompletion = completion;
	}

	blockFrame += stepSize;
    }

    FeatureExtractionPlugin::FeatureSet features = m_plugin->getRemainingFeatures();

    for (size_t fi = 0; fi < features[m_outputFeatureNo].size(); ++fi) {
	FeatureExtractionPlugin::Feature feature =
	    features[m_outputFeatureNo][fi];
	addFeature(blockFrame, feature);
    }

    setCompletion(100);
}


void
FeatureExtractionPluginTransform::addFeature(size_t blockFrame,
					     const FeatureExtractionPlugin::Feature &feature)
{
    size_t inputRate = m_input->getSampleRate();

//    std::cerr << "FeatureExtractionPluginTransform::addFeature("
//	      << blockFrame << ")" << std::endl;

    int valueCount = 1;
    if (m_descriptor->hasFixedValueCount) {
	valueCount = m_descriptor->valueCount;
    }

    size_t frame = blockFrame;

    if (m_descriptor->sampleType ==
	FeatureExtractionPlugin::OutputDescriptor::VariableSampleRate) {

	if (!feature.hasTimestamp) {
	    std::cerr
		<< "WARNING: FeatureExtractionPluginTransform::addFeature: "
		<< "Feature has variable sample rate but no timestamp!"
		<< std::endl;
	    return;
	} else {
	    frame = RealTime::realTime2Frame(feature.timestamp, inputRate);
	}

    } else if (m_descriptor->sampleType ==
	       FeatureExtractionPlugin::OutputDescriptor::FixedSampleRate) {

	if (feature.hasTimestamp) {
	    //!!! warning: sampleRate may be non-integral
	    frame = RealTime::realTime2Frame(feature.timestamp,
					     m_descriptor->sampleRate);
	} else {
	    frame = m_output->getEndFrame() + 1;
	}
    }
	
    if (valueCount == 0) {

	SparseOneDimensionalModel *model = getOutput<SparseOneDimensionalModel>();
	if (!model) return;
	model->addPoint(SparseOneDimensionalModel::Point(frame, feature.label.c_str()));
	
    } else if (valueCount == 1 ||
	       m_descriptor->sampleType == 
	       FeatureExtractionPlugin::OutputDescriptor::VariableSampleRate) {

	float value = 0.0;
	if (feature.values.size() > 0) value = feature.values[0];

	SparseTimeValueModel *model = getOutput<SparseTimeValueModel>();
	if (!model) return;
	model->addPoint(SparseTimeValueModel::Point(frame, value, feature.label.c_str()));
	
    } else {
	
	DenseThreeDimensionalModel::BinValueSet values = feature.values;
	
	DenseThreeDimensionalModel *model = getOutput<DenseThreeDimensionalModel>();
	if (!model) return;

	model->setBinValues(frame, values);
    }
}

void
FeatureExtractionPluginTransform::setCompletion(int completion)
{
    int valueCount = 1;
    if (m_descriptor->hasFixedValueCount) {
	valueCount = m_descriptor->valueCount;
    }

    if (valueCount == 0) {

	SparseOneDimensionalModel *model = getOutput<SparseOneDimensionalModel>();
	if (!model) return;
	model->setCompletion(completion);

    } else if (valueCount == 1 ||
	       m_descriptor->sampleType ==
	       FeatureExtractionPlugin::OutputDescriptor::VariableSampleRate) {

	SparseTimeValueModel *model = getOutput<SparseTimeValueModel>();
	if (!model) return;
	model->setCompletion(completion);

    } else {

	DenseThreeDimensionalModel *model = getOutput<DenseThreeDimensionalModel>();
	if (!model) return;
	model->setCompletion(completion);
    }
}

