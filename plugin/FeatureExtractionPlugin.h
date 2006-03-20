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

#ifndef _FEATURE_EXTRACTION_PLUGIN_H_
#define _FEATURE_EXTRACTION_PLUGIN_H_

#include "PluginInstance.h"

#include <string>
#include <vector>
#include <map>

#include "base/RealTime.h"

/**
 * FeatureExtractionPlugin is a base class for plugin instance classes
 * that provide feature extraction from audio or related data.
 *
 * In most cases, the input will be audio and the output will be a
 * stream of derived data at a lower sampling resolution than the
 * input.
 *
 * Note that this class inherits several abstract methods from
 * PluginInstance, that must be implemented by the subclass.
 */

class FeatureExtractionPlugin : public PluginInstance
{
public:
    /**
     * Initialise a plugin to prepare it for use with the given number
     * of input channels, step size (window increment, in sample
     * frames) and block size (window size, in sample frames).
     *
     * The input sample rate should have been already specified at
     * construction time.
     * 
     * Return true for successful initialisation, false if the number
     * of input channels, step size and/or block size cannot be
     * supported.
     */
    virtual bool initialise(size_t inputChannels,
			    size_t stepSize,
			    size_t blockSize) = 0;

    /**
     * Reset the plugin after use, to prepare it for another clean
     * run.  Not called for the first initialisation (i.e. initialise
     * must also do a reset).
     */
    virtual void reset() = 0;

    /**
     * Get the preferred step size (window increment -- the distance
     * in sample frames between the start frames of consecutive blocks
     * passed to the process() function) for the plugin.  This should
     * be called before initialise().
     */
    virtual size_t getPreferredStepSize() const = 0;

    /**
     * Get the preferred block size (window size -- the number of
     * sample frames passed in each block to the process() function).
     * This should be called before initialise().
     */
    virtual size_t getPreferredBlockSize() const { return getPreferredStepSize(); }

    /**
     * Get the minimum supported number of input channels.
     */
    virtual size_t getMinChannelCount() const { return 1; }

    /**
     * Get the maximum supported number of input channels.
     */
    virtual size_t getMaxChannelCount() const { return 1; }

    struct OutputDescriptor
    {
	/**
	 * The name of the output, in computer-usable form.  Should be
	 * reasonably short and without whitespace or punctuation.
	 */
	std::string name;

	/**
	 * The human-readable name of the output.
	 */
	std::string description;

	/**
	 * The unit of the output, in human-readable form.
	 */
	std::string unit;

	/**
	 * True if the output has the same number of values per result
	 * for every output result.  Outputs for which this is false
	 * are unlikely to be very useful in a general-purpose host.
	 */
	bool hasFixedValueCount;

	/**
	 * The number of values per result of the output.  Undefined
	 * if hasFixedValueCount is false.  If this is zero, the output
	 * is point data (i.e. only the time of each output is of
	 * interest, the value list will be empty).
	 *
	 * Note that this gives the number of values of a single
	 * output result, not of the output stream (which has one more
	 * dimension: time).
	 */
	size_t valueCount;

	/**
	 * The names of each of the values, if appropriate.  This is
	 * always optional.
	 */
	std::vector<std::string> valueNames;

	/**
	 * True if the results in the output have a fixed numeric
	 * range (minimum and maximum values).  Undefined if
	 * valueCount is zero.
	 */
	bool hasKnownExtents;

	/**
	 * Minimum value of the results in the output.  Undefined if
	 * hasKnownExtents is false or valueCount is zero.
	 */
	float minValue;

	/**
	 * Maximum value of the results in the output.  Undefined if
	 * hasKnownExtents is false or valueCount is zero.
	 */
	float maxValue;

	/**
	 * True if the output values are quantized to a particular
	 * resolution.  Undefined if valueCount is zero.
	 */
	bool isQuantized;

	/**
	 * Quantization resolution of the output values (e.g. 1.0 if
	 * they are all integers).  Undefined if isQuantized is false
	 * or valueCount is zero.
	 */
	float quantizeStep;

	enum SampleType {

	    /// Results from each process() align with that call's block start
	    OneSamplePerStep,

	    /// Results are evenly spaced in time (sampleRate specified below)
	    FixedSampleRate,

	    /// Results are unevenly spaced and have individual timestamps
	    VariableSampleRate
	};

	/**
	 * Positioning in time of the output results.
	 */
	SampleType sampleType;

	/**
	 * Sample rate of the output results.  Undefined if sampleType
	 * is OneSamplePerStep.
	 *
	 * If sampleType is VariableSampleRate and this value is
	 * non-zero, then it may be used to calculate a resolution for
	 * the output (i.e. the "duration" of each value, in time).
	 * It's recommended to set this to zero if that behaviour is
	 * not desired.
	 */
	float sampleRate;
    };

    typedef std::vector<OutputDescriptor> OutputList;

    /**
     * Get the outputs of this plugin.  An output's index in this list
     * is used as its numeric index when looking it up in the
     * FeatureSet returned from the process() call.
     */
    virtual OutputList getOutputDescriptors() const = 0;

    struct Feature
    {
	/**
	 * True if an output feature has its own timestamp.  This is
	 * mandatory if the output has VariableSampleRate, and is
	 * likely to be disregarded otherwise.
	 */
	bool hasTimestamp;

	/**
	 * Timestamp of the output feature.  This is mandatory if the
	 * output has VariableSampleRate, and is likely to be
	 * disregarded otherwise.  Undefined if hasTimestamp is false.
	 */
	RealTime timestamp;
	
	/**
	 * Results for a single sample of this feature.  If the output
	 * hasFixedValueCount, there must be the same number of values
	 * as the output's valueCount count.
	 */
	std::vector<float> values;

	/**
	 * Label for the sample of this feature.
	 */
	std::string label;
    };

    typedef std::vector<Feature> FeatureList;
    typedef std::map<int, FeatureList> FeatureSet; // key is output no

    /**
     * Process a single block of input data.  inputBuffers points to
     * one array of floats per input channel, and each of those arrays
     * contains the blockSize number of samples (the host will
     * zero-pad as necessary).  The timestamp is the real time in
     * seconds of the start of the supplied block of samples.
     *
     * Return any features that have become available after this
     * process call.  (These do not necessarily have to fall within
     * the process block, except for OneSamplePerStep outputs.)
     */
    virtual FeatureSet process(float **inputBuffers,
			       RealTime timestamp) = 0;

    /**
     * After all blocks have been processed, calculate and return any
     * remaining features derived from the complete input.
     */
    virtual FeatureSet getRemainingFeatures() = 0;

protected:
    FeatureExtractionPlugin(float inputSampleRate) :
	m_inputSampleRate(inputSampleRate) { }

    float m_inputSampleRate;
};

#endif



