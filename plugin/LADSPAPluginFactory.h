/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2005 Chris Cannam and Richard Bown.
*/

#ifndef _LADSPA_PLUGIN_FACTORY_H_
#define _LADSPA_PLUGIN_FACTORY_H_

#include "RealTimePluginFactory.h"
#include "api/ladspa.h"

#include <vector>
#include <map>
#include <set>
#include <QString>

class LADSPAPluginInstance;

class LADSPAPluginFactory : public RealTimePluginFactory
{
public:
    virtual ~LADSPAPluginFactory();

    virtual void discoverPlugins();

    virtual const std::vector<QString> &getPluginIdentifiers() const;

    virtual void enumeratePlugins(std::vector<QString> &list);

    virtual RealTimePluginInstance *instantiatePlugin(QString identifier,
						      int clientId,
						      int position,
						      unsigned int sampleRate,
						      unsigned int blockSize,
						      unsigned int channels);

    float getPortMinimum(const LADSPA_Descriptor *, int port);
    float getPortMaximum(const LADSPA_Descriptor *, int port);
    float getPortDefault(const LADSPA_Descriptor *, int port);
    int getPortDisplayHint(const LADSPA_Descriptor *, int port);

protected:
    LADSPAPluginFactory();
    friend class RealTimePluginFactory;

    virtual std::vector<QString> getPluginPath();

#ifdef HAVE_LIBLRDF
    virtual std::vector<QString> getLRDFPath(QString &baseUri);
#endif

    virtual void discoverPlugins(QString soName);
    virtual void generateTaxonomy(QString uri, QString base);
    virtual void generateFallbackCategories();

    virtual void releasePlugin(RealTimePluginInstance *, QString);

    virtual const LADSPA_Descriptor *getLADSPADescriptor(QString identifier);

    void loadLibrary(QString soName);
    void unloadLibrary(QString soName);
    void unloadUnusedLibraries();

    std::vector<QString> m_identifiers;

    std::map<unsigned long, QString> m_taxonomy;
    std::map<QString, QString> m_fallbackCategories;
    std::map<unsigned long, std::map<int, float> > m_portDefaults;

    std::set<RealTimePluginInstance *> m_instances;

    typedef std::map<QString, void *> LibraryHandleMap;
    LibraryHandleMap m_libraryHandles;
};

#endif

