/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005
    
    This is experimental software.  Not for distribution.
*/

#ifndef _FEATURE_EXTRACTION_PLUGIN_FACTORY_H_
#define _FEATURE_EXTRACTION_PLUGIN_FACTORY_H_

#include <QString>
#include <vector>

class FeatureExtractionPlugin;

class FeatureExtractionPluginFactory
{
public:
    static FeatureExtractionPluginFactory *instance(QString pluginType);
    static FeatureExtractionPluginFactory *instanceFor(QString identifier);
    static std::vector<QString> getAllPluginIdentifiers();

    std::vector<QString> getPluginIdentifiers();

    // We don't set blockSize or channels on this -- they're
    // negotiated and handled via initialize() on the plugin
    virtual FeatureExtractionPlugin *instantiatePlugin(QString identifier,
						       float inputSampleRate);

protected:
};

#endif
