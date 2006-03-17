/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "FeatureExtractionPluginFactory.h"
#include "PluginIdentifier.h"

#include "plugins/BeatDetect.h" //!!!
#include "plugins/ChromagramPlugin.h" //!!!
#include "plugins/ZeroCrossing.h" //!!!
#include "plugins/SpectralCentroid.h" //!!!
#include "plugins/TonalChangeDetect.h" //!!!

#include <iostream>

static FeatureExtractionPluginFactory *_nativeInstance = 0;

FeatureExtractionPluginFactory *
FeatureExtractionPluginFactory::instance(QString pluginType)
{
    if (pluginType == "sv") {
	if (!_nativeInstance) {
	    std::cerr << "FeatureExtractionPluginFactory::instance(" << pluginType.toStdString()
		      << "): creating new FeatureExtractionPluginFactory" << std::endl;
	    _nativeInstance = new FeatureExtractionPluginFactory();
	}
	return _nativeInstance;
    }

    else return 0;
}

FeatureExtractionPluginFactory *
FeatureExtractionPluginFactory::instanceFor(QString identifier)
{
    QString type, soName, label;
    PluginIdentifier::parseIdentifier(identifier, type, soName, label);
    return instance(type);
}

std::vector<QString>
FeatureExtractionPluginFactory::getAllPluginIdentifiers()
{
    FeatureExtractionPluginFactory *factory;
    std::vector<QString> rv;
    
    factory = instance("sv");
    if (factory) {
	std::vector<QString> tmp = factory->getPluginIdentifiers();
	for (size_t i = 0; i < tmp.size(); ++i) {
	    rv.push_back(tmp[i]);
	}
    }

    // Plugins can change the locale, revert it to default.
    setlocale(LC_ALL, "C");
    return rv;
}

std::vector<QString>
FeatureExtractionPluginFactory::getPluginIdentifiers()
{
    std::vector<QString> rv;
    rv.push_back("sv:_builtin:beats"); //!!!
    rv.push_back("sv:_builtin:chromagram"); //!!!
    rv.push_back("sv:_builtin:zerocrossing"); //!!!
    rv.push_back("sv:_builtin:spectralcentroid"); //!!!
    rv.push_back("sv:_builtin:tonalchange"); //!!!
    return rv;
}

FeatureExtractionPlugin *
FeatureExtractionPluginFactory::instantiatePlugin(QString identifier,
						  float inputSampleRate)
{
    QString type, soName, label;
    PluginIdentifier::parseIdentifier(identifier, type, soName, label);
    if (type != "sv") {
	std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: Wrong factory for plugin type " << type.toStdString() << std::endl;
	return 0;
    }

    //!!!
    if (soName != PluginIdentifier::BUILTIN_PLUGIN_SONAME) {
	std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: Non-built-in plugins not yet supported (paradoxically enough)" << std::endl;
	return 0;
    }

    if (label == "beats") {
	return new BeatDetector(inputSampleRate); //!!!
    }

    if (label == "chromagram") {
	return new ChromagramPlugin(inputSampleRate); //!!!
    }

    if (label == "zerocrossing") {
	return new ZeroCrossing(inputSampleRate); //!!!
    }

    if (label == "spectralcentroid") {
	return new SpectralCentroid(inputSampleRate); //!!!
    }

    if (label == "tonalchange") {
	return new TonalChangeDetect(inputSampleRate); //!!!
    }

    std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: Unknown plugin \"" << identifier.toStdString() << "\"" << std::endl;
    
    return 0;
}

