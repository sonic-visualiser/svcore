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

#ifndef _FEATURE_EXTRACTION_PLUGIN_FACTORY_H_
#define _FEATURE_EXTRACTION_PLUGIN_FACTORY_H_

#include <QString>
#include <vector>

namespace Vamp { class Plugin; }

class FeatureExtractionPluginFactory
{
public:
    static FeatureExtractionPluginFactory *instance(QString pluginType);
    static FeatureExtractionPluginFactory *instanceFor(QString identifier);
    static std::vector<QString> getAllPluginIdentifiers();

    virtual std::vector<QString> getPluginPath();

    virtual std::vector<QString> getPluginIdentifiers();

    virtual QString findPluginFile(QString soname, QString inDir = "");

    // We don't set blockSize or channels on this -- they're
    // negotiated and handled via initialize() on the plugin
    virtual Vamp::Plugin *instantiatePlugin(QString identifier,
                                            float inputSampleRate);

protected:
    std::vector<QString> m_pluginPath;
};

#endif
