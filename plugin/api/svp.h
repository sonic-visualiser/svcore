/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser Plugin API 
    A plugin interface for audio feature extraction plugins.
    Centre for Digital Music, Queen Mary, University of London.
    Copyright 2006 Chris Cannam.
 
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License
    as published by the Free Software Foundation; either version 2.1
    of the License, or (at your option) any later version.  See the
    file COPYING included with this distribution for more information.
*/

#ifndef SVP_HEADER_INCLUDED
#define SVP_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SVPParameterDescriptor
{
    const char *name;
    const char *description;
    const char *unit;
    float minValue;
    float maxValue;
    float defaultValue;
    int isQuantized;
    float quantizeStep;

} SVPParameterDescriptor;

typedef enum
{
    svpOneSamplePerStep,
    svpFixedSampleRate,
    svpVariableSampleRate

} SVPSampleType;

typedef struct _SVPOutputDescriptor
{
    const char *name;
    const char *description;
    const char *unit;
    int hasFixedValueCount;
    unsigned int valueCount;
    const char **valueNames;
    int hasKnownExtents;
    float minValue;
    float maxValue;
    int isQuantized;
    float quantizeStep;
    SVPSampleType sampleType;
    float sampleRate;

} SVPOutputDescriptor;

typedef struct _SVPFeature
{
    int hasTimestamp;
    int sec;
    int nsec;
    unsigned int valueCount;
    float *values;
    char *label;

} SVPFeature;

typedef struct _SVPFeatureList
{
    unsigned int featureCount;
    SVPFeature *features;

} SVPFeatureList;

typedef void *SVPPluginHandle;

typedef struct _SVPPluginDescriptor
{
    const char *name;
    const char *description;
    const char *maker;
    int pluginVersion;
    const char *copyright;
    unsigned int parameterCount;
    const SVPParameterDescriptor **parameters;
    unsigned int programCount;
    const char **programs;
    
    SVPPluginHandle (*instantiate)(const struct _SVPPluginDescriptor *,
                                   float inputSampleRate);

    void (*cleanup)(SVPPluginHandle);

    int (*initialise)(SVPPluginHandle,
                      unsigned int inputChannels,
                      unsigned int stepSize, 
                      unsigned int blockSize);

    void (*reset)(SVPPluginHandle);

    float (*getParameter)(SVPPluginHandle, int);
    void  (*setParameter)(SVPPluginHandle, int, float);

    unsigned int (*getCurrentProgram)(SVPPluginHandle);
    void  (*selectProgram)(SVPPluginHandle, unsigned int);
    
    unsigned int (*getPreferredStepSize)(SVPPluginHandle);
    unsigned int (*getPreferredBlockSize)(SVPPluginHandle);
    unsigned int (*getMinChannelCount)(SVPPluginHandle);
    unsigned int (*getMaxChannelCount)(SVPPluginHandle);
    unsigned int (*getOutputCount)(SVPPluginHandle);

    SVPOutputDescriptor *(*getOutputDescriptor)(SVPPluginHandle,
                                                unsigned int);
    void (*releaseOutputDescriptor)(SVPOutputDescriptor *);

    SVPFeatureList **(*process)(SVPPluginHandle,
                                float **inputBuffers,
                                int sec,
                                int nsec);
    SVPFeatureList **(*getRemainingFeatures)(SVPPluginHandle);
    void (*releaseFeatureSet)(SVPFeatureList **);

} SVPPluginDescriptor;

const SVPPluginDescriptor *svpGetPluginDescriptor(unsigned int index);

typedef const SVPPluginDescriptor *(SVPGetPluginDescriptorFunction)(unsigned int);

#ifdef __cplusplus
}
#endif

#endif
