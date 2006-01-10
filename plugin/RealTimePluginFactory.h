/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2005 Chris Cannam.
*/

#ifndef _REALTIME_PLUGIN_FACTORY_H_
#define _REALTIME_PLUGIN_FACTORY_H_

#include <QString>
#include <vector>

class RealTimePluginInstance;

class RealTimePluginFactory
{
public:
    static RealTimePluginFactory *instance(QString pluginType);
    static RealTimePluginFactory *instanceFor(QString identifier);
    static std::vector<QString> getAllPluginIdentifiers();
    static void enumerateAllPlugins(std::vector<QString> &);

    static void setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }

    /**
     * Look up the plugin path and find the plugins in it.  Called 
     * automatically after construction of a factory.
     */
    virtual void discoverPlugins() = 0;

    /**
     * Return a reference to a list of all plugin identifiers that can
     * be created by this factory.
     */
    virtual const std::vector<QString> &getPluginIdentifiers() const = 0;

    /**
     * Append to the given list descriptions of all the available
     * plugins and their ports.  This is in a standard format, see
     * the LADSPA implementation for details.
     */
    virtual void enumeratePlugins(std::vector<QString> &list) = 0;

    /**
     * Instantiate a plugin.
     */
    virtual RealTimePluginInstance *instantiatePlugin(QString identifier,
						      int clientId,
						      int position,
						      unsigned int sampleRate,
						      unsigned int blockSize,
						      unsigned int channels) = 0;

protected:
    RealTimePluginFactory() { }

    // for call by RealTimePluginInstance dtor
    virtual void releasePlugin(RealTimePluginInstance *, QString identifier) = 0;
    friend class RealTimePluginInstance;

    static int m_sampleRate;
};

#endif
